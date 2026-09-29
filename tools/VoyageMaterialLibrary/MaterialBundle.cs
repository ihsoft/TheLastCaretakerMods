using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace VoyageMaterialLibrary;

internal static partial class Program
{
    static readonly string[] BundlePbrRoles = ["baseColor", "normal", "orm", "emissive", "opacity"];

    internal static int FinalizeMaterialBundle(string requestPath)
    {
        var request = JsonNode.Parse(File.ReadAllText(requestPath, Encoding.UTF8))!.AsObject();
        var bundleName = request["bundleName"]?.GetValue<string>() ?? "";
        if (!System.Text.RegularExpressions.Regex.IsMatch(bundleName, "^[A-Za-z0-9_.-]+$") || bundleName is "." or "..")
            throw new ArgumentException("bundleName must contain only letters, digits, dot, underscore or hyphen.");
        var output = Path.GetFullPath(request["outputPath"]!.GetValue<string>());
        if (File.Exists(output) || Path.GetFileName(output) != bundleName + ".materialbundle.zip")
            throw new IOException("Output must be a fresh <BundleName>.materialbundle.zip path.");
        var packs = request["materialPacks"]!.AsArray().Select(x => Path.GetFullPath(x!.GetValue<string>())).ToArray();
        if (packs.Length is < 1 or > 128 || packs.Distinct(StringComparer.OrdinalIgnoreCase).Count() != packs.Length)
            throw new ArgumentException("materialPacks must contain 1..128 unique paths.");

        var materialPayloads = new SortedDictionary<string, byte[]>(StringComparer.Ordinal);
        var previewPayloads = new SortedDictionary<string, byte[]>(StringComparer.Ordinal);
        var texturePayloads = new SortedDictionary<string, byte[]>(StringComparer.Ordinal);
        var materialPaths = new List<string>();
        long rawTextureBytes = 0;
        var textureReferences = 0;
        foreach (var pack in packs)
        {
            VerifyMaterialPack(pack);
            using var archive = ZipFile.OpenRead(pack);
            var manifest = ReadJsonEntry(archive, "manifest.json");
            if (ParseMaterialPackProfile(manifest["profile"]?.GetValue<string>() ?? "Full") != MaterialPackProfile.AnalysisCompact ||
                ParseSourceTexturePolicy(manifest["sourceTexturePolicy"]?.GetValue<string>() ?? "Reconstructable") != SourceTexturePolicy.MetadataOnly ||
                manifest["sourceTextures"]!.AsArray().Any(x => x!["included"]!.GetValue<bool>()))
                throw new InvalidDataException("Material bundle accepts only AnalysisCompact MetadataOnly packs.");
            var materialName = manifest["material"]!["name"]!.GetValue<string>();
            var materialPath = "materials/" + materialName + ".json";
            if (materialPayloads.ContainsKey(materialPath)) throw new InvalidDataException("Duplicate material name in bundle: " + materialName);

            var previewBytes = ReadZipEntry(archive, manifest["preview"]!["file"]!.GetValue<string>());
            var previewPath = "previews/" + materialName + ".webp";
            previewPayloads.Add(previewPath, previewBytes);
            manifest["preview"]!["file"] = "../" + previewPath;

            var referencedFiles = new JsonArray
            {
                BundleFileRecord("../" + previewPath, previewBytes)
            };
            var pbr = manifest["pbr"]!.AsObject();
            foreach (var role in BundlePbrRoles)
            {
                if (pbr[role] is not JsonObject entry) continue;
                var bytes = ReadZipEntry(archive, entry["file"]!.GetValue<string>());
                var sha = Convert.ToHexString(SHA256.HashData(bytes));
                if (entry["sha256"]?.GetValue<string>() != sha) throw new InvalidDataException("PBR hash mismatch: " + role);
                var texturePath = "textures/" + sha.ToLowerInvariant() + ".webp";
                texturePayloads.TryAdd(texturePath, bytes);
                rawTextureBytes += bytes.LongLength;
                textureReferences++;
                entry["file"] = "../" + texturePath;
                referencedFiles.Add(BundleFileRecord("../" + texturePath, bytes));
            }
            manifest["files"] = referencedFiles;
            var bytesOut = Encoding.UTF8.GetBytes(manifest.ToJsonString(JsonOptions) + Environment.NewLine);
            materialPayloads.Add(materialPath, bytesOut);
            materialPaths.Add(materialPath);
        }

        var storedTextureBytes = texturePayloads.Values.Sum(x => (long)x.Length);
        var bundle = new JsonObject
        {
            ["schemaVersion"] = 1,
            ["profile"] = "AnalysisCompact",
            ["bundleName"] = bundleName,
            ["materials"] = JsonSerializer.SerializeToNode(materialPaths, JsonOptions),
            ["textureCount"] = texturePayloads.Count,
            ["textureReferences"] = textureReferences,
            ["deduplicatedTextureReferences"] = textureReferences - texturePayloads.Count,
            ["previewCount"] = previewPayloads.Count,
            ["totalRawTextureBytesBeforeDedup"] = rawTextureBytes,
            ["totalStoredTextureBytesAfterDedup"] = storedTextureBytes,
            ["exporterSha256"] = Hash(typeof(Program).Assembly.Location)
        };
        var bundleBytes = Encoding.UTF8.GetBytes(bundle.ToJsonString(JsonOptions) + Environment.NewLine);
        Directory.CreateDirectory(Path.GetDirectoryName(output)!);
        using (var stream = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, false, Encoding.UTF8))
        {
            WritePackEntry(archive, "bundle.json", bundleBytes);
            foreach (var pair in materialPayloads) WritePackEntry(archive, pair.Key, pair.Value);
            foreach (var pair in previewPayloads) WritePackEntry(archive, pair.Key, pair.Value);
            foreach (var pair in texturePayloads) WritePackEntry(archive, pair.Key, pair.Value);
        }
        Console.WriteLine(JsonSerializer.Serialize(new { status = "bundled", materialBundle = output, bundleName,
            materials = materialPayloads.Count, textures = texturePayloads.Count, textureReferences,
            deduplicatedTextureReferences = textureReferences - texturePayloads.Count,
            bytes = new FileInfo(output).Length, sha256 = Hash(output) }));
        return 0;
    }

    internal static int VerifyMaterialBundle(string path)
    {
        var fullPath = Path.GetFullPath(path);
        using var archive = ZipFile.OpenRead(fullPath);
        var names = archive.Entries.Select(x => x.FullName).ToArray();
        if (names.Length == 0 || names.Distinct(StringComparer.Ordinal).Count() != names.Length ||
            names.Distinct(StringComparer.OrdinalIgnoreCase).Count() != names.Length ||
            names.Any(x => x.EndsWith('/') || x.EndsWith(".glb", StringComparison.OrdinalIgnoreCase) ||
                x.EndsWith(".gltf", StringComparison.OrdinalIgnoreCase) || x.EndsWith(".bin", StringComparison.OrdinalIgnoreCase) ||
                x.StartsWith("source/", StringComparison.Ordinal)))
            throw new InvalidDataException("Material bundle has invalid, duplicate, geometry or source-pixel entries.");
        foreach (var name in names) ValidatePackRelativePath(name);
        var bundle = ReadJsonEntry(archive, "bundle.json");
        if (bundle["schemaVersion"]?.GetValue<int>() != 1 || bundle["profile"]?.GetValue<string>() != "AnalysisCompact")
            throw new InvalidDataException("Unsupported material-bundle schema/profile.");
        var materialPaths = bundle["materials"]!.AsArray().Select(x => x!.GetValue<string>()).ToArray();
        if (materialPaths.Length is < 1 or > 128 || materialPaths.Distinct(StringComparer.OrdinalIgnoreCase).Count() != materialPaths.Length)
            throw new InvalidDataException("Invalid material manifest list.");
        var expected = new HashSet<string>(StringComparer.Ordinal) { "bundle.json" };
        var textureReferences = 0;
        var textureHashes = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var materialPath in materialPaths)
        {
            if (!materialPath.StartsWith("materials/", StringComparison.Ordinal) || !materialPath.EndsWith(".json", StringComparison.Ordinal))
                throw new InvalidDataException("Invalid material manifest path.");
            expected.Add(materialPath);
            var manifest = ReadJsonEntry(archive, materialPath);
            foreach (var section in new[] { "material", "renderState", "pbr", "parameters", "textureBindings", "sourceTextures", "reconstruction", "uv", "preview", "provenance", "files" })
                if (manifest[section] == null) throw new InvalidDataException("Missing bundled material section: " + section);
            if (manifest["profile"]?.GetValue<string>() != "AnalysisCompact" ||
                manifest["sourceTexturePolicy"]?.GetValue<string>() != "MetadataOnly" ||
                manifest["sourceTextures"]!.AsArray().Any(x => x!["included"]!.GetValue<bool>()))
                throw new InvalidDataException("Bundled material violates AnalysisCompact policy.");
            ValidateAssets([manifest["material"]!["source"]!.GetValue<string>()]);
            var previewPath = ResolveBundleReference(materialPath, manifest["preview"]!["file"]!.GetValue<string>(), "previews/");
            expected.Add(previewPath);
            var previewBytes = ReadZipEntry(archive, previewPath);
            var ownReferences = new HashSet<string>(StringComparer.Ordinal) { "../" + previewPath };
            var pbr = manifest["pbr"]!.AsObject();
            foreach (var role in BundlePbrRoles)
            {
                if (pbr[role] is not JsonObject entry) continue;
                var texturePath = ResolveBundleReference(materialPath, entry["file"]!.GetValue<string>(), "textures/");
                expected.Add(texturePath);
                ownReferences.Add("../" + texturePath);
                textureReferences++;
                var bytes = ReadZipEntry(archive, texturePath);
                var sha = Convert.ToHexString(SHA256.HashData(bytes));
                if (entry["sha256"]?.GetValue<string>() != sha ||
                    Path.GetFileNameWithoutExtension(texturePath) != sha.ToLowerInvariant())
                    throw new InvalidDataException("Bundled texture content address mismatch.");
                var dimensions = ImageDimensions(bytes);
                if (dimensions.Width > 512 || dimensions.Height > 1024 ||
                    entry["width"]?.GetValue<int>() != dimensions.Width || entry["height"]?.GetValue<int>() != dimensions.Height)
                    throw new InvalidDataException("Bundled AnalysisCompact dimensions mismatch.");
                if (role == "normal") ValidateUnitNormals(bytes);
                if (role == "orm" && (entry["channels"]?["R"]?.GetValue<string>() != "occlusion" ||
                    entry["channels"]?["G"]?.GetValue<string>() != "roughness" || entry["channels"]?["B"]?.GetValue<string>() != "metallic"))
                    throw new InvalidDataException("Bundled ORM channel contract mismatch.");
                textureHashes[texturePath] = sha;
            }
            var inventory = manifest["files"]!.AsArray().Select(x => x!.AsObject()).ToArray();
            if (!inventory.Select(x => x["path"]!.GetValue<string>()).ToHashSet(StringComparer.Ordinal).SetEquals(ownReferences))
                throw new InvalidDataException("Bundled material file inventory mismatch.");
            foreach (var record in inventory)
            {
                var reference = record["path"]!.GetValue<string>();
                var payloadPath = ResolveBundleReference(materialPath, reference,
                    reference.StartsWith("../textures/", StringComparison.Ordinal) ? "textures/" : "previews/");
                var bytes = ReadZipEntry(archive, payloadPath);
                if (record["sha256"]?.GetValue<string>() != Convert.ToHexString(SHA256.HashData(bytes)) ||
                    record["bytes"]?.GetValue<long>() != bytes.LongLength)
                    throw new InvalidDataException("Bundled material inventory hash/size mismatch.");
            }
        }
        if (!expected.SetEquals(names)) throw new InvalidDataException("Material bundle has missing or orphan files.");
        if (textureHashes.Values.Distinct(StringComparer.Ordinal).Count() != textureHashes.Count)
            throw new InvalidDataException("Identical bundle texture payload is stored more than once.");
        var textureCount = textureHashes.Count;
        if (bundle["textureCount"]?.GetValue<int>() != textureCount ||
            bundle["textureReferences"]?.GetValue<int>() != textureReferences ||
            bundle["deduplicatedTextureReferences"]?.GetValue<int>() != textureReferences - textureCount ||
            bundle["previewCount"]?.GetValue<int>() != materialPaths.Length)
            throw new InvalidDataException("Material bundle counts are inconsistent.");
        Console.WriteLine(JsonSerializer.Serialize(new { status = "verified", materialBundle = fullPath,
            bundleName = bundle["bundleName"]!.GetValue<string>(), materials = materialPaths.Length,
            textures = textureCount, textureReferences, deduplicatedTextureReferences = textureReferences - textureCount,
            bytes = new FileInfo(fullPath).Length, sha256 = Hash(fullPath) }));
        return 0;
    }

    static JsonObject BundleFileRecord(string path, byte[] bytes)
    {
        var dimensions = ImageDimensions(bytes);
        return new JsonObject
        {
            ["path"] = path,
            ["sha256"] = Convert.ToHexString(SHA256.HashData(bytes)),
            ["bytes"] = bytes.LongLength,
            ["width"] = dimensions.Width,
            ["height"] = dimensions.Height,
            ["mimeType"] = "image/webp"
        };
    }

    static JsonObject ReadJsonEntry(ZipArchive archive, string name)
    {
        var bytes = ReadZipEntry(archive, name);
        return JsonNode.Parse(bytes)!.AsObject();
    }

    static byte[] ReadZipEntry(ZipArchive archive, string name)
    {
        var entry = archive.GetEntry(name) ?? throw new InvalidDataException("Missing archive entry: " + name);
        using var stream = entry.Open();
        using var memory = new MemoryStream();
        stream.CopyTo(memory);
        return memory.ToArray();
    }

    static string ResolveBundleReference(string materialPath, string reference, string requiredPrefix)
    {
        if (!reference.StartsWith("../", StringComparison.Ordinal)) throw new InvalidDataException("Invalid bundle reference.");
        var resolved = reference[3..];
        ValidatePackRelativePath(resolved);
        if (!resolved.StartsWith(requiredPrefix, StringComparison.Ordinal)) throw new InvalidDataException("Misrouted bundle reference.");
        return resolved;
    }
}

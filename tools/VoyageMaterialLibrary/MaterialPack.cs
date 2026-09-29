using System.Numerics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using CUE4Parse.UE4.Assets.Exports.Material;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse_Conversion.Options;
using SharpGLTF.Materials;
using SharpGLTF.Scenes;
using SixLabors.ImageSharp.Formats.Webp;
using SkiaSharp;

namespace VoyageMaterialLibrary;

internal static partial class Program
{
    internal static MaterialReconstruction ReconstructMaterial(StockProvider provider, string asset, string materialMode,
        IDictionary<string, TextureRecord> textureCache, IDictionary<string, UUnrealMaterial> textureSources)
    {
        var material = provider.LoadPackage(asset).GetExports().OfType<UMaterialInterface>().SingleOrDefault()
            ?? throw new InvalidDataException($"Not one Material/MaterialInstance: {asset}");
        var chain = new List<UUnrealMaterial>();
        for (UUnrealMaterial? current = material; current != null; current = (current as UMaterialInstance)?.Parent)
        {
            if (chain.Count >= 64 || chain.Any(x => x.GetPathName() == current.GetPathName()))
                throw new InvalidDataException($"Cyclic/too-deep material parent chain: {asset}");
            chain.Add(current);
        }
        if (chain[^1] is not UMaterial master) throw new InvalidDataException($"Unresolved master material: {asset}");
        var parameters = new CMaterialParams2();
        material.GetParams(parameters, EMaterialDepth.AllLayers);
        var report = new MaterialRecord
        {
            Source = asset,
            Name = asset.Split('/')[^1],
            Parents = chain.Skip(1).Select(x => x.GetPathName()).ToArray(),
            Parameters = JsonNode.Parse(Newtonsoft.Json.JsonConvert.SerializeObject(parameters))!
        };
        report.Warnings.Add(materialMode == "BakeReconstructed"
            ? "Reconstructed bake from cooked parameters and known recipes; the stripped Unreal expression graph is not executed."
            : "Approximation, not Unreal shader baking. Layer mixing, world/object coordinates, UV math, vertex data, animation and runtime effects are not evaluated.");
        foreach (var pair in parameters.Textures.OrderBy(x => x.Key, StringComparer.Ordinal))
        {
            var path = pair.Value.GetPathName();
            report.Textures[pair.Key] = path;
            textureSources.TryAdd(path, pair.Value);
        }
        TextureRecord Resolve(string path)
        {
            if (!textureCache.TryGetValue(path, out var texture))
                textureCache[path] = texture = Decode(parameters.Textures.Values.First(t => t.GetPathName() == path));
            return texture;
        }
        var builder = MakeMaterial(report, parameters, Resolve, materialMode);
        var blend = master.BlendMode;
        var twoSided = master.TwoSided;
        var cutoff = master.OpacityMaskClipValue;
        foreach (var instance in chain.AsEnumerable().Reverse().OfType<UMaterialInstance>())
        {
            var overrides = instance.GetOrDefault<CUE4Parse.UE4.Assets.Objects.FStructFallback>("BasePropertyOverrides");
            if (overrides == null) continue;
            if (overrides.GetOrDefault<bool>("bOverride_BlendMode")) blend = overrides.GetOrDefault<EBlendMode>("BlendMode");
            if (overrides.GetOrDefault<bool>("bOverride_TwoSided")) twoSided = overrides.GetOrDefault<bool>("TwoSided");
            if (overrides.GetOrDefault<bool>("bOverride_OpacityMaskClipValue")) cutoff = overrides.GetOrDefault<float>("OpacityMaskClipValue");
        }
        builder.WithDoubleSide(twoSided);
        if (blend == EBlendMode.BLEND_Masked) builder.WithAlpha(AlphaMode.MASK, Math.Clamp(cutoff, 0, 1));
        else if (blend == EBlendMode.BLEND_Translucent) builder.WithAlpha(AlphaMode.BLEND);
        if (blend != EBlendMode.BLEND_Opaque)
            report.Warnings.Add($"Unreal {blend}; opacity graph not evaluated. Mask/blend uses base-color alpha only; other blend modes stay opaque.");
        report.RenderState = new { blend = blend.ToString(), twoSided, cutoff, masterShadingModel = master.ShadingModel.ToString() };
        builder.Extras = JsonSerializer.SerializeToNode(report, JsonOptions);
        return new MaterialReconstruction(report, parameters, builder);
    }

    static int ExportMaterialPackStage(StockProvider provider, string[] assets, string output, string materialMode,
        SourceTexturePolicy sourceTexturePolicy, string build, string exeHash, string mappingPath)
    {
        if (assets.Length != 1) throw new ArgumentException("MaterialPackStage exports exactly one material.");
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Material pack stage exists; choose a fresh path.");
        Directory.CreateDirectory(output);
        var pbrDirectory = Path.Combine(output, "pbr");
        var sourceDirectory = Path.Combine(output, "source");
        Directory.CreateDirectory(pbrDirectory);

        var textureCache = new Dictionary<string, TextureRecord>(StringComparer.Ordinal);
        var textureSources = new Dictionary<string, UUnrealMaterial>(StringComparer.Ordinal);
        var reconstruction = ReconstructMaterial(provider, assets[0], materialMode, textureCache, textureSources);
        var report = reconstruction.Report;
        var scene = new SceneBuilder();
        scene.AddRigidMesh(Swatch(reconstruction.Builder, report.Name), Matrix4x4.Identity);
        var model = scene.ToGltf2();
        MatchUsedImages(model, textureCache.Values);
        MatchBakeOutputs(model, [report]);
        var previewSource = Path.Combine(output, "_preview-source.glb");
        model.SaveGLB(previewSource);

        var pbr = new JsonObject
        {
            ["baseColorFactor"] = JsonSerializer.SerializeToNode(report.BaseColorFactor),
            ["metallicFactor"] = report.MetallicFactor,
            ["roughnessFactor"] = report.RoughnessFactor,
            ["emissiveFactor"] = JsonSerializer.SerializeToNode(report.EmissiveFactor),
            ["emissiveStrength"] = report.EmissiveStrength
        };
        AddPbrFile(pbr, textureCache.Values, "baseColor", "baseColor", Path.Combine(pbrDirectory, "basecolor.webp"),
            "pbr/basecolor.webp", true);
        AddPbrFile(pbr, textureCache.Values, "normal", "normal", Path.Combine(pbrDirectory, "normal.png"),
            "pbr/normal.png", false, new JsonObject { ["convention"] = "OpenGL" });
        AddPbrFile(pbr, textureCache.Values, "ORM", "orm", Path.Combine(pbrDirectory, "orm.png"),
            "pbr/orm.png", false, new JsonObject
            {
                ["channels"] = new JsonObject { ["R"] = "occlusion", ["G"] = "roughness", ["B"] = "metallic" }
            });
        AddPbrFile(pbr, textureCache.Values, "emissive", "emissive", Path.Combine(pbrDirectory, "emissive.webp"),
            "pbr/emissive.webp", true);

        var render = JsonSerializer.SerializeToNode(report.RenderState, JsonOptions)!.AsObject();
        var sourceEntries = DescribePackSources(report, reconstruction.Parameters, textureSources, textureCache, sourceDirectory,
            sourceTexturePolicy);
        var skippedEffects = report.SkippedTextures.OrderBy(x => x.Key, StringComparer.Ordinal).Select(entry => new
        {
            texture = entry.Key,
            parameters = report.Textures.Where(x => x.Value == entry.Key).Select(x => x.Key).OrderBy(x => x, StringComparer.Ordinal).ToArray(),
            reason = entry.Value
        }).ToArray();
        var manifest = new JsonObject
        {
            ["schemaVersion"] = 1,
            ["sourceTexturePolicy"] = sourceTexturePolicy.ToString(),
            ["material"] = new JsonObject
            {
                ["name"] = report.Name,
                ["source"] = report.Source,
                ["parents"] = JsonSerializer.SerializeToNode(report.Parents)
            },
            ["renderState"] = new JsonObject
            {
                ["blendMode"] = render["blend"]?.DeepClone(),
                ["shadingModel"] = render["masterShadingModel"]?.DeepClone(),
                ["twoSided"] = render["twoSided"]?.DeepClone(),
                ["opacityMaskClipValue"] = render["blend"]?.GetValue<string>() == "BLEND_Masked" ? render["cutoff"]?.DeepClone() : null
            },
            ["pbr"] = pbr,
            ["parameters"] = ParameterManifest(reconstruction.Parameters),
            ["textureBindings"] = JsonSerializer.SerializeToNode(report.Bindings, JsonOptions),
            ["sourceTextures"] = JsonSerializer.SerializeToNode(sourceEntries, JsonOptions),
            ["uv"] = InferUv(reconstruction.Parameters),
            ["reconstruction"] = new JsonObject
            {
                ["mode"] = materialMode,
                ["bakeOperations"] = JsonSerializer.SerializeToNode(report.BakeOperations, JsonOptions),
                ["warnings"] = JsonSerializer.SerializeToNode(report.Warnings, JsonOptions),
                ["skippedEffects"] = JsonSerializer.SerializeToNode(skippedEffects, JsonOptions)
            },
            ["preview"] = new JsonObject
            {
                ["file"] = "preview.webp",
                ["recipe"] = "voyage.material-sphere/1",
                ["width"] = 768,
                ["height"] = 768,
                ["renderer"] = "Blender EEVEE",
                ["exposure"] = 0
            },
            ["provenance"] = new JsonObject
            {
                ["steamBuildId"] = build,
                ["executableSha256"] = exeHash,
                ["mappingSha256"] = Hash(mappingPath),
                ["conversionCommit"] = "ec6595e46448a817ac21ea9bde01caa48f80a420",
                ["exporterSha256"] = Hash(typeof(Program).Assembly.Location)
            }
        };
        var manifestPath = Path.Combine(output, "manifest.json");
        File.WriteAllText(manifestPath, manifest.ToJsonString(JsonOptions), new System.Text.UTF8Encoding(false));
        var reportPath = Path.Combine(Directory.GetCurrentDirectory(), "export-report.json");
        var result = new
        {
            schema = "voyage.material-pack-stage/1",
            status = textureCache.Values.Any(x => x.Error != null) ? "partial-textures" : "staged",
            materialMode,
            sourceTexturePolicy = sourceTexturePolicy.ToString(),
            material = report.Source,
            stagePath = output,
            manifestPath,
            previewSourceGlb = previewSource,
            pbrMaps = pbr.Where(x => x.Value is JsonObject value && value["file"] != null).Select(x => x.Key).ToArray(),
            sourceTextureCount = sourceEntries.Length,
            includedSourceTextureCount = sourceEntries.Count(x => x.included)
        };
        File.WriteAllText(reportPath, JsonSerializer.Serialize(new { result, report, sourceTextures = sourceEntries }, JsonOptions));
        Console.WriteLine(JsonSerializer.Serialize(new { result.status, result.materialMode, result.sourceTexturePolicy,
            result.material, result.stagePath,
            result.manifestPath, result.previewSourceGlb, result.pbrMaps, result.sourceTextureCount,
            result.includedSourceTextureCount, reportPath }));
        return 0;
    }

    static void AddPbrFile(JsonObject pbr, IEnumerable<TextureRecord> textures, string variantRole, string manifestRole,
        string output, string relative, bool webp, JsonObject? details = null)
    {
        var variant = textures.SelectMany(x => x.Variants).SingleOrDefault(x => x.Role == variantRole);
        if (variant?.Data == null) return;
        var bytes = webp ? EncodeWebpLossless(variant.Data) : variant.Data;
        File.WriteAllBytes(output, bytes);
        var dimensions = ImageDimensions(bytes);
        details ??= new JsonObject();
        details["file"] = relative;
        details["colorSpace"] = manifestRole is "baseColor" or "emissive" ? "sRGB" : "linear";
        details["sha256"] = Convert.ToHexString(SHA256.HashData(bytes));
        details["width"] = dimensions.Width;
        details["height"] = dimensions.Height;
        if (webp) details["encoding"] = "WebP lossless";
        pbr[manifestRole] = details;
    }

    static byte[] EncodeWebpLossless(byte[] png)
    {
        using var image = SixLabors.ImageSharp.Image.Load(png);
        using var stream = new MemoryStream();
        image.Save(stream, new WebpEncoder { FileFormat = WebpFileFormatType.Lossless });
        return stream.ToArray();
    }

    static (int Width, int Height) ImageDimensions(byte[] data)
    {
        using var bitmap = SKBitmap.Decode(data) ?? throw new InvalidDataException("Written material-pack image cannot be decoded.");
        return (bitmap.Width, bitmap.Height);
    }

    static PackSourceTexture[] DescribePackSources(MaterialRecord report, CMaterialParams2 parameters,
        IReadOnlyDictionary<string, UUnrealMaterial> textureSources, IDictionary<string, TextureRecord> textureCache,
        string sourceDirectory, SourceTexturePolicy sourceTexturePolicy)
    {
        var active = report.Bindings.Values.Where(x => x.StartsWith('/')).ToHashSet(StringComparer.Ordinal);
        active.UnionWith(report.BakeOperations.SelectMany(x => x.InputTextures));
        var storedByHash = new Dictionary<string, string>(StringComparer.Ordinal);
        var result = new List<PackSourceTexture>();
        foreach (var pair in textureSources.OrderBy(x => x.Key, StringComparer.Ordinal))
        {
            var roles = report.Textures.Where(x => x.Value == pair.Key).Select(x => x.Key).OrderBy(x => x, StringComparer.Ordinal).ToArray();
            var entry = new PackSourceTexture { role = roles.FirstOrDefault() ?? "Unknown", roles = roles, packagePath = pair.Key };
            entry.affectsSkippedEffect = report.SkippedTextures.ContainsKey(pair.Key);
            if (active.Contains(pair.Key))
                entry.selectionReason = "Visual contribution is represented by the baked PBR outputs; source pixels are not duplicated.";
            else if (pair.Value is not UTexture2D)
                entry.selectionReason = "Referenced asset is not a decodable Texture2D; retained as metadata only.";
            else
            {
                entry.reconstructableCandidate = IsImportantSkippedSource(pair.Key, roles, parameters, out var selectionReason);
                entry.selectionReason = selectionReason;
            }
            TextureRecord? texture = null;
            if (entry.reconstructableCandidate)
            {
                if (!textureCache.TryGetValue(pair.Key, out texture)) textureCache[pair.Key] = texture = Decode(pair.Value);
                entry.decodedSourceSha256 = texture.Sha256;
                entry.width = texture.Width;
                entry.height = texture.Height;
                entry.format = texture.Format;
                entry.colorSpace = texture.Srgb ? "sRGB" : "linear";
            }
            if (sourceTexturePolicy == SourceTexturePolicy.MetadataOnly)
                entry.reason = "Source pixel data omitted by MetadataOnly export policy.";
            else if (!entry.reconstructableCandidate)
                entry.reason = entry.selectionReason;
            else if (texture!.Png == null)
                entry.reason = "Important skipped-effect input was selected, but texture decoding failed: " + texture.Error;
            else
            {
                Directory.CreateDirectory(sourceDirectory);
                var storedBytes = texture.Png;
                if (!texture.IsNormal) storedBytes = EncodeWebpLossless(storedBytes);
                var extension = texture.IsNormal ? ".png" : ".webp";
                var pixelHash = Convert.ToHexString(SHA256.HashData(storedBytes));
                if (!storedByHash.TryGetValue(pixelHash, out var relative))
                {
                    var leaf = Regex.Replace(pair.Key.Split('/')[^1].Split('.')[0], "[^A-Za-z0-9_.-]", "_");
                    relative = "source/" + leaf + "-" + Convert.ToHexString(SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(pair.Key)))[..8] + extension;
                    File.WriteAllBytes(Path.Combine(sourceDirectory, relative["source/".Length..]), storedBytes);
                    storedByHash[pixelHash] = relative;
                }
                entry.included = true;
                entry.file = relative;
                entry.sha256 = pixelHash;
                entry.storageEncoding = texture.IsNormal ? "PNG lossless" : "WebP lossless";
                entry.reason = "Required to reproduce skipped or unreconstructed effect parameters: " +
                    string.Join(", ", roles) + ".";
            }
            result.Add(entry);
        }
        return result.ToArray();
    }

    internal static bool IsImportantSkippedSource(string packagePath, IReadOnlyCollection<string> roles,
        CMaterialParams2 parameters, out string reason)
    {
        var leaf = Normalize(packagePath.Split('/')[^1].Split('.')[0]);
        var allEvidence = roles.Append(packagePath.Split('/')[^1]).Select(Normalize).ToArray();
        var defaults = new[] { "default", "white", "black", "grey", "gray", "flatnormal", "fallback", "checker" };
        if (allEvidence.Any(x => defaults.Any(x.Contains)))
        {
            reason = "Default/fallback texture is not copied; identity is retained as metadata.";
            return false;
        }
        var semanticRoles = roles.Select(Normalize).Where(x => x != leaf).ToArray();
        if (semanticRoles.Length == 0)
        {
            reason = "Only an inherited texture-name binding is available; active layered use is not proven, so pixels are not copied.";
            return false;
        }
        var effects = new[] { "rust", "dirt", "damage", "wear", "height", "pom", "parallax", "world", "object", "grunge", "paint", "mask" };
        var matched = effects.Where(effect => semanticRoles.Any(x => x.Contains(effect, StringComparison.Ordinal))).ToArray();
        if (matched.Length == 0)
        {
            reason = "No supported active PBR binding or recognized skipped layered-effect role; retained as metadata only.";
            return false;
        }
        var switches = parameters.Switches.Select(x => (Name: Normalize(x.Key), x.Value)).ToArray();
        if (matched.Any(effect => switches.Any(x => !x.Value && x.Name.Contains(effect, StringComparison.Ordinal)) &&
                                  !switches.Any(x => x.Value && x.Name.Contains(effect, StringComparison.Ordinal))))
        {
            reason = "A related static switch explicitly disables this skipped effect; source pixels are not copied.";
            return false;
        }
        reason = "Recognized skipped layered-effect input selected for preservation.";
        return true;
    }

    internal static SourceTexturePolicy ParseSourceTexturePolicy(string value)
    {
        return value switch
        {
            "MetadataOnly" => SourceTexturePolicy.MetadataOnly,
            "Reconstructable" => SourceTexturePolicy.Reconstructable,
            _ => throw new ArgumentException("sourceTexturePolicy must be MetadataOnly or Reconstructable.")
        };
    }

    static JsonObject ParameterManifest(CMaterialParams2 parameters)
    {
        var colors = new JsonObject();
        foreach (var pair in parameters.Colors.OrderBy(x => x.Key, StringComparer.Ordinal))
            colors[pair.Key] = JsonSerializer.SerializeToNode(new[] { pair.Value.R, pair.Value.G, pair.Value.B, pair.Value.A });
        var scalars = new JsonObject();
        foreach (var pair in parameters.Scalars.OrderBy(x => x.Key, StringComparer.Ordinal)) scalars[pair.Key] = pair.Value;
        var switches = new JsonObject();
        foreach (var pair in parameters.Switches.OrderBy(x => x.Key, StringComparer.Ordinal)) switches[pair.Key] = pair.Value;
        return new JsonObject { ["colors"] = colors, ["scalars"] = scalars, ["switches"] = switches };
    }

    static JsonObject InferUv(CMaterialParams2 parameters)
    {
        var names = parameters.Textures.Keys.Concat(parameters.Scalars.Keys).Concat(parameters.Switches.Keys).Select(Normalize).ToArray();
        var mode = names.Any(x => x.Contains("triplanar", StringComparison.Ordinal)) ? "triplanar" :
            names.Any(x => x.Contains("worldaligned", StringComparison.Ordinal) || x.Contains("worldspace", StringComparison.Ordinal)) ? "world" :
            names.Any(x => x.Contains("objectspace", StringComparison.Ordinal)) ? "object" : "unknown";
        float? Scalar(params string[] aliases)
        {
            var values = parameters.Scalars.Where(x => aliases.Contains(Normalize(x.Key))).Select(x => x.Value).ToArray();
            return values.Length == 1 && float.IsFinite(values[0]) ? values[0] : null;
        }
        var tiling = Scalar("tiling", "uvtiling", "texturescale");
        var tilingU = Scalar("tilingu", "utiling");
        var tilingV = Scalar("tilingv", "vtiling");
        var offsetU = Scalar("offsetu", "uoffset");
        var offsetV = Scalar("offsetv", "voffset");
        return new JsonObject
        {
            ["mode"] = mode,
            ["tiling"] = tiling.HasValue ? JsonSerializer.SerializeToNode(new[] { tiling.Value, tiling.Value }) :
                tilingU.HasValue && tilingV.HasValue ? JsonSerializer.SerializeToNode(new[] { tilingU.Value, tilingV.Value }) : null,
            ["offset"] = offsetU.HasValue && offsetV.HasValue ? JsonSerializer.SerializeToNode(new[] { offsetU.Value, offsetV.Value }) : null,
            ["rotationDegrees"] = Scalar("rotationdegrees", "uvrotation")
        };
    }

    internal static int FinalizeMaterialPack(string stagePath, string outputPath)
    {
        var stage = Path.GetFullPath(stagePath);
        var output = Path.GetFullPath(outputPath);
        if (!Directory.Exists(stage)) throw new DirectoryNotFoundException("Material-pack stage does not exist.");
        if (File.Exists(output) || !output.EndsWith(".materialpack.zip", StringComparison.OrdinalIgnoreCase))
            throw new IOException("Output must be a fresh .materialpack.zip path.");
        var manifestPath = Path.Combine(stage, "manifest.json");
        var manifest = JsonNode.Parse(File.ReadAllText(manifestPath, Encoding.UTF8))!.AsObject();
        var name = manifest["material"]!["name"]!.GetValue<string>();
        if (Path.GetFileName(output) != name + ".materialpack.zip")
            throw new InvalidDataException("Output filename must match <MaterialName>.materialpack.zip.");
        var payloads = PackReferencedFiles(manifest).OrderBy(x => x, StringComparer.Ordinal).ToDictionary(relative =>
        {
            ValidatePackRelativePath(relative);
            var path = Path.GetFullPath(Path.Combine(stage, relative.Replace('/', Path.DirectorySeparatorChar)));
            var prefix = stage.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
            if (!path.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) || !File.Exists(path))
                throw new InvalidDataException("Missing or unsafe staged file: " + relative);
            return relative;
        }, relative => File.ReadAllBytes(Path.Combine(stage, relative.Replace('/', Path.DirectorySeparatorChar))), StringComparer.Ordinal);
        var inventory = new JsonArray();
        foreach (var pair in payloads)
        {
            var dimensions = ImageDimensions(pair.Value);
            inventory.Add(new JsonObject
            {
                ["path"] = pair.Key,
                ["sha256"] = Convert.ToHexString(SHA256.HashData(pair.Value)),
                ["bytes"] = pair.Value.Length,
                ["width"] = dimensions.Width,
                ["height"] = dimensions.Height,
                ["mimeType"] = pair.Key.EndsWith(".webp", StringComparison.OrdinalIgnoreCase) ? "image/webp" : "image/png"
            });
        }
        manifest["files"] = inventory;
        var manifestBytes = Encoding.UTF8.GetBytes(manifest.ToJsonString(JsonOptions) + Environment.NewLine);
        Directory.CreateDirectory(Path.GetDirectoryName(output)!);
        using (var stream = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, false, Encoding.UTF8))
        {
            WritePackEntry(archive, "manifest.json", manifestBytes);
            foreach (var pair in payloads) WritePackEntry(archive, pair.Key, pair.Value);
        }
        var result = new { status = "packed", materialPack = output, entries = payloads.Count + 1,
            bytes = new FileInfo(output).Length, sha256 = Hash(output) };
        Console.WriteLine(JsonSerializer.Serialize(result));
        return 0;
    }

    internal static int VerifyMaterialPack(string path)
    {
        var fullPath = Path.GetFullPath(path);
        using var stream = File.OpenRead(fullPath);
        using var archive = new ZipArchive(stream, ZipArchiveMode.Read, false, Encoding.UTF8);
        var names = archive.Entries.Select(x => x.FullName).ToArray();
        if (names.Length == 0 || names.Any(x => x.EndsWith('/') || x.EndsWith(".glb", StringComparison.OrdinalIgnoreCase) ||
                x.EndsWith(".gltf", StringComparison.OrdinalIgnoreCase) || x.EndsWith(".bin", StringComparison.OrdinalIgnoreCase)))
            throw new InvalidDataException("Material pack is empty, has a directory entry, or contains model/geometry payload.");
        foreach (var name in names) ValidatePackRelativePath(name);
        if (names.Distinct(StringComparer.Ordinal).Count() != names.Length || names.Distinct(StringComparer.OrdinalIgnoreCase).Count() != names.Length)
            throw new InvalidDataException("Material pack contains duplicate or case-colliding entries.");
        var entries = archive.Entries.ToDictionary(x => x.FullName, StringComparer.Ordinal);
        if (!entries.TryGetValue("manifest.json", out var manifestEntry)) throw new InvalidDataException("Missing manifest.json.");
        JsonObject manifest;
        using (var reader = new StreamReader(manifestEntry.Open(), new UTF8Encoding(false, true)))
            manifest = JsonNode.Parse(reader.ReadToEnd())!.AsObject();
        if (manifest["schemaVersion"]?.GetValue<int>() != 1) throw new InvalidDataException("Unsupported material-pack schema.");
        foreach (var section in new[] { "material", "renderState", "pbr", "parameters", "textureBindings", "sourceTextures", "reconstruction", "uv", "preview", "provenance" })
            if (manifest[section] == null) throw new InvalidDataException("Missing manifest section: " + section);
        var sourceTexturePolicy = ParseSourceTexturePolicy(manifest["sourceTexturePolicy"]?.GetValue<string>() ?? "Reconstructable");
        var material = manifest["material"]!.AsObject();
        var materialName = material["name"]!.GetValue<string>();
        var materialSource = material["source"]!.GetValue<string>();
        ValidateAssets([materialSource]);
        if (Path.GetFileName(fullPath) != materialName + ".materialpack.zip")
            throw new InvalidDataException("Archive filename must match <MaterialName>.materialpack.zip.");
        var preview = manifest["preview"]!.AsObject();
        if (preview["file"]?.GetValue<string>() != "preview.webp" || preview["width"]?.GetValue<int>() != 768 || preview["height"]?.GetValue<int>() != 768)
            throw new InvalidDataException("Invalid fixed preview contract.");
        var expected = PackReferencedFiles(manifest).Append("manifest.json").ToHashSet(StringComparer.Ordinal);
        if (!expected.SetEquals(names)) throw new InvalidDataException("Archive has missing or unreferenced entries.");
        var pbr = manifest["pbr"]!.AsObject();
        if (pbr["baseColorFactor"]?.AsArray().Count != 4 || pbr["metallicFactor"] == null || pbr["roughnessFactor"] == null)
            throw new InvalidDataException("Invalid PBR factors.");
        ValidatePbrEntry(pbr, "baseColor", ".webp", "sRGB");
        ValidatePbrEntry(pbr, "normal", ".png", "linear");
        ValidatePbrEntry(pbr, "orm", ".png", "linear");
        ValidatePbrEntry(pbr, "emissive", ".webp", "sRGB");
        ValidatePbrEntry(pbr, "opacity", ".png", "linear");
        if (pbr["normal"] is JsonObject normal && normal["convention"]?.GetValue<string>() != "OpenGL")
            throw new InvalidDataException("Normal convention must be OpenGL.");
        if (pbr["orm"] is JsonObject orm && (orm["channels"]?["R"]?.GetValue<string>() != "occlusion" ||
                orm["channels"]?["G"]?.GetValue<string>() != "roughness" || orm["channels"]?["B"]?.GetValue<string>() != "metallic"))
            throw new InvalidDataException("Invalid ORM channel contract.");
        foreach (var source in manifest["sourceTextures"]!.AsArray().Select(x => x!.AsObject()))
        {
            if (source["packagePath"] == null || source["reason"] == null || source["roles"]?.AsArray().Count == 0)
                throw new InvalidDataException("Incomplete source texture metadata.");
            var included = source["included"]!.GetValue<bool>();
            var file = source["file"]?.GetValue<string>();
            if (included != (file != null) || file != null && (!file.StartsWith("source/", StringComparison.Ordinal) ||
                    !(file.EndsWith(".png", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".webp", StringComparison.OrdinalIgnoreCase))))
                throw new InvalidDataException("Invalid source texture payload contract.");
        }
        if (sourceTexturePolicy == SourceTexturePolicy.MetadataOnly &&
            manifest["sourceTextures"]!.AsArray().Any(x => x!["included"]!.GetValue<bool>()))
            throw new InvalidDataException("MetadataOnly material pack contains source texture pixels.");
        var inventory = manifest["files"]?.AsArray() ?? throw new InvalidDataException("Missing file inventory.");
        var records = inventory.Select(x => x!.AsObject()).ToDictionary(x => x["path"]!.GetValue<string>(), StringComparer.Ordinal);
        if (!records.Keys.ToHashSet(StringComparer.Ordinal).SetEquals(expected.Where(x => x != "manifest.json")))
            throw new InvalidDataException("File inventory does not match archive payload.");
        if (records.Values.GroupBy(x => x["sha256"]!.GetValue<string>(), StringComparer.Ordinal).Any(x => x.Count() > 1))
            throw new InvalidDataException("Duplicate image payloads are stored more than once.");
        foreach (var role in new[] { "baseColor", "normal", "orm", "emissive", "opacity" })
        {
            if (pbr[role] is not JsonObject entry) continue;
            var file = entry["file"]!.GetValue<string>();
            if (entry["sha256"]?.GetValue<string>() != records[file]["sha256"]?.GetValue<string>() ||
                entry["width"]?.GetValue<int>() != records[file]["width"]?.GetValue<int>() ||
                entry["height"]?.GetValue<int>() != records[file]["height"]?.GetValue<int>())
                throw new InvalidDataException("PBR image provenance mismatch: " + role);
        }
        foreach (var source in manifest["sourceTextures"]!.AsArray().Select(x => x!.AsObject()).Where(x => x["included"]!.GetValue<bool>()))
        {
            var file = source["file"]!.GetValue<string>();
            if (source["sha256"]?.GetValue<string>() != records[file]["sha256"]?.GetValue<string>())
                throw new InvalidDataException("Source image provenance mismatch: " + file);
        }
        foreach (var name in expected.Where(x => x != "manifest.json"))
        {
            using var payloadStream = entries[name].Open();
            using var memory = new MemoryStream();
            payloadStream.CopyTo(memory);
            var bytes = memory.ToArray();
            var record = records[name];
            var dimensions = ImageDimensions(bytes);
            if (record["sha256"]?.GetValue<string>() != Convert.ToHexString(SHA256.HashData(bytes)) ||
                record["bytes"]?.GetValue<long>() != bytes.LongLength || record["width"]?.GetValue<int>() != dimensions.Width ||
                record["height"]?.GetValue<int>() != dimensions.Height)
                throw new InvalidDataException("File inventory mismatch: " + name);
        }
        var pbrMaps = new[] { "baseColor", "normal", "orm", "emissive", "opacity" }.Where(x => pbr[x] != null).ToArray();
        var sources = manifest["sourceTextures"]!.AsArray();
        var result = new { status = "verified", materialPack = fullPath, material = materialSource,
            materialMode = manifest["reconstruction"]!["mode"]!.GetValue<string>(), pbrMaps,
            sourceTexturePolicy = sourceTexturePolicy.ToString(),
            sourceTextures = sources.Count, includedSourceTextures = sources.Count(x => x!["included"]!.GetValue<bool>()),
            entries = names.Length, bytes = new FileInfo(fullPath).Length, sha256 = Hash(fullPath) };
        Console.WriteLine(JsonSerializer.Serialize(result));
        return 0;
    }

    static HashSet<string> PackReferencedFiles(JsonObject manifest)
    {
        var files = new HashSet<string>(StringComparer.Ordinal) { manifest["preview"]!["file"]!.GetValue<string>() };
        var pbr = manifest["pbr"]!.AsObject();
        foreach (var role in new[] { "baseColor", "normal", "orm", "emissive", "opacity" })
            if (pbr[role] is JsonObject entry) files.Add(entry["file"]!.GetValue<string>());
        foreach (var source in manifest["sourceTextures"]!.AsArray().Select(x => x!.AsObject()).Where(x => x["included"]!.GetValue<bool>()))
            files.Add(source["file"]!.GetValue<string>());
        return files;
    }

    static void ValidatePackRelativePath(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || path.Contains('\\') || path.StartsWith('/') || path.Contains(':') ||
            path.Split('/').Any(x => x is "" or "." or ".."))
            throw new InvalidDataException("Unsafe material-pack entry: " + path);
    }

    static void WritePackEntry(ZipArchive archive, string name, byte[] bytes)
    {
        var entry = archive.CreateEntry(name, CompressionLevel.NoCompression);
        entry.LastWriteTime = new DateTimeOffset(1980, 1, 1, 0, 0, 0, TimeSpan.Zero);
        using var stream = entry.Open();
        stream.Write(bytes);
    }

    static void ValidatePbrEntry(JsonObject pbr, string role, string extension, string colorSpace)
    {
        if (pbr[role] is not JsonObject entry) return;
        var file = entry["file"]?.GetValue<string>() ?? "";
        if (!file.StartsWith("pbr/", StringComparison.Ordinal) || !file.EndsWith(extension, StringComparison.OrdinalIgnoreCase) ||
            entry["colorSpace"]?.GetValue<string>() != colorSpace)
            throw new InvalidDataException("Invalid PBR file contract: " + role);
    }
}

internal sealed record MaterialReconstruction(MaterialRecord Report, CMaterialParams2 Parameters, MaterialBuilder Builder);

internal enum SourceTexturePolicy
{
    MetadataOnly,
    Reconstructable
}

internal sealed class PackSourceTexture
{
    public string role { get; set; } = "";
    public string[] roles { get; set; } = [];
    public string packagePath { get; set; } = "";
    public bool included { get; set; }
    public string? file { get; set; }
    public string reason { get; set; } = "";
    public string? sha256 { get; set; }
    public string? decodedSourceSha256 { get; set; }
    public int width { get; set; }
    public int height { get; set; }
    public string? format { get; set; }
    public string? colorSpace { get; set; }
    public string? storageEncoding { get; set; }
    public bool affectsSkippedEffect { get; set; }
    public bool reconstructableCandidate { get; set; }
    public string selectionReason { get; set; } = "";
}

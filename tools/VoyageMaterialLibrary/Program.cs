using System.Numerics;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using CUE4Parse.FileProvider;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Assets.Exports.Material;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse.UE4.Versions;
using CUE4Parse_Conversion.Options;
using CUE4Parse_Conversion.Textures;
using SharpGLTF.Geometry;
using SharpGLTF.Geometry.VertexTypes;
using SharpGLTF.Materials;
using SharpGLTF.Memory;
using SharpGLTF.Schema2;
using SharpGLTF.Scenes;
using SkiaSharp;

namespace VoyageMaterialLibrary;

internal static class Program
{
    internal static string Hash(string path) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)));
    static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

    static int Main(string[] args)
    {
        try
        {
            if (args.Length == 2 && args[0] == "--self-test") return SelfTests.Run(args[1]);
            if (args.Length != 1) throw new ArgumentException("Usage: VoyageMaterialLibrary <request.json>");
            var request = JsonNode.Parse(File.ReadAllText(args[0]))!;
            string Str(string key) => request[key]!.GetValue<string>();
            var output = Path.GetFullPath(Str("output"));
            if (File.Exists(output)) throw new IOException("Output exists; choose a fresh GLB path.");
            var assets = request["materials"]!.AsArray().Select(x => x!.GetValue<string>()).ToArray();
            ValidateAssets(assets);
            var fingerprint = JsonNode.Parse(File.ReadAllText(Str("fingerprintPath")))!;
            var mapping = JsonNode.Parse(File.ReadAllText(Str("mappingManifestPath")))!;
            var gameRoot = fingerprint["gameRoot"]!.GetValue<string>();
            var exeHash = fingerprint["executable"]!["sha256"]!.GetValue<string>();
            var build = fingerprint["steam"]!["buildId"]!.GetValue<string>();
            if (mapping["engineVersion"]!.GetValue<string>() != "5.8" ||
                mapping["steamBuildId"]!.GetValue<string>() != build ||
                mapping["executableSha256"]!.GetValue<string>() != exeHash ||
                mapping["mappingSha256"]!.GetValue<string>() != Hash(Str("mappingPath")) ||
                Hash(Path.Combine(gameRoot, "Voyage/Binaries/Win64", fingerprint["executable"]!["name"]!.GetValue<string>())) != exeHash)
                throw new InvalidDataException("Game/mapping provenance mismatch.");

            // Upstream native helper extraction is confined to this fresh evidence directory.
            Directory.SetCurrentDirectory(Path.GetDirectoryName(Path.GetFullPath(args[0]))!);
            TextureDecoder.UseAssetRipperTextureDecoder = true;
            using var provider = new StockProvider(new DirectoryInfo(Path.Combine(gameRoot, "Voyage/Content/Paks")));
            provider.MappingsContainer = new FileUsmapTypeMappingsProvider(Str("mappingPath"));
            provider.Initialize(); provider.Mount(); provider.PostMount(); provider.LoadVirtualPaths();
            var textureCache = new Dictionary<string, TextureRecord>(StringComparer.Ordinal);
            var reports = new List<MaterialRecord>();
            var scene = new SceneBuilder();
            foreach (var asset in assets)
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
                if (chain[^1] is not UMaterial) throw new InvalidDataException($"Unresolved master material: {asset}");
                var parameters = new CMaterialParams2();
                material.GetParams(parameters, EMaterialDepth.AllLayers);
                var report = new MaterialRecord { Source = asset, Name = asset.Split('/')[^1],
                    Parents = chain.Skip(1).Select(x => x.GetPathName()).ToArray(),
                    Parameters = JsonNode.Parse(Newtonsoft.Json.JsonConvert.SerializeObject(parameters))! };
                report.Warnings.Add("Approximation, not Unreal shader baking. Layer mixing, world/object coordinates, UV math, vertex data, animation and runtime effects are not evaluated.");
                foreach (var pair in parameters.Textures.OrderBy(x => x.Key, StringComparer.Ordinal))
                {
                    var path = pair.Value.GetPathName();
                    report.Textures[pair.Key] = path;
                }
                TextureRecord Resolve(string path)
                {
                    if (!textureCache.TryGetValue(path, out var texture))
                        textureCache[path] = texture = Decode(parameters.Textures.Values.First(t => t.GetPathName() == path));
                    return texture;
                }
                var builder = MakeMaterial(report, parameters, Resolve);
                var master = (UMaterial)chain[^1];
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
                if (blend == EBlendMode.BLEND_Masked) builder.WithAlpha(SharpGLTF.Materials.AlphaMode.MASK, Math.Clamp(cutoff, 0, 1));
                else if (blend == EBlendMode.BLEND_Translucent) builder.WithAlpha(SharpGLTF.Materials.AlphaMode.BLEND);
                if (blend != EBlendMode.BLEND_Opaque) report.Warnings.Add($"Unreal {blend}; opacity graph not evaluated. Mask/blend uses base-color alpha only; other blend modes stay opaque.");
                report.RenderState = new { blend = blend.ToString(), twoSided, cutoff, masterShadingModel = master.ShadingModel.ToString() };
                builder.Extras = JsonSerializer.SerializeToNode(report, JsonOptions);
                var mesh = Swatch(builder, $"{reports.Count:D3}_{report.Name}");
                scene.AddRigidMesh(mesh, Matrix4x4.CreateTranslation((reports.Count % 4) * 1.4f, -(reports.Count / 4) * 1.4f, 0));
                reports.Add(report);
            }
            var model = scene.ToGltf2();
            MatchUsedImages(model, textureCache.Values);
            var evidence = new { schemaVersion = 2, status = textureCache.Values.Any(t => t.Error != null) ? "partial-textures" : "approximate",
                steamBuildId = build, executableSha256 = exeHash, mappingSha256 = Hash(Str("mappingPath")),
                conversionCommit = "ec6595e46448a817ac21ea9bde01caa48f80a420",
                exporterSha256 = Hash(typeof(Program).Assembly.Location),
                parserSha256 = Hash(typeof(DefaultFileProvider).Assembly.Location),
                stockContainers = fingerprint["containers"]!.AsArray().Where(x => StockProvider.IsStock(x!["name"]!.GetValue<string>())).Select(x => x!.DeepClone()).ToArray(),
                materials = reports, textures = textureCache.Values,
                note = "One UV sample panel per material; glTF PBR is a named-parameter approximation. Only used texture variants are embedded. Unknown, ambiguous and disabled dependencies are listed, not decoded or archived." };
            model.Extras = JsonSerializer.SerializeToNode(evidence, JsonOptions);
            var pending = Path.Combine(Directory.GetCurrentDirectory(), "pending.glb");
            model.SaveGLB(pending);
            var readback = ModelRoot.Load(pending);
            if (readback.LogicalMaterials.Count != reports.Count || readback.LogicalImages.Count != model.LogicalImages.Count)
                throw new InvalidDataException("GLB material/image readback mismatch.");
            Directory.CreateDirectory(Path.GetDirectoryName(output)!);
            File.Move(pending, output, false);
            var reportPath = Path.Combine(Directory.GetCurrentDirectory(), "export-report.json");
            File.WriteAllText(reportPath, JsonSerializer.Serialize(evidence, JsonOptions));
            Console.WriteLine(JsonSerializer.Serialize(new { status = evidence.status, glbPath = output, sha256 = Hash(output),
                materialCount = reports.Count, imageCount = readback.LogicalImages.Count,
                pbrTexturedMaterials = reports.Count(r => r.Bindings.ContainsKey("baseColor") || r.Bindings.ContainsKey("normal") || r.Bindings.ContainsKey("ORM") || r.Bindings.ContainsKey("emissive")),
                failedTextures = textureCache.Values.Count(t => t.Error != null), reportPath }));
            return 0;
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); return 1; }
    }

    internal static void ValidateAssets(string[] assets)
    {
        if (assets.Length == 0 || assets.Length > 128 || assets.Distinct(StringComparer.OrdinalIgnoreCase).Count() != assets.Length ||
            assets.Any(x => !Regex.IsMatch(x, @"^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$") || x.Trim() != x))
            throw new ArgumentException("Use 1..128 unique exact /Game/... (or plugin mount) material package paths, no object suffix, wildcard or traversal.");
    }

    static TextureRecord Decode(UUnrealMaterial source)
    {
        var record = new TextureRecord { Source = source.GetPathName() };
        try
        {
            if (source is not UTexture2D texture) throw new NotSupportedException("Only Texture2D can become a glTF PNG.");
            record.Format = texture.Format.ToString(); record.Srgb = texture.SRGB;
            record.IsNormal = texture.IsNormalMap;
            var pixels = texture.Decode() ?? throw new InvalidDataException("No decodable resident mip.");
            record.Width = pixels.Width; record.Height = pixels.Height;
            record.Png = pixels.Encode(ETextureFormat.Png, false, out _);
            record.Sha256 = Convert.ToHexString(SHA256.HashData(record.Png));
            if (PixelFormatUtils.IsHDR(pixels.PixelFormat)) record.Warning = "HDR quantized to PNG for glTF; original radiance range is not preserved.";
        }
        catch (Exception ex) { record.Error = ex.GetType().Name + ": " + ex.Message; }
        return record;
    }

    static string Normalize(string value) => Regex.Replace(value.ToLowerInvariant(), "[^a-z0-9]", "");

    internal static MaterialBuilder MakeMaterial(MaterialRecord report, CMaterialParams2 parameters, Func<string, TextureRecord> resolve)
    {
        var builder = new MaterialBuilder(report.Name).WithMetallicRoughnessShader();
        TextureRecord? Select(string role, params string[] aliases)
        {
            var candidates = report.Textures.Where(p => aliases.Contains(Normalize(p.Key)))
                .Select(p => p.Value).Distinct().ToArray();
            if (candidates.Length == 0)
                candidates = report.Textures.Values.Distinct().Where(path => aliases.Any(a =>
                    Normalize(path.Split('/')[^1].Split('.')[0]).EndsWith(a, StringComparison.Ordinal))).ToArray();
            if (candidates.Length == 1)
            {
                var texture = resolve(candidates[0]);
                if (texture.Png == null)
                {
                    report.SkippedTextures[candidates[0]] = "Selected texture failed decoding: " + texture.Error;
                    report.Warnings.Add($"Unavailable {role}: {candidates[0]}");
                    return null;
                }
                report.Bindings[role] = candidates[0];
                return texture;
            }
            if (candidates.Length > 1) report.Warnings.Add($"Ambiguous {role}; left unbound: {string.Join(", ", candidates)}");
            return null;
        }
        float Scalar(string key, float fallback)
        {
            var values = parameters.Scalars.Where(x => Normalize(x.Key) == key).ToArray();
            if (values.Length != 1 || !float.IsFinite(values[0].Value)) return fallback;
            report.Bindings[key + "Factor"] = values[0].Key + " (named scalar; shader operation not evaluated)";
            return Math.Clamp(values[0].Value, 0, 1);
        }
        var baseColor = Select("baseColor", "basecolor", "basecolortexture", "albedo", "diffuse", "diffusetexture");
        var normal = Select("normal", "normal", "normalmap", "normaltexture");
        var orm = Select("ORM", "orm", "occlusionroughnessmetallic", "occlusionroughnessmetallictexture");
        var strength = parameters.Scalars.Where(x => new[] { "emissivestrength", "emissivestrenght", "emissiveintensity" }.Contains(Normalize(x.Key))).ToArray();
        var emissionValue = strength.Length == 1 && float.IsFinite(strength[0].Value) ? Math.Max(0, strength[0].Value) : 1;
        var emissive = emissionValue > 0 ? Select("emissive", "emissive", "emissivetexture", "emissivecolor") : null;
        if (emissionValue == 0) report.Warnings.Add("Emission is disabled by named scalar; its texture is neither decoded nor embedded.");
        builder.WithBaseColor(Vector4.One).WithMetallicRoughness(Scalar("metallic", orm != null ? 1 : 0), Scalar("roughness", orm != null ? 1 : .8f));
        if (baseColor != null) builder.WithBaseColor(UsedImage(baseColor, "baseColor", ColorPng(baseColor), baseColor.Srgb ? "none" : "linear-to-sRGB"));
        if (normal != null)
        {
            builder.WithNormal(UsedImage(normal, "normal", FlipNormalGreen(normal.Png!), "invert-green"));
            report.Warnings.Add("Normal Y inverted from Unreal/DirectX to glTF/OpenGL; only the converted image is embedded.");
        }
        if (orm != null)
        {
            var image = UsedImage(orm, "ORM", orm.Png!, "none");
            builder.WithMetallicRoughness(image).WithOcclusion(image);
            report.Warnings.Add("ORM inferred by explicit name: R=occlusion, G=roughness, B=metallic; shader wiring is not proven.");
        }
        if (emissive != null)
        {
            builder.WithEmissive(UsedImage(emissive, "emissive", ColorPng(emissive), emissive.Srgb ? "none" : "linear-to-sRGB"),
                new Vector3(Math.Min(emissionValue, 1)), Math.Max(emissionValue, 1));
            if (strength.Length == 1) report.Bindings["emissiveStrength"] = strength[0].Key + " (named scalar)";
        }
        var color = parameters.Colors.Where(x => new[] { "basecolor", "basecolour", "colortint" }.Contains(Normalize(x.Key))).ToArray();
        var disableTint = parameters.Switches.Any(p => Normalize(p.Key) == "usecolortint" && !p.Value);
        if (color.Length == 1 && !disableTint)
        {
            var c = color[0].Value;
            builder.WithBaseColor(Vector4.Clamp(new Vector4(c.R, c.G, c.B, 1), Vector4.Zero, Vector4.One));
            report.Bindings["baseColorFactor"] = color[0].Key + " (linear named tint; shader switch not evaluated)";
        }
        foreach (var source in report.Textures.Values.Distinct().Where(p => !report.Bindings.ContainsValue(p)))
            report.SkippedTextures.TryAdd(source, "No supported unambiguous active PBR binding; image omitted.");
        report.Warnings.Add("PBR roles are inferred from unambiguous parameter/texture names. Unused resources are listed only; no image payload is kept for them.");
        return builder;
    }

    static ImageBuilder UsedImage(TextureRecord source, string role, byte[] png, string transform)
    {
        var hash = Convert.ToHexString(SHA256.HashData(png));
        if (!source.Variants.Any(v => v.Role == role && v.Sha256 == hash))
            source.Variants.Add(new TextureVariant { Role = role, Transform = transform, Sha256 = hash });
        return ImageBuilder.From(new MemoryImage(png));
    }

    internal static void MatchUsedImages(ModelRoot model, IEnumerable<TextureRecord> textures)
    {
        var images = model.LogicalImages.ToDictionary(i => i.LogicalIndex,
            i => Convert.ToHexString(SHA256.HashData(i.Content.Content.ToArray())));
        var described = new HashSet<int>();
        foreach (var texture in textures)
            foreach (var variant in texture.Variants)
            {
                var index = images.Single(i => i.Value == variant.Sha256).Key;
                variant.ImageIndex = index;
                described.Add(index);
                model.LogicalImages[index].Name ??= texture.Source.Split('/')[^1] + "_" + variant.Role;
            }
        if (described.Count != model.LogicalImages.Count)
            throw new InvalidDataException("An embedded image has no used-variant provenance.");
    }

    internal static byte[] FlipNormalGreen(byte[] png) => Transform(png, c => new SKColor(c.Red, (byte)(255 - c.Green), c.Blue, c.Alpha));
    static byte[] ColorPng(TextureRecord record) => record.Srgb ? record.Png! : Transform(record.Png!, c =>
        new SKColor(LinearToSrgb(c.Red), LinearToSrgb(c.Green), LinearToSrgb(c.Blue), c.Alpha));
    static byte LinearToSrgb(byte b)
    {
        var v = b / 255f;
        return (byte)Math.Clamp((int)MathF.Round(255 * (v <= .0031308f ? 12.92f * v : 1.055f * MathF.Pow(v, 1 / 2.4f) - .055f)), 0, 255);
    }
    static byte[] Transform(byte[] png, Func<SKColor, SKColor> transform)
    {
        using var bitmap = SKBitmap.Decode(png) ?? throw new InvalidDataException("PNG decode failed.");
        var colors = bitmap.Pixels;
        for (var i = 0; i < colors.Length; i++) colors[i] = transform(colors[i]);
        bitmap.Pixels = colors;
        using var data = bitmap.Encode(SKEncodedImageFormat.Png, 100);
        return data.ToArray();
    }
    internal static MeshBuilder<VertexPositionNormal, VertexTexture1, VertexEmpty> Swatch(MaterialBuilder material, string name)
    {
        var mesh = new MeshBuilder<VertexPositionNormal, VertexTexture1, VertexEmpty>(name);
        var primitive = mesh.UsePrimitive(material);
        VertexBuilder<VertexPositionNormal, VertexTexture1, VertexEmpty> V(float x, float y, float u, float v)
            => new(new VertexPositionNormal(new Vector3(x, y, 0), Vector3.UnitZ), new VertexTexture1(new Vector2(u, v)));
        primitive.AddTriangle(V(-.5f, -.5f, 0, 1), V(.5f, -.5f, 1, 1), V(.5f, .5f, 1, 0));
        primitive.AddTriangle(V(-.5f, -.5f, 0, 1), V(.5f, .5f, 1, 0), V(-.5f, .5f, 0, 0));
        return mesh;
    }
}

internal sealed class MaterialRecord
{
    public string Source { get; set; } = "";
    public string Name { get; set; } = "";
    public string[] Parents { get; set; } = [];
    public JsonNode Parameters { get; set; } = new JsonObject();
    public object? RenderState { get; set; }
    public Dictionary<string, string> Textures { get; } = new();
    public Dictionary<string, string> Bindings { get; } = new();
    public Dictionary<string, string> SkippedTextures { get; } = new();
    public List<string> Warnings { get; } = [];
}
internal sealed class TextureRecord
{
    public string Source { get; set; } = "";
    public string? Format { get; set; }
    public bool Srgb { get; set; }
    public bool IsNormal { get; set; }
    public int Width { get; set; }
    public int Height { get; set; }
    public List<TextureVariant> Variants { get; } = [];
    public string? Sha256 { get; set; }
    public string? Error { get; set; }
    public string? Warning { get; set; }
    [System.Text.Json.Serialization.JsonIgnore] public byte[]? Png { get; set; }
}
internal sealed class TextureVariant
{
    public string Role { get; set; } = "";
    public string Transform { get; set; } = "";
    public string Sha256 { get; set; } = "";
    public int ImageIndex { get; set; }
}
internal sealed class StockProvider(DirectoryInfo directory) : DefaultFileProvider(directory,
    SearchOption.TopDirectoryOnly, new VersionContainer(EGame.GAME_UE5_8), StringComparer.OrdinalIgnoreCase)
{
    internal static bool IsStock(string name) => name.Equals("global.utoc", StringComparison.OrdinalIgnoreCase) ||
        Regex.IsMatch(name, @"^pakchunk\d+(?:optional)?-Windows\.utoc$", RegexOptions.IgnoreCase);
    public override void Initialize()
    {
        foreach (var file in directory.EnumerateFiles("*.utoc").Where(f => IsStock(f.Name))) RegisterVfs(file);
    }
}

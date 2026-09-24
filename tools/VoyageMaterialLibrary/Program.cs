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
            var materialMode = Str("materialMode");
            if (materialMode is not ("PbrApproximation" or "BakeReconstructed"))
                throw new ArgumentException("materialMode must be PbrApproximation or BakeReconstructed.");
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
            var textureSources = new Dictionary<string, UUnrealMaterial>(StringComparer.Ordinal);
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
            var sourceArtifacts = EmbedSourceArtifacts(model, materialMode, textureSources, textureCache, reports);
            var materialPipeline = new
            {
                schema = "voyage.material-pipeline/1",
                requestedMode = materialMode,
                fidelity = materialMode == "BakeReconstructed" ? "reconstructed" : "approximate-pbr",
                bakeOperations = reports.SelectMany(x => x.BakeOperations),
                generatedImages = textureCache.Values.SelectMany(x => x.Variants.Select(v => new
                {
                    source = x.Source,
                    v.Role,
                    v.Transform,
                    v.Sha256,
                    v.ImageIndex
                })),
                sourceArtifacts,
                unresolvedLayers = reports.SelectMany(x => x.SkippedTextures.Select(layer => new
                {
                    material = x.Source,
                    texture = layer.Key,
                    parameters = x.Textures.Where(p => p.Value == layer.Key).Select(p => p.Key),
                    reason = layer.Value
                }))
            };
            var evidence = new { schemaVersion = 3, status = textureCache.Values.Any(t => t.Error != null) ? "partial-textures" :
                    materialMode == "BakeReconstructed" ? "reconstructed-bake" : "approximate",
                materialMode,
                steamBuildId = build, executableSha256 = exeHash, mappingSha256 = Hash(Str("mappingPath")),
                conversionCommit = "ec6595e46448a817ac21ea9bde01caa48f80a420",
                exporterSha256 = Hash(typeof(Program).Assembly.Location),
                parserSha256 = Hash(typeof(DefaultFileProvider).Assembly.Location),
                stockContainers = fingerprint["containers"]!.AsArray().Where(x => StockProvider.IsStock(x!["name"]!.GetValue<string>())).Select(x => x!.DeepClone()).ToArray(),
                materials = reports, textures = textureCache.Values, materialPipeline,
                note = materialMode == "BakeReconstructed"
                    ? "One UV sample panel per material; known cooked-parameter recipes are reconstructed. All decodable Texture2D inputs are embedded as machine-indexed source artifacts."
                    : "One UV sample panel per material; glTF PBR is a named-parameter approximation. Only used texture variants are embedded." };
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
                materialMode,
                materialCount = reports.Count, imageCount = readback.LogicalImages.Count,
                pbrTexturedMaterials = reports.Count(r => r.Bindings.ContainsKey("baseColor") || r.Bindings.ContainsKey("normal") || r.Bindings.ContainsKey("ORM") || r.Bindings.ContainsKey("emissive")),
                failedTextures = textureCache.Values.Count(t => t.Error != null), sourceArtifactCount = sourceArtifacts.Count, reportPath }));
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

    internal static MaterialBuilder MakeMaterial(MaterialRecord report, CMaterialParams2 parameters, Func<string, TextureRecord> resolve,
        string materialMode = "PbrApproximation")
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
        var baseColor = Select("baseColor", "basecolor", "basecolortexture", "albedo", "diffuse", "diffusetexture", "pmdiffuse");
        var normal = Select("normal", "normal", "normalmap", "normaltexture", "pmnormals");
        var orm = Select("ORM", "orm", "occlusionroughnessmetallic", "occlusionroughnessmetallictexture");
        var strength = parameters.Scalars.Where(x => new[] { "emissivestrength", "emissivestrenght", "emissiveintensity" }.Contains(Normalize(x.Key))).ToArray();
        var emissionValue = strength.Length == 1 && float.IsFinite(strength[0].Value) ? Math.Max(0, strength[0].Value) : 1;
        var emissive = emissionValue > 0 ? Select("emissive", "emissive", "emissivetexture", "emissivecolor") : null;
        if (emissionValue == 0) report.Warnings.Add("Emission is disabled by named scalar; its texture is neither decoded nor embedded.");
        builder.WithBaseColor(Vector4.One).WithMetallicRoughness(Scalar("metallic", orm != null ? 1 : 0), Scalar("roughness", orm != null ? 1 : .8f));
        if (baseColor != null)
        {
            var baseColorPng = ColorPng(baseColor);
            var baseColorTransform = baseColor.Srgb ? "none" : "linear-to-sRGB";
            var maskPaths = report.Textures.Where(x => Normalize(x.Key) == "colormask")
                .Select(x => x.Value).Distinct(StringComparer.Ordinal).ToArray();
            var maskChannels = new List<(string Name, int Component, Vector3 Color)>();
            foreach (var channel in new[] { (Name: "Red", Component: 0), (Name: "Green", Component: 1), (Name: "Blue", Component: 2) })
            {
                var enabled = parameters.Switches.Any(x => Normalize(x.Key) == Normalize(channel.Name + " Mask") && x.Value);
                var colors = parameters.Colors.Where(x => Normalize(x.Key) == Normalize("MaskedColor " + channel.Name)).ToArray();
                if (enabled && colors.Length == 1)
                {
                    var tint = colors[0].Value;
                    maskChannels.Add((channel.Name, channel.Component,
                        Vector3.Clamp(new Vector3(tint.R, tint.G, tint.B), Vector3.Zero, Vector3.One)));
                }
            }
            if (materialMode == "BakeReconstructed" && maskPaths.Length == 1 && maskChannels.Count > 0)
            {
                var mask = resolve(maskPaths[0]);
                if (mask.Png != null && mask.Width == baseColor.Width && mask.Height == baseColor.Height)
                {
                    baseColorPng = BakeMaskedBaseColor(baseColor, mask, maskChannels);
                    baseColorTransform = "masked-color-bake:" + string.Join(",", maskChannels.Select(x =>
                        $"{x.Name}={LinearColorHex(x.Color)}"));
                    report.Bindings["colorMask"] = maskPaths[0];
                    report.BakeOperations.Add(new BakeOperationRecord
                    {
                        Id = report.Name + ":baseColor:masked-color",
                        Material = report.Source,
                        OutputRole = "baseColor",
                        Algorithm = "baseColor * lerp(white, maskedColor, maskChannel)",
                        InputTextures = [baseColor.Source, mask.Source],
                        Parameters = maskChannels.ToDictionary(x => x.Name + "MaskColor", x => LinearColorHex(x.Color), StringComparer.Ordinal),
                        OutputSha256 = Convert.ToHexString(SHA256.HashData(baseColorPng)),
                        Fidelity = "reconstructed"
                    });
                    report.Warnings.Add("Masked-color bake assumes BaseColor * lerp(white, MaskedColor, mask channel); the cooked Unreal shader graph was not evaluated.");
                }
                else report.SkippedTextures[maskPaths[0]] = mask.Png == null
                    ? "ColorMask selected for baking but decoding failed: " + mask.Error
                    : $"ColorMask dimensions {mask.Width}x{mask.Height} do not match Base Color {baseColor.Width}x{baseColor.Height}.";
            }
            else if (materialMode == "BakeReconstructed" && maskPaths.Length > 1)
                report.Warnings.Add($"Ambiguous ColorMask; masked-color bake skipped: {string.Join(", ", maskPaths)}");
            builder.WithBaseColor(UsedImage(baseColor, "baseColor", baseColorPng, baseColorTransform));
        }
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
            report.SkippedTextures.TryAdd(source, materialMode == "BakeReconstructed"
                ? "Not composited into the PBR result; raw source is retained in materialPipeline.sourceArtifacts when decodable."
                : "No supported unambiguous active PBR binding; image omitted.");
        report.Warnings.Add(materialMode == "BakeReconstructed"
            ? "PBR roles are inferred from names. All decodable Texture2D inputs are additionally retained as machine-indexed source artifacts."
            : "PBR roles are inferred from unambiguous parameter/texture names. Unused resources are listed only; no image payload is kept for them.");
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

    static List<TextureArtifactRecord> EmbedSourceArtifacts(ModelRoot model, string materialMode,
        IReadOnlyDictionary<string, UUnrealMaterial> textureSources, IDictionary<string, TextureRecord> textureCache,
        IReadOnlyCollection<MaterialRecord> reports)
    {
        var result = new List<TextureArtifactRecord>();
        if (materialMode != "BakeReconstructed") return result;
        foreach (var pair in textureSources.OrderBy(x => x.Key, StringComparer.Ordinal))
        {
            if (!textureCache.TryGetValue(pair.Key, out var texture)) textureCache[pair.Key] = texture = Decode(pair.Value);
            var artifact = new TextureArtifactRecord
            {
                Source = pair.Key,
                Sha256 = texture.Sha256,
                Width = texture.Width,
                Height = texture.Height,
                Format = texture.Format,
                Srgb = texture.Srgb,
                IsNormal = texture.IsNormal,
                Error = texture.Error,
                Consumers = reports.SelectMany(material => material.Textures.Where(x => x.Value == pair.Key)
                    .Select(x => new TextureConsumerRecord { Material = material.Source, Parameter = x.Key })).ToArray()
            };
            if (texture.Png != null)
            {
                var image = model.UseImage(new MemoryImage(texture.Png));
                image.Name ??= pair.Key.Split('/')[^1].Split('.')[0] + "_source";
                artifact.ImageIndex = image.LogicalIndex;
                artifact.Disposition = "embedded-source";
            }
            else artifact.Disposition = "decode-failed";
            result.Add(artifact);
        }
        foreach (var operation in reports.SelectMany(x => x.BakeOperations))
        {
            var image = model.LogicalImages.Single(x => Convert.ToHexString(SHA256.HashData(x.Content.Content.ToArray())) == operation.OutputSha256);
            operation.OutputImageIndex = image.LogicalIndex;
        }
        return result;
    }

    internal static byte[] FlipNormalGreen(byte[] png) => Transform(png, c => new SKColor(c.Red, (byte)(255 - c.Green), c.Blue, c.Alpha));
    static byte[] ColorPng(TextureRecord record) => record.Srgb ? record.Png! : Transform(record.Png!, c =>
        new SKColor(LinearToSrgb(c.Red), LinearToSrgb(c.Green), LinearToSrgb(c.Blue), c.Alpha));
    static byte LinearToSrgb(byte b)
    {
        var v = b / 255f;
        return (byte)Math.Clamp((int)MathF.Round(255 * (v <= .0031308f ? 12.92f * v : 1.055f * MathF.Pow(v, 1 / 2.4f) - .055f)), 0, 255);
    }
    static byte[] BakeMaskedBaseColor(TextureRecord baseColor, TextureRecord mask,
        IReadOnlyList<(string Name, int Component, Vector3 Color)> channels)
    {
        using var baseBitmap = SKBitmap.Decode(baseColor.Png!) ?? throw new InvalidDataException("Base Color PNG decode failed.");
        using var maskBitmap = SKBitmap.Decode(mask.Png!) ?? throw new InvalidDataException("ColorMask PNG decode failed.");
        var basePixels = baseBitmap.Pixels;
        var maskPixels = maskBitmap.Pixels;
        for (var i = 0; i < basePixels.Length; i++)
        {
            var source = basePixels[i];
            var maskPixel = maskPixels[i];
            var linear = new Vector3(baseColor.Srgb ? SrgbToLinear(source.Red) : source.Red / 255f,
                baseColor.Srgb ? SrgbToLinear(source.Green) : source.Green / 255f,
                baseColor.Srgb ? SrgbToLinear(source.Blue) : source.Blue / 255f);
            foreach (var channel in channels)
            {
                var amount = channel.Component switch
                {
                    0 => maskPixel.Red / 255f,
                    1 => maskPixel.Green / 255f,
                    _ => maskPixel.Blue / 255f
                };
                linear *= Vector3.Lerp(Vector3.One, channel.Color, amount);
            }
            basePixels[i] = new SKColor(LinearToSrgb(linear.X), LinearToSrgb(linear.Y), LinearToSrgb(linear.Z), source.Alpha);
        }
        baseBitmap.Pixels = basePixels;
        using var data = baseBitmap.Encode(SKEncodedImageFormat.Png, 100);
        return data.ToArray();
    }
    static float SrgbToLinear(byte value)
    {
        var v = value / 255f;
        return v <= .04045f ? v / 12.92f : MathF.Pow((v + .055f) / 1.055f, 2.4f);
    }
    static byte LinearToSrgb(float value)
    {
        var v = Math.Clamp(value, 0, 1);
        return (byte)Math.Clamp((int)MathF.Round(255 * (v <= .0031308f ? 12.92f * v : 1.055f * MathF.Pow(v, 1 / 2.4f) - .055f)), 0, 255);
    }
    static string LinearColorHex(Vector3 color) => $"#{LinearToSrgb(color.X):X2}{LinearToSrgb(color.Y):X2}{LinearToSrgb(color.Z):X2}";
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
    public List<BakeOperationRecord> BakeOperations { get; } = [];
    public List<string> Warnings { get; } = [];
}

internal sealed class BakeOperationRecord
{
    public string Id { get; set; } = "";
    public string Material { get; set; } = "";
    public string OutputRole { get; set; } = "";
    public string Algorithm { get; set; } = "";
    public string[] InputTextures { get; set; } = [];
    public Dictionary<string, string> Parameters { get; set; } = new();
    public string OutputSha256 { get; set; } = "";
    public int OutputImageIndex { get; set; }
    public string Fidelity { get; set; } = "";
}

internal sealed class TextureArtifactRecord
{
    public string Source { get; set; } = "";
    public int? ImageIndex { get; set; }
    public string? Sha256 { get; set; }
    public int Width { get; set; }
    public int Height { get; set; }
    public string? Format { get; set; }
    public bool Srgb { get; set; }
    public bool IsNormal { get; set; }
    public string Disposition { get; set; } = "";
    public string? Error { get; set; }
    public TextureConsumerRecord[] Consumers { get; set; } = [];
}

internal sealed class TextureConsumerRecord
{
    public string Material { get; set; } = "";
    public string Parameter { get; set; } = "";
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

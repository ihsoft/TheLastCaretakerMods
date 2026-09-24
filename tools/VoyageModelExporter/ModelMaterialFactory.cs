using System.Numerics;
using System.Security.Cryptography;
using System.Globalization;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using CUE4Parse.UE4.Assets.Exports.Material;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse_Conversion.Options;
using CUE4Parse_Conversion.Textures;
using SharpGLTF.Materials;
using SharpGLTF.Memory;
using SharpGLTF.Schema2;
using SkiaSharp;

namespace VoyageModelExporter;

internal sealed class ModelMaterialFactory
{
    readonly Dictionary<string, MaterialBuilder> builders = new(StringComparer.Ordinal);
    readonly Dictionary<string, TextureRecord> textureCache = new(StringComparer.Ordinal);
    internal List<MaterialRecord> Materials { get; } = [];
    internal IReadOnlyCollection<TextureRecord> Textures => textureCache.Values;

    internal static string PackagePath(string objectPath)
    {
        var slash = objectPath.LastIndexOf('/');
        var dot = objectPath.IndexOf('.', Math.Max(0, slash));
        return dot < 0 ? objectPath : objectPath[..dot];
    }

    internal MaterialBuilder Get(UMaterialInterface? material, string fallbackName)
    {
        var key = material == null ? "missing:" + fallbackName : PackagePath(material.GetPathName());
        if (builders.TryGetValue(key, out var cached)) return cached;
        if (material == null)
        {
            var missing = new MaterialBuilder(fallbackName).WithMetallicRoughnessShader()
                .WithBaseColor(new Vector4(.5f, .5f, .5f, 1)).WithMetallicRoughness(0, .8f);
            var missingRecord = new MaterialRecord { Source = null, Name = fallbackName };
            missingRecord.Warnings.Add("Material reference is missing or unresolved; neutral fallback used.");
            missing.Extras = JsonSerializer.SerializeToNode(missingRecord, Program.JsonOptions);
            Materials.Add(missingRecord);
            return builders[key] = missing;
        }

        var chain = new List<UUnrealMaterial>();
        for (UUnrealMaterial? current = material; current != null; current = (current as UMaterialInstance)?.Parent)
        {
            if (chain.Count >= 64 || chain.Any(x => x.GetPathName() == current.GetPathName()))
                throw new InvalidDataException($"Cyclic/too-deep material parent chain: {key}");
            chain.Add(current);
        }
        if (chain.Count == 0 || chain[^1] is not UMaterial master)
            throw new InvalidDataException($"Unresolved master material: {key}");
        var parameters = new CMaterialParams2();
        material.GetParams(parameters, EMaterialDepth.AllLayers);
        var report = new MaterialRecord
        {
            Source = key,
            Name = key.Split('/')[^1],
            Parents = chain.Skip(1).Select(x => PackagePath(x.GetPathName())).ToArray(),
            Parameters = JsonNode.Parse(Newtonsoft.Json.JsonConvert.SerializeObject(parameters))!
        };
        foreach (var pair in parameters.Scalars.Where(x => IsDeferredEffectName(x.Key)))
            report.EffectControls[pair.Key] = pair.Value.ToString("R", CultureInfo.InvariantCulture);
        foreach (var pair in parameters.Switches.Where(x => IsDeferredEffectName(x.Key)))
            report.EffectControls[pair.Key] = pair.Value.ToString();
        report.Warnings.Add("Approximation, not Unreal shader baking. Layer mixing, world/object coordinates, UV math, vertex data, animation and runtime effects are not evaluated.");
        foreach (var pair in parameters.Textures.OrderBy(x => x.Key, StringComparer.Ordinal))
            report.TextureParameters[pair.Key] = PackagePath(pair.Value.GetPathName());
        TextureRecord Resolve(string path)
        {
            var source = parameters.Textures.Values.First(x => PackagePath(x.GetPathName()) == path);
            if (!textureCache.TryGetValue(path, out var texture)) textureCache[path] = texture = Decode(source);
            return texture;
        }
        var builder = MakeMaterial(report, parameters, Resolve);
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
        if (blend != EBlendMode.BLEND_Opaque)
            report.Warnings.Add($"Unreal {blend}; opacity/refraction graph not evaluated. Mask/blend uses base-color alpha only.");
        report.RenderState = new { blend = blend.ToString(), twoSided, cutoff, masterShadingModel = master.ShadingModel.ToString() };
        builder.Extras = JsonSerializer.SerializeToNode(report, Program.JsonOptions);
        Materials.Add(report);
        return builders[key] = builder;
    }

    static TextureRecord Decode(UUnrealMaterial source)
    {
        var record = new TextureRecord { Source = PackagePath(source.GetPathName()) };
        try
        {
            if (source is not UTexture2D texture) throw new NotSupportedException("Only Texture2D can become a glTF PNG.");
            record.Format = texture.Format.ToString();
            record.Srgb = texture.SRGB;
            record.IsNormal = texture.IsNormalMap;
            var pixels = texture.Decode() ?? throw new InvalidDataException("No decodable resident mip.");
            record.Width = pixels.Width;
            record.Height = pixels.Height;
            record.Png = pixels.Encode(ETextureFormat.Png, false, out _);
            record.Sha256 = Convert.ToHexString(SHA256.HashData(record.Png));
            if (PixelFormatUtils.IsHDR(pixels.PixelFormat)) record.Warning = "HDR quantized to PNG for glTF; original radiance range is not preserved.";
        }
        catch (Exception ex) { record.Error = ex.GetType().Name + ": " + ex.Message; }
        return record;
    }

    static string Normalize(string value) => Regex.Replace(value.ToLowerInvariant(), "[^a-z0-9]", "");
    static bool IsDeferredEffectName(string value)
    {
        var normalized = Normalize(value);
        return normalized.Contains("damage", StringComparison.Ordinal) ||
            normalized.Contains("rust", StringComparison.Ordinal) ||
            normalized.Contains("wear", StringComparison.Ordinal) ||
            normalized.Contains("dirt", StringComparison.Ordinal);
    }

    static MaterialBuilder MakeMaterial(MaterialRecord report, CMaterialParams2 parameters, Func<string, TextureRecord> resolve)
    {
        var builder = new MaterialBuilder(report.Name).WithMetallicRoughnessShader();
        TextureRecord? Select(string role, params string[] aliases)
        {
            var candidates = report.TextureParameters.Where(p => aliases.Contains(Normalize(p.Key)))
                .Select(p => p.Value).Distinct().ToArray();
            if (candidates.Length == 0)
                candidates = report.TextureParameters.Values.Distinct().Where(path => aliases.Any(a =>
                    Normalize(path.Split('/')[^1]).EndsWith(a, StringComparison.Ordinal))).ToArray();
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
            var maskPaths = report.TextureParameters.Where(x => Normalize(x.Key) == "colormask")
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
            if (maskPaths.Length == 1 && maskChannels.Count > 0)
            {
                var mask = resolve(maskPaths[0]);
                if (mask.Png != null && mask.Width == baseColor.Width && mask.Height == baseColor.Height)
                {
                    baseColorPng = BakeMaskedBaseColor(baseColor, mask, maskChannels);
                    baseColorTransform = "masked-color-bake:" + string.Join(",", maskChannels.Select(x =>
                        $"{x.Name}={LinearColorHex(x.Color)}"));
                    report.Bindings["colorMask"] = maskPaths[0];
                    report.BakedEffects.Add($"ColorMask tint baked into Base Color with {string.Join(", ", maskChannels.Select(x => x.Name + "=" + LinearColorHex(x.Color)))}.");
                    report.Warnings.Add("Masked-color bake assumes BaseColor * lerp(white, MaskedColor, mask channel); the cooked Unreal shader graph was not evaluated.");
                }
                else
                {
                    report.SkippedTextures[maskPaths[0]] = mask.Png == null
                        ? "ColorMask selected for baking but decoding failed: " + mask.Error
                        : $"ColorMask dimensions {mask.Width}x{mask.Height} do not match Base Color {baseColor.Width}x{baseColor.Height}.";
                }
            }
            else if (maskPaths.Length > 1)
            {
                report.Warnings.Add($"Ambiguous ColorMask; masked-color bake skipped: {string.Join(", ", maskPaths)}");
            }
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
        foreach (var source in report.TextureParameters.Values.Distinct().Where(p => !report.Bindings.ContainsValue(p)))
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

    internal void MatchUsedImages(ModelRoot model)
    {
        var images = model.LogicalImages.ToDictionary(x => x.LogicalIndex,
            x => Convert.ToHexString(SHA256.HashData(x.Content.Content.ToArray())));
        var described = new HashSet<int>();
        foreach (var texture in textureCache.Values)
            foreach (var variant in texture.Variants)
            {
                var index = images.Single(x => x.Value == variant.Sha256).Key;
                variant.ImageIndex = index;
                described.Add(index);
                model.LogicalImages[index].Name ??= texture.Source.Split('/')[^1] + "_" + variant.Role;
            }
        if (described.Count != model.LogicalImages.Count)
            throw new InvalidDataException("An embedded image has no used-variant provenance.");
    }

    static byte[] FlipNormalGreen(byte[] png) => Transform(png, c => new SKColor(c.Red, (byte)(255 - c.Green), c.Blue, c.Alpha));
    static byte[] ColorPng(TextureRecord record) => record.Srgb ? record.Png! : Transform(record.Png!, c =>
        new SKColor(LinearToSrgb(c.Red), LinearToSrgb(c.Green), LinearToSrgb(c.Blue), c.Alpha));
    static byte LinearToSrgb(byte value)
    {
        var v = value / 255f;
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
            basePixels[i] = new SKColor(LinearToSrgb(linear.X), LinearToSrgb(linear.Y),
                LinearToSrgb(linear.Z), source.Alpha);
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
}

internal sealed class MaterialRecord
{
    public string? Source { get; set; }
    public string Name { get; set; } = "";
    public string[] Parents { get; set; } = [];
    public JsonNode Parameters { get; set; } = new JsonObject();
    public object? RenderState { get; set; }
    public Dictionary<string, string> TextureParameters { get; } = new();
    public Dictionary<string, string> Bindings { get; } = new();
    public Dictionary<string, string> SkippedTextures { get; } = new();
    public Dictionary<string, string> EffectControls { get; } = new();
    public List<string> BakedEffects { get; } = [];
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

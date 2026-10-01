using System.Numerics;
using System.Text.Json;
using CUE4Parse.UE4.Assets.Exports.Material;
using SharpGLTF.Schema2;
using SharpGLTF.Scenes;
using SkiaSharp;

namespace VoyageMaterialLibrary;

internal static class SelfTests
{
    public static int Run(string outputDirectory)
    {
        if (Directory.Exists(outputDirectory)) throw new IOException("Self-test directory must be fresh.");
        Directory.CreateDirectory(outputDirectory);
        var checks = 0;
        void Check(bool value, string name) { if (!value) throw new Exception(name); checks++; }
        foreach (var invalid in new[] { Array.Empty<string>(), new[] { "/Game/M_A", "/Game/M_A" }, new[] { "/Game/../M_A" }, new[] { "/Game/M_A.M_A" } })
        {
            var rejected = false;
            try { Program.ValidateAssets(invalid); } catch (ArgumentException) { rejected = true; }
            Check(rejected, "invalid identity not rejected");
        }
        Program.ValidateAssets(["/Game/Test/M_A", "/Engine/Test/M_B", "/Channel37/Test/MI_C"]);
        checks++;
        using var bitmap = new SKBitmap(2, 2);
        bitmap.Erase(new SKColor(30, 80, 220, 255));
        using var data = bitmap.Encode(SKEncodedImageFormat.Png, 100);
        var png = data.ToArray();
        using var flipped = SKBitmap.Decode(Program.FlipNormalGreen(png));
        Check(flipped.GetPixel(0, 0) == new SKColor(30, 175, 220, 255), "normal convention");
        using var packedRoughness = SKBitmap.Decode(Program.PackRoughness(png));
        Check(packedRoughness.GetPixel(0, 0) == new SKColor(255, 30, 0, 255), "standalone roughness packed to ORM green");
        Check(Program.IsColorPreviewCandidate(new TextureRecord { Source = "/Game/T_PaintedMetal_BC.T_PaintedMetal_BC", Srgb = true, Png = png }, ["BaseColorT"]),
            "preview includes color texture");
        Check(Program.IsColorPreviewCandidate(new TextureRecord { Source = "/Game/T_White_Color.T_White_Color", Srgb = true, Png = png }, ["Color Map"]),
            "color map must not be mistaken for ORM data");
        Check(!Program.IsColorPreviewCandidate(new TextureRecord { Source = "/Game/T_PaintedMetal_N.T_PaintedMetal_N", Srgb = false, IsNormal = true, Png = png }, ["NormalT"]),
            "preview excludes normal texture");
        Check(!Program.IsColorPreviewCandidate(new TextureRecord { Source = "/Game/T_DirtMask_M.T_DirtMask_M", Srgb = true, Png = png }, ["WorldAlignedTextureMask"]),
            "preview excludes named mask even when marked sRGB");
        Check(!Program.IsColorPreviewCandidate(new TextureRecord { Source = "/Game/T_PaintedMetal_ORM.T_PaintedMetal_ORM", Srgb = false, Png = png }, ["PM_SpecularMasks"]),
            "preview excludes linear data texture");
        var sourcePolicyParameters = new CMaterialParams2();
        Check(Program.IsImportantSkippedSource("/Game/T_RustMask.T_RustMask", ["Rust Mask"], sourcePolicyParameters, out _),
            "material pack includes recognized skipped rust masks");
        sourcePolicyParameters.Switches["Use Rust"] = false;
        Check(!Program.IsImportantSkippedSource("/Game/T_RustMask.T_RustMask", ["Rust Mask"], sourcePolicyParameters, out _),
            "material pack excludes explicitly disabled effects");
        Check(!Program.IsImportantSkippedSource("/Game/T_DefaultWhite.T_DefaultWhite", ["Damage Mask"], new CMaterialParams2(), out _),
            "material pack excludes fallback pixels");
        Check(!Program.IsImportantSkippedSource("/Game/T_GenericColor.T_GenericColor", ["Unknown Parent Input"], new CMaterialParams2(), out _),
            "material pack keeps unrelated parent inputs as metadata only");
        Check(!Program.IsImportantSkippedSource("/Game/T_DirtMask_M.T_DirtMask_M", ["T_DirtMask_M"], new CMaterialParams2(), out _),
            "material pack does not treat inherited texture-name bindings as active skipped effects");
        Check(Program.ParseSourceTexturePolicy("MetadataOnly") == SourceTexturePolicy.MetadataOnly,
            "material pack parses MetadataOnly source policy");
        Check(Program.ParseSourceTexturePolicy("Reconstructable") == SourceTexturePolicy.Reconstructable,
            "material pack parses Reconstructable source policy");
        var invalidSourcePolicyRejected = false;
        try { Program.ParseSourceTexturePolicy("All"); } catch (ArgumentException) { invalidSourcePolicyRejected = true; }
        Check(invalidSourcePolicyRejected, "material pack rejects unknown source policy");
        var numericSourcePolicyRejected = false;
        try { Program.ParseSourceTexturePolicy("0"); } catch (ArgumentException) { numericSourcePolicyRejected = true; }
        Check(numericSourcePolicyRejected, "material pack rejects numeric source policy");
        Check(Program.ParseMaterialPackProfile("Full") == MaterialPackProfile.Full &&
            Program.ParseMaterialPackProfile("AnalysisCompact") == MaterialPackProfile.AnalysisCompact &&
            Program.ParseMaterialPackProfile("Reconstructable") == MaterialPackProfile.Reconstructable,
            "material pack parses quality profiles");
        using var normalSource = new SKBitmap(4, 8);
        normalSource.Erase(new SKColor(128, 128, 255, 255));
        using var normalSourceData = normalSource.Encode(SKEncodedImageFormat.Png, 100);
        using var compactNormal = SKBitmap.Decode(Program.ResizeAnalysisImage(normalSourceData.ToArray(), true, 2, 4));
        Check(compactNormal.Width == 2 && compactNormal.Height == 4, "analysis normal preserves aspect ratio and bounds");
        var compactPixel = compactNormal.GetPixel(0, 0);
        var compactVector = new Vector3(compactPixel.Red / 127.5f - 1, compactPixel.Green / 127.5f - 1,
            compactPixel.Blue / 127.5f - 1);
        Check(Math.Abs(compactVector.Length() - 1) < .02f, "analysis normal is renormalized");
        var record = new MaterialRecord { Source = "/Game/Test/M_A", Name = "M_A" };
        record.Textures["BaseColor"] = "color";
        record.Textures["Normal"] = "normal";
        record.Textures["ORM"] = "orm";
        record.Textures["Emissive"] = "emissive";
        record.Textures["UnknownParentDependency"] = "unused";
        var textures = record.Textures.Values.ToDictionary(x => x, x => new TextureRecord { Source = x, Srgb = true, Png = png });
        var decoded = new HashSet<string>();
        TextureRecord Resolve(string source) { decoded.Add(source); return textures[source]; }
        var parameters = new CMaterialParams2();
        parameters.Scalars["Emissive Strenght"] = 0;
        parameters.Scalars["Metallic"] = .7f;
        parameters.Scalars["Roughness"] = .4f;
        var material = Program.MakeMaterial(record, parameters, Resolve);
        Check(!decoded.Contains("emissive") && !decoded.Contains("unused"), "disabled/unknown textures must not be decoded");
        Check(record.SkippedTextures.ContainsKey("emissive") && record.SkippedTextures.ContainsKey("unused"), "omissions must be reported");
        Check(record.Bindings["ORM"] == "orm", "ORM binding");
        var scene = new SceneBuilder();
        scene.AddRigidMesh(Program.Swatch(material, "sample"), Matrix4x4.Identity);
        var path = Path.Combine(outputDirectory, "synthetic.glb");
        var model = scene.ToGltf2();
        Program.MatchUsedImages(model, textures.Values);
        model.SaveGLB(path);
        var roundtrip = ModelRoot.Load(path);
        Check(roundtrip.LogicalMaterials.Count == 1 && roundtrip.LogicalImages.Count >= 2, "textured GLB readback");
        // Inspect actual glTF representation, independent of material-builder properties.
        var bytes = File.ReadAllBytes(path);
        using var json = JsonDocument.Parse(bytes.AsMemory(20, BitConverter.ToInt32(bytes, 12)));
        var m = json.RootElement.GetProperty("materials")[0];
        var pbr = m.GetProperty("pbrMetallicRoughness");
        Check(Math.Abs(pbr.GetProperty("metallicFactor").GetSingle() - .7f) < .00001f, "metallic scalar");
        Check(Math.Abs(pbr.GetProperty("roughnessFactor").GetSingle() - .4f) < .00001f, "roughness scalar");
        Check(!m.TryGetProperty("emissiveFactor", out var emission) || emission.EnumerateArray().All(x => x.GetSingle() == 0), "NoEmis must remain dark even in importers without extensions");
        Check(!m.TryGetProperty("emissiveTexture", out _), "disabled emission must not carry an image");
        var referenced = new HashSet<int>();
        foreach (var entry in pbr.EnumerateObject().Concat(m.EnumerateObject()))
            if (entry.Value.ValueKind == JsonValueKind.Object && entry.Value.TryGetProperty("index", out var textureIndex))
                referenced.Add(json.RootElement.GetProperty("textures")[textureIndex.GetInt32()].GetProperty("source").GetInt32());
        Check(referenced.Count == roundtrip.LogicalImages.Count, "no orphan images");
        Check(textures["normal"].Variants.Count == 1 && textures["normal"].Variants[0].Transform == "invert-green", "only converted normal retained");
        var automotive = new MaterialRecord { Name = "automotive" };
        automotive.Textures["Color Map"] = "color";
        var automotiveParameters = new CMaterialParams2();
        automotiveParameters.Colors["Tint"] = new CUE4Parse.UE4.Objects.Core.Math.FLinearColor(.016f, .016f, .016f, 1);
        automotiveParameters.Switches["Use Tint"] = true;
        var automotiveMaterial = Program.MakeMaterial(automotive, automotiveParameters, Resolve, "BakeReconstructed");
        Check(automotive.Bindings["baseColor"] == "color" && automotive.Bindings.ContainsKey("baseColorFactor"),
            "reconstructed automotive material uses Color Map and Tint");
        var stacked = new MaterialRecord
        {
            Source = "/Game/Test/MI_Stacked",
            Name = "MI_Stacked",
            Parents = ["/Game/AssetSets/Items/Materials/Stacks/Materials/Parent/M_StackedMaterial_Opaque.M_StackedMaterial_Opaque"]
        };
        stacked.Textures["BaseColorTexture"] = "stacked-color";
        stacked.Textures["MicroNormal"] = "stacked-normal";
        stacked.Textures["MicroORM"] = "stacked-orm";
        var stackedTextures = stacked.Textures.Values.ToDictionary(x => x,
            x => new TextureRecord { Source = x, Srgb = x == "stacked-color", Png = png, Width = 2, Height = 2 });
        var stackedParameters = new CMaterialParams2();
        stackedParameters.Colors["BaseColor"] = new CUE4Parse.UE4.Objects.Core.Math.FLinearColor(.8f, .8f, .8f, 1);
        stackedParameters.Scalars["MicroTiling"] = 2;
        stackedParameters.Scalars["RoughnessMin"] = .2f;
        stackedParameters.Scalars["RoughnessMax"] = .4f;
        stackedParameters.Switches["UseMicroRoughness?"] = true;
        Program.MakeMaterial(stacked, stackedParameters, source => stackedTextures[source], "BakeReconstructed");
        Check(stacked.BakeOperations.Count == 3 && stacked.BakeOperations.Select(x => x.OutputRole)
            .OrderBy(x => x).SequenceEqual(new[] { "baseColor", "normal", "ORM" }.OrderBy(x => x)),
            "stacked opaque bake records portable base color, normal and ORM operations");
        using var stackedBaseColor = SKBitmap.Decode(stackedTextures["stacked-color"].Variants.Single().Data);
        var stackedColorPixel = stackedBaseColor.GetPixel(0, 0);
        Check(stackedColorPixel.Red == stackedColorPixel.Green && stackedColorPixel.Green == stackedColorPixel.Blue,
            "stacked opaque bake removes inherited micro-texture chroma before BaseColor tint");
        using var stackedOrm = SKBitmap.Decode(stackedTextures["stacked-orm"].Variants.Single().Data);
        Check(Math.Abs(stackedOrm.GetPixel(0, 0).Green / 255f - (.2f + .2f * (80 / 255f))) < .01f,
            "stacked opaque bake remaps micro roughness to configured bounds");
        Check(stacked.Bindings["baseColorFactor"].StartsWith("BaseColor", StringComparison.Ordinal),
            "stacked opaque bake preserves material hue as glTF BaseColor factor");
        record.Textures["Albedo"] = "secondColor";
        textures["secondColor"] = new TextureRecord { Source = "secondColor", Srgb = true, Png = png };
        var ambiguous = new MaterialRecord { Name = "ambiguous" };
        foreach (var entry in record.Textures) ambiguous.Textures.Add(entry.Key, entry.Value);
        decoded.Clear();
        Program.MakeMaterial(ambiguous, parameters, Resolve);
        Check(!ambiguous.Bindings.ContainsKey("baseColor") && ambiguous.Warnings.Any(x => x.Contains("Ambiguous baseColor")), "ambiguous maps must not be guessed");
        Check(!decoded.Contains("color") && !decoded.Contains("secondColor"), "ambiguous images must not be decoded");
        var plain = new MaterialRecord { Name = "plain" };
        plain.Textures["Unknown"] = "must-not-decode";
        var plainMaterial = Program.MakeMaterial(plain, new CMaterialParams2(), _ => throw new Exception("Unexpected decode"));
        var plainScene = new SceneBuilder();
        plainScene.AddRigidMesh(Program.Swatch(plainMaterial, "plain"), Matrix4x4.Identity);
        var plainModel = plainScene.ToGltf2();
        Program.MatchUsedImages(plainModel, []);
        Check(plainModel.LogicalImages.Count == 0, "parameter-only material needs no images");
        var packStage = Path.Combine(outputDirectory, "material-pack-stage");
        Directory.CreateDirectory(Path.Combine(packStage, "pbr"));
        using var previewBitmap = new SKBitmap(768, 768);
        previewBitmap.Erase(new SKColor(40, 50, 60));
        using (var previewData = previewBitmap.Encode(SKEncodedImageFormat.Webp, 100))
            File.WriteAllBytes(Path.Combine(packStage, "preview.webp"), previewData.ToArray());
        var packBaseColor = Path.Combine(packStage, "pbr", "basecolor.webp");
        using (var baseColorData = bitmap.Encode(SKEncodedImageFormat.Webp, 100))
            File.WriteAllBytes(packBaseColor, baseColorData.ToArray());
        var packManifest = new
        {
            schemaVersion = 1,
            sourceTexturePolicy = "MetadataOnly",
            material = new { name = "MI_Test", source = "/Game/Test/MI_Test", parents = Array.Empty<string>() },
            renderState = new { blendMode = "BLEND_Opaque", shadingModel = "MSM_DefaultLit", twoSided = false, opacityMaskClipValue = (float?)null },
            pbr = new { baseColorFactor = new[] { 1, 1, 1, 1 }, metallicFactor = 0, roughnessFactor = 1,
                baseColor = new { file = "pbr/basecolor.webp", colorSpace = "sRGB", sha256 = Program.Hash(packBaseColor), width = 2, height = 2 } },
            parameters = new { colors = new { }, scalars = new { }, switches = new { } },
            textureBindings = new { }, sourceTextures = Array.Empty<object>(),
            uv = new { mode = "unknown", tiling = (float[]?)null, offset = (float[]?)null, rotationDegrees = (float?)null },
            reconstruction = new { mode = "BakeReconstructed", bakeOperations = Array.Empty<object>(), warnings = new[] { "bounded" }, skippedEffects = Array.Empty<object>() },
            preview = new { file = "preview.webp", recipe = "voyage.material-sphere/1", width = 768, height = 768, renderer = "Blender EEVEE", exposure = 0 },
            provenance = new { steamBuildId = "test" }
        };
        File.WriteAllText(Path.Combine(packStage, "manifest.json"), JsonSerializer.Serialize(packManifest));
        var packOne = Path.Combine(outputDirectory, "pack-one", "MI_Test.materialpack.zip");
        var packTwo = Path.Combine(outputDirectory, "pack-two", "MI_Test.materialpack.zip");
        Check(Program.FinalizeMaterialPack(packStage, packOne) == 0 && Program.VerifyMaterialPack(packOne) == 0,
            "material-pack build/readback");
        Check(Program.FinalizeMaterialPack(packStage, packTwo) == 0 && Program.VerifyMaterialPack(packTwo) == 0,
            "material-pack second build/readback");
        Check(Program.Hash(packOne) == Program.Hash(packTwo), "material-pack archive must be deterministic");
        Console.WriteLine(JsonSerializer.Serialize(new { status = "passed", checks, path }));
        return 0;
    }
}

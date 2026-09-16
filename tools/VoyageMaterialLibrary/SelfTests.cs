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
        Console.WriteLine(JsonSerializer.Serialize(new { status = "passed", checks, path }));
        return 0;
    }
}

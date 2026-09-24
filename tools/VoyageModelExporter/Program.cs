using System.Numerics;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using CUE4Parse.FileProvider;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Assets.Exports.Component;
using CUE4Parse.UE4.Assets.Exports.Component.StaticMesh;
using CUE4Parse.UE4.Assets.Exports.Engine;
using CUE4Parse.UE4.Assets.Exports.Material;
using CUE4Parse.UE4.Assets.Exports.StaticMesh;
using CUE4Parse.UE4.Objects.Core.Math;
using CUE4Parse.UE4.Objects.UObject;
using CUE4Parse.UE4.Versions;
using CUE4Parse_Conversion.Options;
using CUE4Parse_Conversion.Textures;
using SharpGLTF.Scenes;
using SharpGLTF.Schema2;

namespace VoyageModelExporter;

internal static class Program
{
    internal const string ConversionCommit = "ec6595e46448a817ac21ea9bde01caa48f80a420";
    internal static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };
    internal static string Hash(string path) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)));

    static int Main(string[] args)
    {
        try
        {
            if (args.Length != 1) throw new ArgumentException("Usage: VoyageModelExporter <request.json>");
            var request = JsonNode.Parse(File.ReadAllText(args[0]))!;
            string Str(string key) => request[key]!.GetValue<string>();
            var source = Str("asset");
            if (!Regex.IsMatch(source, @"^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$") || source.Trim() != source)
                throw new ArgumentException("Use one exact model or Blueprint package identity without object suffix, wildcard or traversal.");
            var output = Path.GetFullPath(Str("output"));
            if (File.Exists(output)) throw new IOException("Output exists; choose a fresh GLB path.");
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

            Directory.SetCurrentDirectory(Path.GetDirectoryName(Path.GetFullPath(args[0]))!);
            TextureDecoder.UseAssetRipperTextureDecoder = true;
            using var provider = new StockProvider(new DirectoryInfo(Path.Combine(gameRoot, "Voyage/Content/Paks")));
            provider.MappingsContainer = new FileUsmapTypeMappingsProvider(Str("mappingPath"));
            provider.Initialize(); provider.Mount(); provider.PostMount(); provider.LoadVirtualPaths();
            var exports = provider.LoadPackage(source).GetExports();
            var scene = new SceneBuilder();
            var materialFactory = new ModelMaterialFactory();
            var meshRecords = new List<MeshRecord>();
            var omitted = new Dictionary<string, int>(StringComparer.Ordinal);
            var omittedInstances = new List<OmittedComponentRecord>();
            var nodeCount = 0;

            void Omit(string key, string node, string reason)
            {
                omitted[key] = omitted.GetValueOrDefault(key) + 1;
                omittedInstances.Add(new OmittedComponentRecord { Node = node, Kind = key, Reason = reason });
            }
            string CleanName(string name) => name.EndsWith("_GEN_VARIABLE", StringComparison.Ordinal)
                ? name[..^"_GEN_VARIABLE".Length] : name;
            NodeBuilder ConfigureNode(NodeBuilder node, USceneComponent component)
            {
                var transform = component.GetRelativeTransform();
                var q = transform.Rotation;
                node.WithLocalTranslation(new Vector3(transform.Translation.X, transform.Translation.Z, transform.Translation.Y) * .01f)
                    .WithLocalRotation(Quaternion.Normalize(new Quaternion(q.X, q.Z, q.Y, -q.W)))
                    .WithLocalScale(new Vector3(transform.Scale3D.X, transform.Scale3D.Z, transform.Scale3D.Y));
                return node;
            }
            MaterialBinding[] ResolveMaterials(UStaticMesh mesh, UStaticMeshComponent? component)
            {
                var result = new MaterialBinding[mesh.StaticMaterials.Length];
                for (var i = 0; i < result.Length; i++)
                {
                    FPackageIndex? selected = null;
                    if (component != null && i < component.OverrideMaterials.Length && component.OverrideMaterials[i] is { IsNull: false } replacement)
                        selected = replacement;
                    else if (mesh.StaticMaterials[i].MaterialInterface is { IsNull: false } original)
                        selected = original;
                    UMaterialInterface? material = selected?.Load<UMaterialInterface>();
                    result[i] = new MaterialBinding(materialFactory.Get(material, mesh.StaticMaterials[i].MaterialSlotName.Text),
                        material == null ? null : ModelMaterialFactory.PackagePath(material.GetPathName()));
                }
                return result;
            }
            void AddMesh(NodeBuilder node, UStaticMesh mesh, UStaticMeshComponent? component, string componentName)
            {
                var bindings = ResolveMaterials(mesh, component);
                using var dto = new CUE4Parse_Conversion.Dto.StaticMeshDto(mesh, EMeshQuality.Highest, ENaniteMeshFormat.NoNanite);
                var lod = dto.LODs.FirstOrDefault() ?? throw new InvalidDataException($"No exportable mesh LOD: {mesh.GetPathName()}");
                var builder = ModelMeshWriter.Build(componentName, lod, bindings.Select(x => x.Builder).ToArray());
                scene.AddRigidMesh(builder, node);
                meshRecords.Add(new MeshRecord
                {
                    Component = componentName,
                    Source = ModelMaterialFactory.PackagePath(mesh.GetPathName()),
                    Geometry = lod.IsNanite ? "Nanite fallback" : $"ordinary LOD{lod.SourceLodIndex}",
                    Vertices = lod.Vertices.Length,
                    Triangles = lod.Sections.Sum(x => x.NumFaces),
                    Sections = lod.Sections.Length,
                    Materials = bindings.Select(x => x.Source).ToArray()
                });
            }
            void Visit(USCS_Node scsNode, NodeBuilder? parent)
            {
                var component = scsNode.GetComponentTemplate();
                if (component == null)
                {
                    Omit("Unresolved SCS component template", scsNode.InternalVariableName.ToString(),
                        "The serialized component template could not be loaded as a scene component.");
                    return;
                }
                nodeCount++;
                var name = CleanName(component.Name);
                var node = ConfigureNode(parent == null ? new NodeBuilder(name) : parent.CreateNode(name), component);
                if (component is UStaticMeshComponent staticComponent)
                {
                    var mesh = staticComponent.GetLoadedStaticMesh();
                    if (mesh == null) Omit("StaticMeshComponent without a mesh", name, "No static mesh is assigned in the default component template.");
                    else AddMesh(node, mesh, staticComponent, name);
                }
                else if (component is not USceneComponent || component.GetType() != typeof(USceneComponent))
                {
                    Omit(component.GetType().Name, name, "Component has no supported static-mesh geometry representation in glTF.");
                }
                foreach (var child in scsNode.GetChildNodes()) Visit(child, node);
            }

            var directMeshes = exports.OfType<UStaticMesh>().ToArray();
            var constructionScripts = exports.OfType<USimpleConstructionScript>().ToArray();
            string sourceKind;
            if (directMeshes.Length == 1 && constructionScripts.Length == 0)
            {
                sourceKind = "StaticMesh";
                nodeCount = 1;
                AddMesh(new NodeBuilder(directMeshes[0].Name), directMeshes[0], null, directMeshes[0].Name);
            }
            else if (constructionScripts.Length == 1)
            {
                sourceKind = "BlueprintSCS";
                foreach (var rootNode in constructionScripts[0].GetRootNodes()) Visit(rootNode, null);
            }
            else
            {
                throw new InvalidDataException($"Expected one StaticMesh or one Blueprint SimpleConstructionScript: {source}");
            }
            if (meshRecords.Count == 0) throw new InvalidDataException("The selected asset produced no static mesh components.");

            var model = scene.ToGltf2();
            materialFactory.MatchUsedImages(model);
            var evidence = new
            {
                schemaVersion = 1,
                status = materialFactory.Textures.Any(x => x.Error != null) ? "partial-textures" : "approximate",
                source,
                sourceKind,
                steamBuildId = build,
                executableSha256 = exeHash,
                mappingSha256 = Hash(Str("mappingPath")),
                conversionCommit = ConversionCommit,
                exporterSha256 = Hash(typeof(Program).Assembly.Location),
                parserSha256 = Hash(typeof(DefaultFileProvider).Assembly.Location),
                nodeCount,
                meshComponents = meshRecords,
                omittedComponents = omitted.OrderBy(x => x.Key).ToDictionary(x => x.Key, x => x.Value),
                omittedComponentInstances = omittedInstances,
                materials = materialFactory.Materials,
                textures = materialFactory.Textures,
                limitations = new[]
                {
                    "Static default representation only: Blueprint construction scripts, runtime spawning, state changes and animations are not executed.",
                    "Only StaticMeshComponent geometry is exported; skeletal meshes, splines, Niagara, decals, widgets, lights, audio, collisions and child actors are reported or omitted.",
                    "Highest ordinary render LOD is preferred; high-resolution Nanite reconstruction is not promised.",
                    "Materials are bounded glTF PBR approximations, not Unreal shader baking."
                }
            };
            model.Extras = JsonSerializer.SerializeToNode(evidence, JsonOptions);
            var pending = Path.Combine(Directory.GetCurrentDirectory(), "pending.glb");
            model.SaveGLB(pending);
            var readback = ModelRoot.Load(pending);
            if (readback.LogicalMeshes.Count != meshRecords.Count || readback.LogicalMaterials.Count > materialFactory.Materials.Count)
                throw new InvalidDataException("GLB mesh/material readback mismatch.");
            Directory.CreateDirectory(Path.GetDirectoryName(output)!);
            File.Move(pending, output, false);
            var reportPath = Path.Combine(Directory.GetCurrentDirectory(), "export-report.json");
            File.WriteAllText(reportPath, JsonSerializer.Serialize(evidence, JsonOptions));
            var omissionsPath = Path.Combine(Directory.GetCurrentDirectory(), "material-omissions.md");
            File.WriteAllText(omissionsPath, MaterialOmissions(materialFactory.Materials));
            Console.WriteLine(JsonSerializer.Serialize(new
            {
                status = evidence.status,
                glbPath = output,
                sha256 = Hash(output),
                sourceKind,
                nodeCount,
                meshCount = meshRecords.Count,
                uniqueMeshCount = meshRecords.Select(x => x.Source).Distinct(StringComparer.Ordinal).Count(),
                materialCount = readback.LogicalMaterials.Count,
                discoveredMaterialRecords = materialFactory.Materials.Count,
                imageCount = readback.LogicalImages.Count,
                failedTextures = materialFactory.Textures.Count(x => x.Error != null),
                omittedComponentKinds = omitted.Count,
                reportPath,
                omissionsPath
            }));
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex);
            return 1;
        }
    }

    static string MaterialOmissions(IEnumerable<MaterialRecord> materials)
    {
        var result = new StringBuilder();
        result.AppendLine("# Material effects and omissions").AppendLine();
        result.AppendLine("Generated alongside the GLB. Paths identify cooked source textures; omitted images are not copied into this evidence directory.").AppendLine();
        foreach (var material in materials.Where(x => x.BakedEffects.Count > 0 || x.SkippedTextures.Count > 0 || x.EffectControls.Count > 0))
        {
            result.Append("## ").AppendLine(material.Name).AppendLine();
            result.Append("Source: `").Append(material.Source ?? "unresolved").AppendLine("`").AppendLine();
            if (material.BakedEffects.Count > 0)
            {
                result.AppendLine("Applied approximations:");
                foreach (var effect in material.BakedEffects) result.Append("- ").AppendLine(effect);
                result.AppendLine();
            }
            if (material.SkippedTextures.Count > 0)
            {
                result.AppendLine("Deferred texture effects:");
                foreach (var skipped in material.SkippedTextures.OrderBy(x => x.Key, StringComparer.Ordinal))
                {
                    var parameterNames = material.TextureParameters.Where(x => x.Value == skipped.Key).Select(x => x.Key).ToArray();
                    result.Append("- ").Append(DeferredEffectName(skipped.Key, parameterNames)).Append(": `").Append(skipped.Key).AppendLine("`");
                    if (parameterNames.Length > 0) result.Append("  - Parameters: `").Append(string.Join("`, `", parameterNames)).AppendLine("`");
                    result.Append("  - Status: ").AppendLine(skipped.Value);
                }
                result.AppendLine();
            }
            if (material.EffectControls.Count > 0)
            {
                result.AppendLine("Relevant controls:");
                foreach (var control in material.EffectControls.OrderBy(x => x.Key, StringComparer.Ordinal))
                    result.Append("- `").Append(control.Key).Append(" = ").Append(control.Value).AppendLine("`");
                result.AppendLine();
            }
        }
        return result.ToString();
    }

    static string DeferredEffectName(string texturePath, IReadOnlyCollection<string> parameterNames)
    {
        var identity = (texturePath + " " + string.Join(" ", parameterNames)).ToLowerInvariant();
        if (identity.Contains("rust", StringComparison.Ordinal) || identity.Contains("damage", StringComparison.Ordinal)) return "Rust/damage layer";
        if (identity.Contains("mask", StringComparison.Ordinal)) return "Mask-driven layer";
        if (identity.Contains("normal", StringComparison.Ordinal)) return "Additional normal detail";
        if (identity.Contains("occlusion", StringComparison.Ordinal) || identity.Contains("rough", StringComparison.Ordinal) || identity.Contains("metal", StringComparison.Ordinal)) return "Additional surface-property map";
        if (identity.Contains("basecolor", StringComparison.Ordinal) || identity.Contains("diffuse", StringComparison.Ordinal)) return "Additional color layer";
        return "Unresolved material layer";
    }
}

internal sealed record MaterialBinding(SharpGLTF.Materials.MaterialBuilder Builder, string? Source);

internal sealed class MeshRecord
{
    public string Component { get; set; } = "";
    public string Source { get; set; } = "";
    public string Geometry { get; set; } = "";
    public int Vertices { get; set; }
    public int Triangles { get; set; }
    public int Sections { get; set; }
    public string?[] Materials { get; set; } = [];
}

internal sealed class OmittedComponentRecord
{
    public string Node { get; set; } = "";
    public string Kind { get; set; } = "";
    public string Reason { get; set; } = "";
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

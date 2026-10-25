using System.Numerics;
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
using CUE4Parse_Conversion.Dto;
using CUE4Parse_Conversion.Writers.Gltf;
using SharpGLTF.Scenes;
using SharpGLTF.Schema2;

namespace VoyageModelExporter;

internal static class Program
{
    internal static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

    static int Main(string[] args)
    {
        try
        {
            if (args.Length != 1) throw new ArgumentException("Usage: VoyageModelExporter <request.json>");
            var request = JsonNode.Parse(File.ReadAllText(args[0]))!;
            string Str(string key) => request[key]!.GetValue<string>();
            var source = Str("asset");
            var materialMode = Str("materialMode");
            var geometryMode = request["geometryMode"]?.GetValue<string>() ?? "Ordinary";
            if (materialMode is not ("PbrApproximation" or "BakeReconstructed"))
                throw new ArgumentException("materialMode must be PbrApproximation or BakeReconstructed.");
            if (geometryMode is not ("Ordinary" or "Nanite"))
                throw new ArgumentException("geometryMode must be Ordinary or Nanite.");
            if (!Regex.IsMatch(source, @"^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$") || source.Trim() != source)
                throw new ArgumentException("Use one exact model or Blueprint package identity without object suffix, wildcard or traversal.");
            var output = Path.GetFullPath(Str("output"));
            if (File.Exists(output)) throw new IOException("Output exists; choose a fresh GLB path.");
            var fingerprint = JsonNode.Parse(File.ReadAllText(Str("fingerprintPath")))!;
            var mapping = JsonNode.Parse(File.ReadAllText(Str("mappingManifestPath")))!;
            var gameRoot = fingerprint["gameRoot"]!.GetValue<string>();
            var build = fingerprint["steam"]!["buildId"]!.GetValue<string>();
            if (mapping["engineVersion"]!.GetValue<string>() != "5.8" ||
                mapping["steamBuildId"]!.GetValue<string>() != build)
                throw new InvalidDataException("Game/mapping identity mismatch.");

            Directory.SetCurrentDirectory(Path.GetDirectoryName(Path.GetFullPath(args[0]))!);
            TextureDecoder.UseAssetRipperTextureDecoder = true;
            using var provider = new StockProvider(new DirectoryInfo(Path.Combine(gameRoot, "Voyage/Content/Paks")));
            provider.ReadNaniteData = geometryMode == "Nanite";
            provider.MappingsContainer = new FileUsmapTypeMappingsProvider(Str("mappingPath"));
            provider.Initialize(); provider.Mount(); provider.PostMount(); provider.LoadVirtualPaths();
            var exports = provider.LoadPackage(source).GetExports();
            var scene = new SceneBuilder();
            var materialFactory = new ModelMaterialFactory(materialMode);
            var meshRecords = new List<MeshRecord>();
            var omitted = new Dictionary<string, int>(StringComparer.Ordinal);
            var omittedInstances = new List<OmittedComponentRecord>();
            var nodeCount = 0;
            var inheritedOverrideCount = 0;
            NaniteMeshData? naniteMeshData = null;

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
            bool InheritedBool(USceneComponent component, string propertyName, bool defaultValue)
            {
                var current = component;
                while (current != null)
                {
                    if (current.TryGetValue(out bool value, propertyName)) return value;
                    current = current.Template?.Load<USceneComponent>();
                }
                return defaultValue;
            }
            bool IsVisibleByDefault(USceneComponent component) =>
                InheritedBool(component, "bVisible", true) &&
                !InheritedBool(component, "bHiddenInGame", false);
            MaterialBinding[] ResolveMaterials(UStaticMesh mesh, UStaticMeshComponent? component)
            {
                FPackageIndex? ResolveOverride(int index)
                {
                    var current = component;
                    while (current != null)
                    {
                        if (current.TryGetValue(out FPackageIndex?[] overrides, "OverrideMaterials"))
                        {
                            if (index < overrides.Length && overrides[index] is { IsNull: false } replacement)
                                return replacement;
                            return null;
                        }
                        current = current.Template?.Load<UStaticMeshComponent>();
                    }
                    return null;
                }
                var result = new MaterialBinding[mesh.StaticMaterials.Length];
                for (var i = 0; i < result.Length; i++)
                {
                    var selected = ResolveOverride(i);
                    if (selected == null && mesh.StaticMaterials[i].MaterialInterface is { IsNull: false } original)
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
                var nanite = geometryMode == "Nanite" ? ValidateNaniteDecode(mesh) : null;
                using var dto = new CUE4Parse_Conversion.Dto.StaticMeshDto(
                    mesh, EMeshQuality.Highest,
                    geometryMode == "Nanite" ? ENaniteMeshFormat.NaniteOnly : ENaniteMeshFormat.NoNanite);
                var lod = dto.LODs.FirstOrDefault() ?? throw new InvalidDataException($"No exportable mesh LOD: {mesh.GetPathName()}");
                if (geometryMode == "Nanite" && (!lod.IsNanite || nanite == null))
                    throw new InvalidDataException($"Nanite-only export unexpectedly fell back: {mesh.GetPathName()}");
                if (nanite != null)
                {
                    var triangleCount = lod.Sections.Sum(x => x.NumFaces);
                    var uvChannels = 1 + lod.ExtraUvs.Length;
                    nanite.ExactZeroAreaTriangles = CountExactZeroAreaTriangles(lod);
                    if (triangleCount != nanite.LeafTriangles || lod.Vertices.Length != nanite.LeafVertices ||
                        uvChannels != nanite.UvChannels)
                        throw new InvalidDataException($"Nanite DTO differs from the complete leaf decode: {mesh.GetPathName()}");
                }
                var includeTangents = nanite?.HasTangents ?? true;
                if (nanite != null) naniteMeshData = ModelMeshWriter.CaptureNanite(lod);
                var builder = ModelMeshWriter.Build(
                    componentName, lod, bindings.Select(x => x.Builder).ToArray(), includeTangents);
                scene.AddRigidMesh(builder, node);
                meshRecords.Add(new MeshRecord
                {
                    Component = componentName,
                    Source = ModelMaterialFactory.PackagePath(mesh.GetPathName()),
                    Geometry = lod.IsNanite ? "decoded Nanite leaf geometry" : $"ordinary LOD{lod.SourceLodIndex}",
                    Vertices = lod.Vertices.Length,
                    Triangles = lod.Sections.Sum(x => x.NumFaces),
                    Sections = lod.Sections.Length,
                    UvChannels = 1 + lod.ExtraUvs.Length,
                    HasTangents = includeTangents,
                    Materials = bindings.Select(x => x.Source).ToArray(),
                    Nanite = nanite
                });
            }
            NodeBuilder AddComponent(USceneComponent component, NodeBuilder? parent, string? componentName = null)
            {
                nodeCount++;
                var name = CleanName(componentName ?? component.Name);
                var node = ConfigureNode(parent == null ? new NodeBuilder(name) : parent.CreateNode(name), component);
                if (component is UStaticMeshComponent staticComponent)
                {
                    if (!IsVisibleByDefault(staticComponent))
                    {
                        Omit("StaticMeshComponent hidden by default", name,
                            "Effective component-template visibility excludes this mesh from the static default GLB; Blueprint runtime logic may show it.");
                    }
                    else
                    {
                        var mesh = staticComponent.GetLoadedStaticMesh();
                        if (mesh == null) Omit("StaticMeshComponent without a mesh", name, "No static mesh is assigned in the default component template.");
                        else AddMesh(node, mesh, staticComponent, name);
                    }
                }
                else if (component is not USceneComponent || component.GetType() != typeof(USceneComponent))
                {
                    Omit(component.GetType().Name, name, "Component has no supported static-mesh geometry representation in glTF.");
                }
                return node;
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
                var node = AddComponent(component, parent);
                foreach (var child in scsNode.GetChildNodes()) Visit(child, node);
            }

            var directMeshes = exports.OfType<UStaticMesh>().ToArray();
            var constructionScripts = exports.OfType<USimpleConstructionScript>().ToArray();
            if (geometryMode == "Nanite" &&
                (directMeshes.Length != 1 || constructionScripts.Length != 0))
                throw new InvalidDataException("Nanite geometry mode requires one direct StaticMesh package.");
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
                var inheritedNodes = new Dictionary<string, NodeBuilder>(StringComparer.Ordinal);
                foreach (var handler in exports.OfType<UInheritableComponentHandler>())
                {
                    foreach (var record in handler.GetRecords())
                    {
                        var variableName = record.ComponentKey.SCSVariableName.Text;
                        var component = record.ComponentTemplate?.Load<USceneComponent>();
                        if (component == null)
                        {
                            Omit("Unresolved inherited component override", variableName,
                                "The inherited component override template could not be loaded as a scene component.");
                            continue;
                        }
                        inheritedNodes[variableName] = AddComponent(component, null, variableName);
                        inheritedOverrideCount++;
                    }
                }
                foreach (var rootNode in constructionScripts[0].GetRootNodes())
                {
                    NodeBuilder? parent = null;
                    if (rootNode.TryGetValue(out FName parentVariableName, "ParentComponentOrVariableName"))
                        inheritedNodes.TryGetValue(parentVariableName.Text, out parent);
                    Visit(rootNode, parent);
                }
            }
            else
            {
                throw new InvalidDataException($"Expected one StaticMesh or one Blueprint SimpleConstructionScript: {source}");
            }
            if (meshRecords.Count == 0) throw new InvalidDataException("The selected asset produced no static mesh components.");

            var model = scene.ToGltf2();
            if (naniteMeshData != null) ModelMeshWriter.ApplyNaniteAccessors(model, naniteMeshData);
            materialFactory.MatchUsedImages(model);
            materialFactory.EmbedSourceArtifacts(model);
            var materialPipeline = new
            {
                schema = "voyage.material-pipeline/1",
                requestedMode = materialMode,
                fidelity = materialMode == "BakeReconstructed" ? "reconstructed" : "approximate-pbr",
                bakeOperations = materialFactory.Materials.SelectMany(x => x.BakeOperations),
                generatedImages = materialFactory.Textures.SelectMany(x => x.Variants.Select(v => new
                {
                    source = x.Source,
                    v.Role,
                    v.Transform,
                    v.Sha256,
                    v.ImageIndex
                })),
                sourceArtifacts = materialFactory.SourceArtifacts,
                unresolvedLayers = materialFactory.Materials.SelectMany(x => x.SkippedTextures.Select(layer => new
                {
                    material = x.Source,
                    texture = layer.Key,
                    parameters = x.TextureParameters.Where(p => p.Value == layer.Key).Select(p => p.Key),
                    reason = layer.Value
                }))
            };
            var evidence = new
            {
                schemaVersion = 2,
                status = materialFactory.Textures.Any(x => x.Error != null) ? "partial-textures" :
                    materialMode == "BakeReconstructed" ? "reconstructed-bake" : "approximate",
                source,
                sourceKind,
                materialMode,
                geometryMode,
                steamBuildId = build,
                mappingPath = Path.GetFullPath(Str("mappingPath")),
                nodeCount,
                inheritedOverrideCount,
                meshComponents = meshRecords,
                omittedComponents = omitted.OrderBy(x => x.Key).ToDictionary(x => x.Key, x => x.Value),
                omittedComponentInstances = omittedInstances,
                materials = materialFactory.Materials,
                textures = materialFactory.Textures,
                materialPipeline,
                limitations = new[]
                {
                    "Static default representation only: Blueprint construction scripts, runtime spawning, state changes and animations are not executed.",
                    "Only StaticMeshComponent geometry is exported; skeletal meshes, splines, Niagara, decals, widgets, lights, audio, collisions and child actors are reported or omitted.",
                    geometryMode == "Nanite"
                        ? "Nanite mode exports complete decoded leaf geometry only and fails instead of falling back to an ordinary LOD."
                        : "Ordinary mode exports the highest ordinary render LOD; high-resolution Nanite reconstruction is not promised.",
                    materialMode == "BakeReconstructed"
                        ? "Material baking is reconstructed from cooked parameters and known recipes; the stripped Unreal expression graph is not executed."
                        : "Materials are bounded glTF PBR approximations; layered shader inputs are not baked."
                }
            };
            model.Extras = JsonSerializer.SerializeToNode(evidence, JsonOptions);
            var pending = Path.Combine(Directory.GetCurrentDirectory(), "pending.glb");
            model.SaveGLB(pending);
            var readback = ModelRoot.Load(pending);
            if (readback.LogicalMeshes.Count != meshRecords.Count || readback.LogicalMaterials.Count > materialFactory.Materials.Count)
                throw new InvalidDataException("GLB mesh/material readback mismatch.");
            var glbGeometry = ReadGlbGeometry(pending);
            if (geometryMode == "Nanite" && glbGeometry.Triangles != meshRecords.Sum(x => x.Triangles))
                throw new InvalidDataException("GLB primitive triangle count differs from the source DTO.");
            if (geometryMode == "Nanite" &&
                glbGeometry.HasTangents != meshRecords.Single().Nanite!.HasTangents)
                throw new InvalidDataException("GLB tangent attribute differs from the decoded Nanite source.");
            Directory.CreateDirectory(Path.GetDirectoryName(output)!);
            File.Move(pending, output, false);
            var reportPath = Path.Combine(Directory.GetCurrentDirectory(), "export-report.json");
            File.WriteAllText(reportPath, JsonSerializer.Serialize(evidence, JsonOptions));
            var omissionsPath = Path.Combine(Directory.GetCurrentDirectory(), "material-omissions.md");
            File.WriteAllText(omissionsPath, MaterialOmissions(materialFactory.Materials, materialMode));
            Console.WriteLine(JsonSerializer.Serialize(new
            {
                status = evidence.status,
                glbPath = output,
                bytes = new FileInfo(output).Length,
                sourceKind,
                materialMode,
                geometryMode,
                geometry = glbGeometry,
                nodeCount,
                meshCount = meshRecords.Count,
                uniqueMeshCount = meshRecords.Select(x => x.Source).Distinct(StringComparer.Ordinal).Count(),
                materialCount = readback.LogicalMaterials.Count,
                discoveredMaterialRecords = materialFactory.Materials.Count,
                imageCount = readback.LogicalImages.Count,
                failedTextures = materialFactory.Textures.Count(x => x.Error != null),
                sourceArtifactCount = materialFactory.SourceArtifacts.Count,
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

    static NaniteDecodeRecord ValidateNaniteDecode(UStaticMesh mesh)
    {
        var nanite = mesh.RenderData?.NaniteResources ??
            throw new InvalidDataException($"Nanite resources are absent: {mesh.GetPathName()}");
        nanite.LoadAllPages();
        var pageCount = nanite.PageStreamingStates.Length;
        var loadedPages = nanite.LoadedPages.Count(page => page != null);
        var clusters = nanite.LoadedPages
            .Where(page => page != null)
            .SelectMany(page => page!.Clusters)
            .ToArray();
        var leaves = clusters.Where(cluster => cluster.EdgeLength < 0).ToArray();
        var leafTriangles = leaves.Sum(cluster => cluster.NumTris);
        var leafVertices = leaves.Sum(cluster => cluster.NumVerts);
        var missingVertices = leaves.Sum(cluster =>
            cluster.Vertices.Count(vertex => vertex?.Attributes == null));
        var uvCounts = leaves.Select(cluster => cluster.NumUVs).Distinct().ToArray();
        var tangentStates = leaves.Select(cluster => cluster.bHasTangents).Distinct().ToArray();
        if (pageCount == 0 || loadedPages != pageCount || clusters.Length != nanite.NumClusters ||
            leaves.Length == 0 || leafTriangles != nanite.NumInputTriangles ||
            missingVertices != 0 || uvCounts.Length != 1 || uvCounts[0] <= 0 ||
            tangentStates.Length != 1)
            throw new InvalidDataException($"Incomplete or inconsistent Nanite leaf decode: {mesh.GetPathName()}");
        if (tangentStates[0])
            throw new InvalidDataException($"Nanite sources with explicit tangents are outside this bounded export mode: {mesh.GetPathName()}");
        var attributes = new List<string> { "POSITION", "NORMAL" };
        if (tangentStates[0]) attributes.Add("TANGENT");
        for (var index = 0; index < uvCounts[0]; index++) attributes.Add($"TEXCOORD_{index}");
        return new NaniteDecodeRecord
        {
            InputTriangles = checked((int)nanite.NumInputTriangles),
            InputVertices = checked((int)nanite.NumInputVertices),
            TotalClusters = checked((int)nanite.NumClusters),
            RootPages = checked((int)nanite.NumRootPages),
            Pages = pageCount,
            LoadedPages = loadedPages,
            DecodedClusters = clusters.Length,
            LeafClusters = leaves.Length,
            LeafTriangles = checked((int)leafTriangles),
            LeafVertices = checked((int)leafVertices),
            MissingVertices = missingVertices,
            UvChannels = checked((int)uvCounts[0]),
            HasTangents = tangentStates[0],
            Attributes = attributes.ToArray(),
            PageInfo = nanite.LoadedPages.Select((page, index) => new NanitePageRecord
            {
                Index = index,
                Clusters = page?.Clusters.Length ?? 0,
                LeafTriangles = checked((int)(page?.Clusters.Where(cluster => cluster.EdgeLength < 0)
                    .Sum(cluster => (long)cluster.NumTris) ?? 0L))
            }).ToArray()
        };
    }

    static int CountExactZeroAreaTriangles(MeshLodDto<MeshVertex> lod)
    {
        var count = 0;
        for (var index = 0; index < lod.Indices.Length; index += 3)
        {
            var a = lod.Vertices[checked((int)lod.Indices[index])].Position;
            var b = lod.Vertices[checked((int)lod.Indices[index + 1])].Position;
            var c = lod.Vertices[checked((int)lod.Indices[index + 2])].Position;
            var abx = (double)b.X - a.X; var aby = (double)b.Y - a.Y; var abz = (double)b.Z - a.Z;
            var acx = (double)c.X - a.X; var acy = (double)c.Y - a.Y; var acz = (double)c.Z - a.Z;
            var x = aby * acz - abz * acy;
            var y = abz * acx - abx * acz;
            var z = abx * acy - aby * acx;
            if (x * x + y * y + z * z == 0d) count++;
        }
        return count;
    }

    static GlbGeometryRecord ReadGlbGeometry(string path)
    {
        using var stream = File.OpenRead(path);
        using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);
        if (reader.ReadUInt32() != 0x46546C67 || reader.ReadUInt32() != 2 || reader.ReadUInt32() != stream.Length)
            throw new InvalidDataException("Invalid GLB header.");
        var jsonLength = reader.ReadUInt32();
        if (reader.ReadUInt32() != 0x4E4F534A)
            throw new InvalidDataException("GLB JSON chunk is absent.");
        using var document = JsonDocument.Parse(reader.ReadBytes(checked((int)jsonLength)));
        var root = document.RootElement;
        var accessors = root.GetProperty("accessors");
        var triangles = 0;
        var primitives = 0;
        var hasTangents = false;
        foreach (var mesh in root.GetProperty("meshes").EnumerateArray())
        foreach (var primitive in mesh.GetProperty("primitives").EnumerateArray())
        {
            if (primitive.TryGetProperty("mode", out var mode) && mode.GetInt32() != 4)
                throw new InvalidDataException("GLB contains a non-triangle primitive.");
            var accessor = accessors[primitive.GetProperty("indices").GetInt32()];
            var indexCount = accessor.GetProperty("count").GetInt32();
            if (indexCount % 3 != 0) throw new InvalidDataException("GLB index count is not divisible by three.");
            triangles += indexCount / 3;
            primitives++;
            hasTangents |= primitive.GetProperty("attributes").TryGetProperty("TANGENT", out _);
        }
        return new GlbGeometryRecord { Primitives = primitives, Triangles = triangles, HasTangents = hasTangents };
    }

    static string MaterialOmissions(IEnumerable<MaterialRecord> materials, string materialMode)
    {
        var result = new StringBuilder();
        result.AppendLine("# Material effects and omissions").AppendLine();
        result.Append("Material mode: `").Append(materialMode).AppendLine("`").AppendLine();
        result.AppendLine(materialMode == "BakeReconstructed"
            ? "Raw decodable Texture2D inputs are embedded in GLB images and indexed by extras.materialPipeline.sourceArtifacts."
            : "Paths identify cooked source textures; unresolved image payloads are not copied in this mode.").AppendLine();
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
    public int UvChannels { get; set; }
    public bool HasTangents { get; set; }
    public string?[] Materials { get; set; } = [];
    public NaniteDecodeRecord? Nanite { get; set; }
}

internal sealed class NaniteDecodeRecord
{
    public int InputTriangles { get; set; }
    public int InputVertices { get; set; }
    public int TotalClusters { get; set; }
    public int RootPages { get; set; }
    public int Pages { get; set; }
    public int LoadedPages { get; set; }
    public int DecodedClusters { get; set; }
    public int LeafClusters { get; set; }
    public int LeafTriangles { get; set; }
    public int LeafVertices { get; set; }
    public int MissingVertices { get; set; }
    public int ExactZeroAreaTriangles { get; set; }
    public int UvChannels { get; set; }
    public bool HasTangents { get; set; }
    public string[] Attributes { get; set; } = [];
    public NanitePageRecord[] PageInfo { get; set; } = [];
}

internal sealed class NanitePageRecord
{
    public int Index { get; set; }
    public int Clusters { get; set; }
    public int LeafTriangles { get; set; }
}

internal sealed class GlbGeometryRecord
{
    public int Primitives { get; set; }
    public int Triangles { get; set; }
    public bool HasTangents { get; set; }
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

using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using CUE4Parse.FileProvider;
using CUE4Parse.FileProvider.Objects;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Assets.Exports.StaticMesh;
using CUE4Parse.UE4.IO.Objects;
using CUE4Parse.UE4.Versions;
using CUE4Parse.UE4.VirtualFileSystem;
using CUE4Parse_Conversion.Dto;
using CUE4Parse_Conversion.Options;
using Serilog;

namespace VoyageStaticMeshValidator;

internal static class Program
{
    internal const string CueCommit = "c38cf24676514773ac106d7b51640f09f4590c61";
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow
    };

    public static int Main(string[] args)
    {
        try
        {
            if (args.Length == 1 && args[0] == "--self-test")
            {
                var render = typeof(FStaticMeshRenderData);
                var proxy = typeof(FStaticMeshRayTracingProxy);
                if (render.GetField("bHasRayTracingProxy") is null || render.GetField("RayTracingProxy") is null || proxy.GetField("bUsingRenderingLODs") is null)
                    throw new InvalidDataException("Private CUE4Parse retained-field runtime smoke check failed.");
                Console.WriteLine("{\"status\":\"passed\",\"retainedRayTracingFields\":3}");
                return 0;
            }
            if (args.Length != 1) throw new RequestException("Usage: VoyageStaticMeshValidator <normalized-request.json>");
            var requestPath = Path.GetFullPath(args[0]);
            var request = JsonSerializer.Deserialize<Request>(File.ReadAllText(requestPath), JsonOptions)
                ?? throw new RequestException("Request is empty.");
            ValidateRequest(request);
            return Run(request, requestPath);
        }
        catch (RequestException exception)
        {
            Console.Error.WriteLine(exception.Message);
            return 2;
        }
        catch (JsonException exception)
        {
            Console.Error.WriteLine($"Invalid request JSON: {exception.Message}");
            return 2;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"Validator setup failed: {exception.GetType().Name}: {exception.Message}");
            return 2;
        }
    }

    private static int Run(Request request, string requestPath)
    {
        var output = Path.GetFullPath(request.OutputDirectory);
        if (Directory.Exists(output) || File.Exists(output))
            throw new RequestException("Output directory already exists; choose a fresh path.");
        Directory.CreateDirectory(output);
        Serilog.Log.Logger = new LoggerConfiguration().WriteTo.Console().CreateLogger();

        var candidate = request.Source.Kind == "utoc" ? new FileInfo(Path.GetFullPath(request.Source.Utoc!)) : null;
        using var provider = new SourceProvider(new DirectoryInfo(Path.Combine(request.GameRoot, "Voyage/Content/Paks")), candidate);
        provider.ReadNaniteData = request.Assets.Any(x => x.DecodeNanite || x.SourceGlb is not null);
        provider.MappingsContainer = new FileUsmapTypeMappingsProvider(request.MappingPath);
        provider.Initialize(); provider.Mount(); provider.PostMount(); provider.LoadVirtualPaths();

        var source = SourceFacts(request.Source, candidate);
        var assetResults = new JsonArray();
        var validations = new JsonArray();
        var assetFailures = false;
        var policyFailures = false;
        foreach (var item in request.Assets)
        {
            JsonObject result;
            try
            {
                result = Inspect(provider, request, item, candidate);
            }
            catch (Exception exception)
            {
                assetFailures = true;
                result = new JsonObject
                {
                    ["id"] = item.Id, ["package"] = item.Package, ["status"] = Classify(exception),
                    ["error"] = new JsonObject { ["type"] = exception.GetType().Name, ["message"] = exception.Message }
                };
            }
            assetResults.Add(result);
            var evaluated = Evaluate(item, result);
            validations.Add(evaluated);
            if (evaluated["status"]?.GetValue<string>() == "failed") policyFailures = true;
        }

        var provenance = new JsonObject
        {
            ["requestSha256"] = Hash(requestPath),
            ["cue4ParseSourceCommit"] = CueCommit,
            ["gameRoot"] = Path.GetFullPath(request.GameRoot),
            ["fingerprintPath"] = Path.GetFullPath(request.FingerprintPath),
            ["fingerprintSha256"] = Hash(request.FingerprintPath),
            ["mappingPath"] = Path.GetFullPath(request.MappingPath),
            ["mappingSha256"] = Hash(request.MappingPath),
            ["mappingManifestPath"] = Path.GetFullPath(request.MappingManifestPath),
            ["mappingManifestSha256"] = Hash(request.MappingManifestPath),
            ["source"] = source
        };
        var inspection = new JsonObject
        {
            ["schemaVersion"] = 1, ["kind"] = "VoyageStaticMeshInspection", ["provenance"] = provenance,
            ["assets"] = assetResults,
            ["claims"] = new JsonArray("Cooked serialization and decoded geometry facts only.",
                "Material, shader, collision, runtime rendering and gameplay parity are not established.")
        };
        var policyRequested = request.Assets.Any(x => x.Expectations.Count > 0 || x.SourceGlb?.Topology is not null || x.SourceGlb?.Attributes.Count > 0);
        var validation = new JsonObject
        {
            ["schemaVersion"] = 1, ["kind"] = "VoyageStaticMeshValidation",
            ["status"] = assetFailures ? "asset-error" : policyFailures ? "failed" : policyRequested ? "passed" : "not-requested",
            ["assets"] = validations
        };
        WriteJson(Path.Combine(output, "inspection.json"), inspection);
        WriteJson(Path.Combine(output, "validation.json"), validation);
        var exitCode = assetFailures ? 3 : policyFailures ? 4 : 0;
        Console.WriteLine(JsonSerializer.Serialize(new
        {
            status = exitCode == 0 ? "passed" : exitCode == 3 ? "asset-error" : "policy-failed",
            inspectionPath = Path.Combine(output, "inspection.json"), validationPath = Path.Combine(output, "validation.json"), exitCode
        }));
        return exitCode;
    }

    private static JsonObject Inspect(SourceProvider provider, Request request, AssetRequest item, FileInfo? candidate)
    {
        if (!provider.TryGetGameFile(item.Package, out var file))
            throw new FileNotFoundException($"Package is not present in mounted sources: {item.Package}");
        var selectedOwner = Owner(file);
        var owners = provider.AllOwners(file.Path);
        if (candidate is not null && !SamePath(selectedOwner, candidate.FullName))
            throw new WrongSourceException($"Package resolved from '{selectedOwner}', not requested candidate '{candidate.FullName}'. Candidate does not own this asset.");

        var exports = provider.LoadPackage(item.Package).GetExports().OfType<UStaticMesh>().ToArray();
        if (exports.Length == 0) throw new UnsupportedException($"Package contains no StaticMesh export: {item.Package}");
        if (exports.Length != 1) throw new UnsupportedException($"Package contains {exports.Length} StaticMesh exports; exact export selection is not supported in schema v1.");
        var mesh = exports[0];
        var render = mesh.RenderData ?? throw new UnsupportedException("StaticMesh RenderData is absent.");
        var lods = new JsonArray();
        for (var i = 0; i < render.LODs.Length; i++)
        {
            var lod = render.LODs[i];
            lods.Add(new JsonObject
            {
                ["index"] = i, ["sections"] = lod.Sections.Length,
                ["triangles"] = lod.Sections.Sum(x => (long)x.NumTriangles),
                ["vertices"] = lod.PositionVertexBuffer?.NumVertices,
                ["decodedPositions"] = lod.PositionVertexBuffer?.Verts.Length,
                ["uvChannels"] = lod.VertexBuffer?.NumTexCoords,
                ["hasVertexColors"] = (lod.ColorVertexBuffer?.NumVertices ?? 0) > 0,
                ["skipLod"] = lod.SkipLod
            });
        }
        var nanite = render.NaniteResources;
        var usableNanite = nanite is not null && nanite.NumRootPages > 0 && nanite.PageStreamingStates.Length > 0 && nanite.NumClusters > 0;
        var naniteFacts = nanite is null ? new JsonObject { ["resourceRecordPresent"] = false, ["present"] = false, ["decodeStatus"] = "unavailable" } : new JsonObject
        {
            ["resourceRecordPresent"] = true, ["present"] = usableNanite, ["rootPages"] = nanite.NumRootPages, ["pages"] = nanite.PageStreamingStates.Length,
            ["clusters"] = nanite.NumClusters, ["inputTriangles"] = nanite.NumInputTriangles,
            ["inputVertices"] = nanite.NumInputVertices, ["positionPrecision"] = nanite.PositionPrecision,
            ["normalPrecision"] = nanite.NormalPrecision, ["streamablePagesSizeOnDisk"] = nanite.StreamablePages?.Header.SizeOnDisk ?? 0,
            ["decodeStatus"] = item.DecodeNanite || item.SourceGlb is not null ? "requested" : "not-requested"
        };
        MeshLodDto<MeshVertex>? decoded = null;
        if (item.DecodeNanite || item.SourceGlb is not null)
        {
            if (!usableNanite || nanite is null) throw new UnsupportedException("Nanite decode was requested but usable Nanite root/page/cluster resources are absent.");
            nanite.LoadAllPages();
            var clusters = nanite.LoadedPages.Where(x => x is not null).SelectMany(x => x!.Clusters).ToArray();
            var leaves = clusters.Where(x => x.EdgeLength < 0).ToArray();
            var loadedPages = nanite.LoadedPages.Count(x => x is not null);
            var pageSlots = nanite.LoadedPages.Length;
            var leafTriangles = leaves.Sum(x => (long)x.NumTris);
            if (nanite.LoadedPages.Any(x => x is null) || clusters.Length != nanite.NumClusters)
                throw new UnsupportedException($"Nanite page decode is incomplete: loaded {loadedPages}/{pageSlots} pages and {clusters.Length}/{nanite.NumClusters} clusters.");
            using var dto = new StaticMeshDto(mesh, EMeshQuality.Highest, ENaniteMeshFormat.NaniteOnly);
            decoded = dto.LODs.SingleOrDefault() ?? throw new UnsupportedException("Nanite DTO produced no LOD.");
            if (!decoded.IsNanite) throw new UnsupportedException("Nanite-only DTO unexpectedly returned ordinary geometry.");
            if (decoded.Indices.Length / 3 != leafTriangles) throw new UnsupportedException($"Nanite DTO triangle count {decoded.Indices.Length / 3} differs from complete leaf decode {leafTriangles}.");
            naniteFacts["decodeStatus"] = "decoded";
            naniteFacts["loadedPages"] = loadedPages;
            naniteFacts["decodedClusters"] = clusters.Length;
            naniteFacts["leafClusters"] = leaves.Length;
            naniteFacts["leafTriangles"] = leafTriangles;
            naniteFacts["decodedTriangles"] = decoded.Indices.Length / 3;
            naniteFacts["decodedVertices"] = decoded.Vertices.Length;
            naniteFacts["decodedSections"] = decoded.Sections.Length;
            naniteFacts["uvChannels"] = 1 + decoded.ExtraUvs.Length;
            naniteFacts["hasVertexColors"] = decoded.VertexColors is { Length: > 0 };
            naniteFacts["hasTangents"] = decoded.Vertices.Any(v => v.Tangent.X != 0 || v.Tangent.Y != 0 || v.Tangent.Z != 0);
            naniteFacts["exactZeroAreaTriangles"] = CountZeroArea(decoded);
        }
        var renderType = render.GetType();
        var hasProxyFlag = (bool?)renderType.GetField("bHasRayTracingProxy")?.GetValue(render)
            ?? throw new UnsupportedException("Private parser did not retain bHasRayTracingProxy; validator bundle provenance is invalid.");
        var proxy = renderType.GetField("RayTracingProxy")?.GetValue(render);
        var proxyLodValues = proxy?.GetType().GetField("LODs")?.GetValue(proxy) as Array;
        var proxyLods = new JsonArray();
        foreach (var pair in (proxyLodValues?.Cast<FStaticMeshRayTracingProxyLOD>() ?? []).Select((value, index) => (value, index)))
            proxyLods.Add(new JsonObject
            {
                ["index"] = pair.index, ["ownsBuffers"] = pair.value.bOwnsBuffers,
                ["ownsRayTracingGeometry"] = pair.value.bOwnsRayTracingGeometry,
                ["buffersInlined"] = pair.value.bBuffersInlined,
                ["ownedSections"] = pair.value.Sections?.Length ?? 0,
                ["streamableDataSizeOnDisk"] = pair.value.StreamableData?.Header.SizeOnDisk ?? 0
            });
        var facts = new JsonObject
        {
            ["source"] = new JsonObject { ["selectedOwner"] = selectedOwner, ["allOwners"] = new JsonArray(owners.Select(x => JsonValue.Create(x)).ToArray()) },
            ["exportName"] = mesh.Name, ["renderingLods"] = lods, ["nanite"] = naniteFacts,
            ["rayTracing"] = new JsonObject
            {
                ["hasProxyFlag"] = hasProxyFlag, ["proxyPresent"] = proxy is not null,
                ["usingRenderingLods"] = proxy is null ? null : (bool?)proxy.GetType().GetField("bUsingRenderingLODs")?.GetValue(proxy),
                ["proxyLodCount"] = proxyLodValues?.Length ?? 0, ["proxyLods"] = proxyLods
            }
        };
        if (item.SourceGlb is not null)
        {
            if (decoded is null) throw new InvalidOperationException("Internal decode state missing.");
            facts["sourceComparison"] = CompareGlb(item.SourceGlb, decoded);
        }
        return new JsonObject { ["id"] = item.Id, ["package"] = item.Package, ["status"] = "inspected", ["facts"] = facts };
    }

    private static JsonObject CompareGlb(SourceGlbRequest request, MeshLodDto<MeshVertex> target)
    {
        if (request.Attributes.Count == 0) throw new RequestException("sourceGlb.attributes must explicitly select at least one attribute.");
        if (!request.Attributes.ContainsKey("POSITION")) throw new RequestException("sourceGlb.attributes must include POSITION for vertex correspondence and topology.");
        foreach (var name in request.Attributes.Keys)
            if (!Regex.IsMatch(name, @"^(POSITION|NORMAL|COLOR_0|TEXCOORD_[0-9]+)$"))
                throw new UnsupportedException($"Unsupported comparison attribute '{name}'. Supported: POSITION, NORMAL, COLOR_0, TEXCOORD_n.");
        if (request.PositionMatchToleranceMeters <= 0) throw new RequestException("sourceGlb.positionMatchToleranceMeters must be positive.");
        var source = Glb.Read(Path.GetFullPath(request.Path), request.MeshIndex, request.MeshName, request.Attributes.Keys.ToArray());
        var decoded = DecodedRows(target, request.Attributes.Keys.ToArray());
        var mapping = Match(source.Rows, decoded, request.PositionMatchToleranceMeters, request.Attributes);
        var sourceFaces = FaceCounts(source.Triangles, source.Rows, null);
        var targetFaces = FaceCounts(target.Indices.Chunk(3).Select(x => x.Select(y => checked((int)y)).ToArray()).ToArray(), source.Rows, mapping);
        var extra = DifferenceCount(targetFaces, sourceFaces);
        var missing = MissingFaces(source.Triangles, source.Rows, sourceFaces, targetFaces);
        var nondegenerate = missing.Count(x => TriangleArea(source.Rows, x) != 0d);
        var maxArea = missing.Select(x => TriangleArea(source.Rows, x)).DefaultIfEmpty(0d).Max();
        return new JsonObject
        {
            ["status"] = "compared", ["sourceGlb"] = Path.GetFullPath(request.Path), ["sourceGlbSha256"] = Hash(request.Path),
            ["coordinateConvention"] = "decoded UE centimeters (X,Y,Z) -> glTF meters (X,Z,Y); normals (X,Y,Z) -> (X,Z,Y); UV/color unchanged",
            ["meshIndex"] = source.MeshIndex, ["meshName"] = source.MeshName, ["attributes"] = new JsonArray(request.Attributes.Keys.Select(x => JsonValue.Create(x)).ToArray()),
            ["sourceTriangles"] = source.Triangles.Length, ["decodedTriangles"] = target.Indices.Length / 3,
            ["extraFaces"] = extra, ["missingFaces"] = missing.Count, ["missingNondegenerateFaces"] = nondegenerate,
            ["maxMissingTriangleAreaSquareMeters"] = maxArea,
            ["maxAbsoluteError"] = JsonSerializer.SerializeToNode(mapping.MaxErrors, JsonOptions),
            ["positionMatchToleranceMeters"] = request.PositionMatchToleranceMeters
        };
    }

    private static MatchResult Match(double[][] source, double[][] target, double cell, Dictionary<string, double> attributes)
    {
        var buckets = new Dictionary<(long, long, long), List<int>>();
        for (var i = 0; i < source.Length; i++)
        {
            var key = Cell(source[i], cell);
            if (!buckets.TryGetValue(key, out var values)) buckets[key] = values = [];
            values.Add(i);
        }
        var mapped = new int[target.Length];
        var max = attributes.Keys.ToDictionary(x => x, _ => 0d);
        for (var i = 0; i < target.Length; i++)
        {
            var key = Cell(target[i], cell);
            var candidates = new List<int>();
            for (var x = -1; x <= 1; x++) for (var y = -1; y <= 1; y++) for (var z = -1; z <= 1; z++)
                if (buckets.TryGetValue((key.Item1 + x, key.Item2 + y, key.Item3 + z), out var found)) candidates.AddRange(found);
            candidates = candidates.Where(candidate => Enumerable.Range(0, 3).Max(axis => Math.Abs(source[candidate][axis] - target[i][axis])) <= cell).ToList();
            if (candidates.Count == 0) throw new UnsupportedException($"No source position candidate for decoded vertex {i} within spatial search.");
            var best = -1; var bestScore = double.PositiveInfinity; Dictionary<string, double>? bestErrors = null;
            foreach (var candidate in candidates)
            {
                var errors = AttributeErrors(source[candidate], target[i], attributes.Keys);
                var score = errors.Sum(x => x.Value / Math.Max(attributes[x.Key], 1e-30));
                if (score < bestScore) { best = candidate; bestScore = score; bestErrors = errors; }
            }
            foreach (var pair in bestErrors!)
            {
                max[pair.Key] = Math.Max(max[pair.Key], pair.Value);
            }
            mapped[i] = best;
        }
        return new MatchResult(mapped, max);
    }

    private static Dictionary<string, double> AttributeErrors(double[] a, double[] b, IEnumerable<string> names)
    {
        var result = new Dictionary<string, double>();
        foreach (var name in names)
        {
            var (offset, width) = Layout(name);
            var error = 0d;
            for (var i = 0; i < width; i++) error = Math.Max(error, Math.Abs(a[offset + i] - b[offset + i]));
            result[name] = error;
        }
        return result;
    }

    private static double[][] DecodedRows(MeshLodDto<MeshVertex> lod, string[] attrs)
    {
        var rows = new double[lod.Vertices.Length][];
        var colors = lod.VertexColors is { Length: > 0 } ? lod.VertexColors[0].Colors : null;
        foreach (var attr in attrs)
        {
            if (attr == "COLOR_0" && colors is null) throw new UnsupportedException("Decoded mesh has no COLOR_0.");
            if (attr.StartsWith("TEXCOORD_") && int.Parse(attr[9..]) >= 1 + lod.ExtraUvs.Length)
                throw new UnsupportedException($"Decoded mesh has no {attr}.");
        }
        for (var i = 0; i < rows.Length; i++)
        {
            var v = lod.Vertices[i];
            var row = new double[LayoutWidth(attrs)];
            Set(row, "POSITION", [v.Position.X * .01, v.Position.Z * .01, v.Position.Y * .01], attrs);
            var length = Math.Sqrt(v.Normal.X * v.Normal.X + v.Normal.Y * v.Normal.Y + v.Normal.Z * v.Normal.Z);
            if (!double.IsFinite(length) || length == 0) throw new UnsupportedException($"Decoded vertex {i} has a zero or non-finite normal.");
            Set(row, "NORMAL", [v.Normal.X / length, v.Normal.Z / length, v.Normal.Y / length], attrs);
            Set(row, "TEXCOORD_0", [v.Uv.U, v.Uv.V], attrs);
            for (var uv = 0; uv < lod.ExtraUvs.Length; uv++) Set(row, $"TEXCOORD_{uv + 1}", [lod.ExtraUvs[uv][i].U, lod.ExtraUvs[uv][i].V], attrs);
            if (colors is not null) Set(row, "COLOR_0", [colors[i].R / 255d, colors[i].G / 255d, colors[i].B / 255d, colors[i].A / 255d], attrs);
            if (row.Any(x => !double.IsFinite(x))) throw new UnsupportedException($"Decoded vertex {i} contains a non-finite selected attribute.");
            rows[i] = row;
        }
        return rows;
    }

    private static JsonObject Evaluate(AssetRequest request, JsonObject result)
    {
        var checks = new JsonArray(); var failed = false;
        if (result["status"]?.GetValue<string>() == "inspected")
        {
            foreach (var expectation in request.Expectations)
            {
                JsonObject check;
                try { check = EvaluateOne((JsonObject)result["facts"]!, expectation); }
                catch (RequestException exception) { check = new JsonObject { ["fact"] = expectation.Fact, ["passed"] = false, ["error"] = exception.Message }; }
                checks.Add(check); failed |= check["passed"]?.GetValue<bool>() == false;
            }
            if (request.SourceGlb?.Topology is { } topology)
            {
                var comparison = (JsonObject)((JsonObject)result["facts"]!)["sourceComparison"]!;
                AddLimit(checks, ref failed, "/sourceComparison/extraFaces", comparison["extraFaces"]!, topology.MaxExtraFaces);
                AddLimit(checks, ref failed, "/sourceComparison/missingNondegenerateFaces", comparison["missingNondegenerateFaces"]!, topology.MaxMissingNondegenerateFaces);
                if (topology.MaxMissingFaces is not null) AddLimit(checks, ref failed, "/sourceComparison/missingFaces", comparison["missingFaces"]!, topology.MaxMissingFaces.Value);
            }
            if (request.SourceGlb is { } comparisonRequest)
            {
                var comparison = (JsonObject)((JsonObject)result["facts"]!)["sourceComparison"]!;
                var errors = (JsonObject)comparison["maxAbsoluteError"]!;
                foreach (var pair in comparisonRequest.Attributes)
                {
                    var actual = errors[pair.Key] ?? throw new InvalidDataException($"Comparison error missing for {pair.Key}.");
                    var pass = actual.GetValue<double>() <= pair.Value; failed |= !pass;
                    checks.Add(new JsonObject { ["fact"] = $"/sourceComparison/maxAbsoluteError/{pair.Key}", ["operation"] = "max", ["expected"] = pair.Value, ["actual"] = actual.DeepClone(), ["passed"] = pass });
                }
            }
        }
        return new JsonObject
        {
            ["id"] = request.Id,
            ["status"] = result["status"]?.GetValue<string>() != "inspected" ? "not-evaluated" : failed ? "failed" : checks.Count == 0 ? "not-requested" : "passed",
            ["checks"] = checks
        };
    }

    private static JsonObject EvaluateOne(JsonObject facts, Expectation expectation)
    {
        var modes = (expectation.Equals is not null ? 1 : 0) + (expectation.Min is not null ? 1 : 0) + (expectation.Max is not null ? 1 : 0) + (expectation.Present is not null ? 1 : 0);
        if (modes != 1) throw new RequestException($"Expectation '{expectation.Fact}' must specify exactly one of equals, min, max or present. Use present for explicit null/missing checks.");
        if (expectation.Present is not null)
        {
            var exists = TryResolve(facts, expectation.Fact, out var presentNode);
            var pass = exists == expectation.Present.Value;
            return new JsonObject { ["fact"] = expectation.Fact, ["operation"] = "present", ["expected"] = expectation.Present.Value, ["actual"] = exists, ["value"] = presentNode?.DeepClone(), ["passed"] = pass };
        }
        var actual = Resolve(facts, expectation.Fact);
        bool passed; string operation; JsonNode? expected;
        if (expectation.Equals is not null) { operation = "equals"; expected = expectation.Equals; passed = JsonNode.DeepEquals(actual, expected); }
        else
        {
            if (actual is null || !double.TryParse(actual.ToJsonString(), out var number)) throw new RequestException($"Expectation '{expectation.Fact}' is not numeric.");
            if (expectation.Min is not null) { operation = "min"; expected = expectation.Min.Value; passed = number >= expectation.Min.Value; }
            else { operation = "max"; expected = expectation.Max!.Value; passed = number <= expectation.Max.Value; }
        }
        return new JsonObject { ["fact"] = expectation.Fact, ["operation"] = operation, ["expected"] = expected?.DeepClone(), ["actual"] = actual?.DeepClone(), ["passed"] = passed };
    }

    private static void AddLimit(JsonArray checks, ref bool failed, string fact, JsonNode actual, long maximum)
    {
        if (!double.TryParse(actual.ToJsonString(), System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var value))
            throw new InvalidDataException($"Topology fact is not numeric: {fact}");
        var pass = value <= maximum; failed |= !pass;
        checks.Add(new JsonObject { ["fact"] = fact, ["operation"] = "max", ["expected"] = maximum, ["actual"] = actual.DeepClone(), ["passed"] = pass });
    }

    private static JsonNode? Resolve(JsonNode root, string pointer)
    {
        if (!TryResolve(root, pointer, out var result)) throw new RequestException($"Fact pointer does not exist: {pointer}");
        return result;
    }
    private static bool TryResolve(JsonNode root, string pointer, out JsonNode? result)
    {
        if (!pointer.StartsWith('/')) throw new RequestException($"Fact must be a JSON pointer beginning with '/': {pointer}");
        JsonNode? node = root;
        var tokens = pointer.Split('/').Skip(1).ToArray();
        for (var tokenIndex = 0; tokenIndex < tokens.Length; tokenIndex++)
        {
            var raw = tokens[tokenIndex];
            var token = raw.Replace("~1", "/").Replace("~0", "~");
            var found = node switch
            {
                JsonObject obj => obj.TryGetPropertyValue(token, out node),
                JsonArray array when int.TryParse(token, out var index) && index >= 0 && index < array.Count => (node = array[index]) is not null || tokenIndex == tokens.Length - 1,
                _ => false
            };
            if (!found || (node is null && tokenIndex != tokens.Length - 1)) { result = null; return false; }
        }
        result = node; return true;
    }

    private static void ValidateRequest(Request request)
    {
        if (request.SchemaVersion != 1) throw new RequestException("schemaVersion must be 1.");
        if (request.Source.Kind is not ("stock" or "utoc")) throw new RequestException("source.kind must be stock or utoc.");
        if (request.Source.Kind == "utoc" && (string.IsNullOrWhiteSpace(request.Source.Utoc) || Path.GetExtension(request.Source.Utoc) != ".utoc"))
            throw new RequestException("UTOC source requires one exact .utoc path.");
        if (request.Source.Kind == "stock" && request.Source.Utoc is not null) throw new RequestException("Stock source must not specify utoc.");
        if (request.Assets.Count == 0) throw new RequestException("assets must contain at least one item.");
        if (request.Assets.Select(x => x.Id).Distinct(StringComparer.Ordinal).Count() != request.Assets.Count) throw new RequestException("Asset ids must be unique.");
        foreach (var item in request.Assets)
        {
            if (string.IsNullOrWhiteSpace(item.Id)) throw new RequestException("Every asset requires a non-empty id.");
            if (!Regex.IsMatch(item.Package, @"^/(Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_ /-]+$") || item.Package.Trim() != item.Package)
                throw new RequestException($"Asset '{item.Id}' must use one exact virtual package path without extension, object suffix, wildcard or traversal.");
            if (item.SourceGlb is { MeshIndex: not null, MeshName: not null }) throw new RequestException($"Asset '{item.Id}' sourceGlb chooses both meshIndex and meshName.");
            if (item.SourceGlb is { } comparison)
            {
                if (!double.IsFinite(comparison.PositionMatchToleranceMeters) || comparison.PositionMatchToleranceMeters <= 0) throw new RequestException($"Asset '{item.Id}' positionMatchToleranceMeters must be finite and positive.");
                if (!comparison.Attributes.ContainsKey("POSITION")) throw new RequestException($"Asset '{item.Id}' sourceGlb.attributes must include POSITION for vertex correspondence and topology.");
                foreach (var tolerance in comparison.Attributes)
                    if (!double.IsFinite(tolerance.Value) || tolerance.Value < 0) throw new RequestException($"Asset '{item.Id}' tolerance for {tolerance.Key} must be finite and nonnegative.");
            }
        }
    }

    private static JsonObject SourceFacts(SourceRequest source, FileInfo? candidate)
    {
        var result = new JsonObject { ["kind"] = source.Kind };
        if (candidate is null) return result;
        if (!candidate.Exists) throw new RequestException($"Candidate UTOC does not exist: {candidate.FullName}");
        result["utoc"] = candidate.FullName; result["utocSha256"] = Hash(candidate.FullName);
        var stem = Path.Combine(candidate.DirectoryName!, Path.GetFileNameWithoutExtension(candidate.Name));
        foreach (var extension in new[] { ".ucas", ".pak" })
        {
            var path = stem + extension;
            if (!File.Exists(path)) throw new RequestException($"Candidate container sibling is missing: {path}");
            result[extension[1..] + "Path"] = Path.GetFullPath(path);
            result[extension[1..] + "Sha256"] = Hash(path);
        }
        return result;
    }

    private static string Owner(GameFile file) => file is VfsEntry entry ? Path.GetFullPath(entry.Vfs.Path) : file.GetType().Name;
    private static bool SamePath(string left, string right) => string.Equals(Path.GetFullPath(left), Path.GetFullPath(right), StringComparison.OrdinalIgnoreCase);
    private static string Hash(string path) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.GetFullPath(path))));
    private static void WriteJson(string path, JsonNode node) => File.WriteAllText(path, node.ToJsonString(JsonOptions));
    private static string Classify(Exception exception) => exception switch
    {
        FileNotFoundException => "missing-package", WrongSourceException => "wrong-source", UnsupportedException => "unsupported",
        PolicyComparisonException => "comparison-failed", _ => "decode-error"
    };

    private static int CountZeroArea(MeshLodDto<MeshVertex> lod)
    {
        var count = 0;
        for (var i = 0; i < lod.Indices.Length; i += 3)
        {
            var p = lod.Indices.Skip(i).Take(3).Select(x => lod.Vertices[checked((int)x)].Position).ToArray();
            var ab = new[] { (double)p[1].X - p[0].X, (double)p[1].Y - p[0].Y, (double)p[1].Z - p[0].Z };
            var ac = new[] { (double)p[2].X - p[0].X, (double)p[2].Y - p[0].Y, (double)p[2].Z - p[0].Z };
            var cross = new[] { ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0] };
            if (cross.Sum(x => x * x) == 0d) count++;
        }
        return count;
    }

    private static (long, long, long) Cell(double[] row, double size) => ((long)Math.Floor(row[0] / size), (long)Math.Floor(row[1] / size), (long)Math.Floor(row[2] / size));
    private static readonly Dictionary<string, (int Offset, int Width)> FixedLayout = new() { ["POSITION"] = (0, 3), ["NORMAL"] = (3, 3), ["COLOR_0"] = (6, 4) };
    private static (int Offset, int Width) Layout(string name)
    {
        if (FixedLayout.TryGetValue(name, out var fixedValue)) return fixedValue;
        if (name.StartsWith("TEXCOORD_") && int.TryParse(name[9..], out var index)) return (10 + index * 2, 2);
        throw new UnsupportedException($"Unsupported attribute: {name}");
    }
    private static int LayoutWidth(IEnumerable<string> names) => names.Select(x => { var l = Layout(x); return l.Offset + l.Width; }).Append(3).Max();
    private static void Set(double[] row, string name, double[] values, string[] selected)
    {
        if (!selected.Contains(name, StringComparer.Ordinal)) return;
        var layout = Layout(name); Array.Copy(values, 0, row, layout.Offset, layout.Width);
    }
    private static string Key(double[] row) => string.Join("|", row.Select(x => x.ToString("R", System.Globalization.CultureInfo.InvariantCulture)));
    private static string FaceKey(int[] face, double[][] rows, int[]? mapping)
    {
        var values = face.Select(x => Key(rows[mapping is null ? x : mapping[x]])).ToArray();
        var variants = new[] { string.Join(";", values), string.Join(";", values.Skip(1).Concat(values.Take(1))), string.Join(";", values.Skip(2).Concat(values.Take(2))) };
        return variants.Min(StringComparer.Ordinal)!;
    }
    private static Dictionary<string, int> FaceCounts(int[][] faces, double[][] rows, int[]? mapping)
    {
        var result = new Dictionary<string, int>();
        foreach (var face in faces) { var key = FaceKey(face, rows, mapping); result[key] = result.GetValueOrDefault(key) + 1; }
        return result;
    }
    private static int DifferenceCount(Dictionary<string, int> left, Dictionary<string, int> right) => left.Sum(x => Math.Max(0, x.Value - right.GetValueOrDefault(x.Key)));
    private static List<int[]> MissingFaces(int[][] faces, double[][] rows, Dictionary<string, int> source, Dictionary<string, int> target)
    {
        var needed = source.ToDictionary(x => x.Key, x => Math.Max(0, x.Value - target.GetValueOrDefault(x.Key)));
        var result = new List<int[]>();
        foreach (var face in faces) { var key = FaceKey(face, rows, null); if (needed.GetValueOrDefault(key) > 0) { result.Add(face); needed[key]--; } }
        return result;
    }
    private static double TriangleArea(double[][] rows, int[] face)
    {
        var a = rows[face[0]]; var b = rows[face[1]]; var c = rows[face[2]];
        var ab = new[] { b[0] - a[0], b[1] - a[1], b[2] - a[2] }; var ac = new[] { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
        var cross = new[] { ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0] };
        return .5 * Math.Sqrt(cross.Sum(x => x * x));
    }
}

internal sealed class SourceProvider(DirectoryInfo gamePaks, FileInfo? candidate) : DefaultFileProvider(gamePaks, SearchOption.TopDirectoryOnly, new VersionContainer(EGame.GAME_UE5_8), StringComparer.OrdinalIgnoreCase)
{
    public override void Initialize()
    {
        foreach (var file in gamePaks.EnumerateFiles("*.utoc").Where(x => x.Name.Equals("global.utoc", StringComparison.OrdinalIgnoreCase) || Regex.IsMatch(x.Name, @"^pakchunk\d+(?:optional)?-Windows\.utoc$", RegexOptions.IgnoreCase))) RegisterVfs(file);
        if (candidate is not null) RegisterVfs(candidate);
    }
    internal string[] AllOwners(string physicalPath)
    {
        if (!Files.TryGetValues(physicalPath, out var files)) return [];
        return files.Select(ProgramOwner).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
    }
    private static string ProgramOwner(GameFile file) => file is VfsEntry entry ? Path.GetFullPath(entry.Vfs.Path) : file.GetType().Name;
}

internal static class Glb
{
    internal static GlbData Read(string path, int? meshIndex, string? meshName, string[] selected)
    {
        using var stream = File.OpenRead(path); using var reader = new BinaryReader(stream, Encoding.UTF8, true);
        if (reader.ReadUInt32() != 0x46546C67 || reader.ReadUInt32() != 2 || reader.ReadUInt32() != stream.Length) throw new UnsupportedException("Invalid GLB header.");
        var jsonLength = reader.ReadInt32(); if (reader.ReadUInt32() != 0x4E4F534A) throw new UnsupportedException("GLB JSON chunk missing.");
        using var doc = JsonDocument.Parse(reader.ReadBytes(jsonLength));
        var binLength = reader.ReadInt32(); if (reader.ReadUInt32() != 0x004E4942) throw new UnsupportedException("GLB BIN chunk missing.");
        var binStart = stream.Position;
        var meshes = doc.RootElement.GetProperty("meshes");
        var index = SelectMesh(meshes, meshIndex, meshName); var mesh = meshes[index];
        EnsureIdentityUse(doc.RootElement, index);
        var rows = new List<double[]>(); var faces = new List<int[]>();
        foreach (var primitive in mesh.GetProperty("primitives").EnumerateArray())
        {
            if (primitive.TryGetProperty("mode", out var mode) && mode.GetInt32() != 4) throw new UnsupportedException("Only triangle GLB primitives are supported.");
            if (primitive.TryGetProperty("extensions", out var extensions) && extensions.TryGetProperty("KHR_draco_mesh_compression", out _)) throw new UnsupportedException("KHR_draco_mesh_compression is not supported.");
            var attrs = primitive.GetProperty("attributes");
            foreach (var name in selected) if (!attrs.TryGetProperty(name, out _)) throw new UnsupportedException($"Selected GLB mesh primitive is missing {name}.");
            if (!attrs.TryGetProperty("POSITION", out var positionAccessor)) throw new UnsupportedException("GLB comparison requires POSITION for correspondence.");
            var count = doc.RootElement.GetProperty("accessors")[positionAccessor.GetInt32()].GetProperty("count").GetInt32();
            var primitiveRows = Enumerable.Range(0, count).Select(_ => new double[ProgramLayoutWidth(selected)]).ToArray();
            foreach (var name in selected)
            {
                var values = Accessor(doc.RootElement, attrs.GetProperty(name).GetInt32(), stream, binStart);
                var layout = ProgramLayout(name);
                for (var i = 0; i < count; i++)
                {
                    if (name == "COLOR_0" && values[i].Length == 3) values[i] = [values[i][0], values[i][1], values[i][2], 1d];
                    if (values[i].Length != layout.Width) throw new UnsupportedException($"GLB {name} accessor width is {values[i].Length}, expected {layout.Width}.");
                    Array.Copy(values[i], 0, primitiveRows[i], layout.Offset, layout.Width);
                }
            }
            var baseVertex = rows.Count; rows.AddRange(primitiveRows);
            var indices = primitive.TryGetProperty("indices", out var indexAccessor)
                ? Accessor(doc.RootElement, indexAccessor.GetInt32(), stream, binStart).Select(x => checked((int)x[0])).ToArray()
                : Enumerable.Range(0, count).ToArray();
            if (indices.Length % 3 != 0) throw new UnsupportedException("GLB index count is not divisible by three.");
            for (var i = 0; i < indices.Length; i += 3) faces.Add([baseVertex + indices[i], baseVertex + indices[i + 1], baseVertex + indices[i + 2]]);
        }
        return new GlbData(rows.ToArray(), faces.ToArray(), index, mesh.TryGetProperty("name", out var n) ? n.GetString() : null);
    }
    private static int SelectMesh(JsonElement meshes, int? index, string? name)
    {
        if (index is not null) { if (index < 0 || index >= meshes.GetArrayLength()) throw new UnsupportedException("sourceGlb.meshIndex is out of range."); return index.Value; }
        if (name is not null)
        {
            var matches = meshes.EnumerateArray().Select((x, i) => (x, i)).Where(x => x.x.TryGetProperty("name", out var n) && n.GetString() == name).ToArray();
            if (matches.Length != 1) throw new UnsupportedException($"sourceGlb.meshName matched {matches.Length} meshes; choose one exact meshIndex."); return matches[0].i;
        }
        if (meshes.GetArrayLength() != 1) throw new UnsupportedException($"GLB contains {meshes.GetArrayLength()} meshes; specify sourceGlb.meshIndex or meshName."); return 0;
    }
    private static void EnsureIdentityUse(JsonElement root, int meshIndex)
    {
        if (!root.TryGetProperty("nodes", out var nodes)) return;
        var parents = new Dictionary<int, int>();
        foreach (var pair in nodes.EnumerateArray().Select((node, index) => (node, index)))
            if (pair.node.TryGetProperty("children", out var children)) foreach (var child in children.EnumerateArray()) parents[child.GetInt32()] = pair.index;
        var users = nodes.EnumerateArray().Select((node, index) => (node, index)).Where(x => x.node.TryGetProperty("mesh", out var mesh) && mesh.GetInt32() == meshIndex).ToArray();
        if (users.Length > 1) throw new UnsupportedException($"Selected GLB mesh is instanced by {users.Length} nodes; transformed/instanced comparison is unsupported.");
        if (users.Length == 0) return;
        var current = users[0].index;
        while (true)
        {
            var node = nodes[current];
            if (!Identity(node)) throw new UnsupportedException($"Selected GLB mesh node or ancestor {current} has a transform; schema v1 compares local untransformed geometry only.");
            if (!parents.TryGetValue(current, out current)) break;
        }
    }
    private static bool Identity(JsonElement node)
    {
        if (node.TryGetProperty("matrix", out var matrix))
        {
            var actual = matrix.EnumerateArray().Select(x => x.GetDouble()).ToArray(); var expected = new[] { 1d,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
            if (!actual.SequenceEqual(expected)) return false;
        }
        bool Same(string name, double[] expected) => !node.TryGetProperty(name, out var value) || value.EnumerateArray().Select(x => x.GetDouble()).SequenceEqual(expected);
        return Same("translation", [0,0,0]) && Same("rotation", [0,0,0,1]) && Same("scale", [1,1,1]);
    }
    private static double[][] Accessor(JsonElement root, int index, Stream stream, long binStart)
    {
        var accessor = root.GetProperty("accessors")[index]; var view = root.GetProperty("bufferViews")[accessor.GetProperty("bufferView").GetInt32()];
        if (accessor.TryGetProperty("sparse", out _)) throw new UnsupportedException("Sparse GLB accessors are not supported.");
        var component = accessor.GetProperty("componentType").GetInt32(); var width = accessor.GetProperty("type").GetString() switch { "SCALAR" => 1, "VEC2" => 2, "VEC3" => 3, "VEC4" => 4, _ => throw new UnsupportedException("Unsupported GLB accessor width.") };
        var bytes = component switch { 5120 or 5121 => 1, 5122 or 5123 => 2, 5125 or 5126 => 4, _ => throw new UnsupportedException($"Unsupported GLB component type {component}.") };
        var count = accessor.GetProperty("count").GetInt32(); var stride = view.TryGetProperty("byteStride", out var s) ? s.GetInt32() : bytes * width;
        var offset = binStart + (view.TryGetProperty("byteOffset", out var viewOffset) ? viewOffset.GetInt64() : 0) + (accessor.TryGetProperty("byteOffset", out var o) ? o.GetInt64() : 0);
        var normalized = accessor.TryGetProperty("normalized", out var norm) && norm.GetBoolean(); var result = new double[count][]; using var reader = new BinaryReader(stream, Encoding.UTF8, true);
        for (var i = 0; i < count; i++)
        {
            stream.Position = offset + i * stride; result[i] = Enumerable.Range(0, width).Select(_ => Read(reader, component, normalized)).ToArray();
            if (result[i].Any(x => !double.IsFinite(x))) throw new UnsupportedException($"GLB accessor {index} contains a non-finite value at element {i}.");
        }
        return result;
    }
    private static double Read(BinaryReader reader, int type, bool normalized) => type switch
    {
        5120 => normalized ? Math.Max(reader.ReadSByte() / 127d, -1d) : reader.ReadSByte(), 5121 => normalized ? reader.ReadByte() / 255d : reader.ReadByte(),
        5122 => normalized ? Math.Max(reader.ReadInt16() / 32767d, -1d) : reader.ReadInt16(), 5123 => normalized ? reader.ReadUInt16() / 65535d : reader.ReadUInt16(),
        5125 => reader.ReadUInt32(), 5126 => reader.ReadSingle(), _ => throw new UnsupportedException("Unsupported GLB component type.")
    };
    private static (int Offset, int Width) ProgramLayout(string name) => name switch { "POSITION" => (0, 3), "NORMAL" => (3, 3), "COLOR_0" => (6, 4), _ when name.StartsWith("TEXCOORD_") => (10 + int.Parse(name[9..]) * 2, 2), _ => throw new UnsupportedException($"Unsupported attribute {name}") };
    private static int ProgramLayoutWidth(IEnumerable<string> names) => names.Select(x => { var l = ProgramLayout(x); return l.Offset + l.Width; }).Append(3).Max();
}

internal sealed record GlbData(double[][] Rows, int[][] Triangles, int MeshIndex, string? MeshName);
internal sealed record MatchResult(int[] Mapping, Dictionary<string, double> MaxErrors) { public static implicit operator int[](MatchResult value) => value.Mapping; }
internal sealed class RequestException(string message) : Exception(message);
internal sealed class UnsupportedException(string message) : Exception(message);
internal sealed class WrongSourceException(string message) : Exception(message);
internal sealed class PolicyComparisonException(string message) : Exception(message);

internal sealed class Request
{
    public int SchemaVersion { get; set; }
    public string GameRoot { get; set; } = "";
    public string MappingPath { get; set; } = "";
    public string MappingManifestPath { get; set; } = "";
    public string FingerprintPath { get; set; } = "";
    public string OutputDirectory { get; set; } = "";
    public SourceRequest Source { get; set; } = new();
    public List<AssetRequest> Assets { get; set; } = [];
}
internal sealed class SourceRequest { public string Kind { get; set; } = ""; public string? Utoc { get; set; } }
internal sealed class AssetRequest
{
    public string Id { get; set; } = ""; public string Package { get; set; } = ""; public bool DecodeNanite { get; set; }
    public SourceGlbRequest? SourceGlb { get; set; } public List<Expectation> Expectations { get; set; } = [];
}
internal sealed class SourceGlbRequest
{
    public string Path { get; set; } = ""; public int? MeshIndex { get; set; } public string? MeshName { get; set; }
    public double PositionMatchToleranceMeters { get; set; } public Dictionary<string, double> Attributes { get; set; } = [];
    public TopologyRequest? Topology { get; set; }
}
internal sealed class TopologyRequest { public long MaxExtraFaces { get; set; } public long MaxMissingNondegenerateFaces { get; set; } public long? MaxMissingFaces { get; set; } }
internal sealed class Expectation { public string Fact { get; set; } = ""; public JsonNode? Equals { get; set; } public double? Min { get; set; } public double? Max { get; set; } public bool? Present { get; set; } }

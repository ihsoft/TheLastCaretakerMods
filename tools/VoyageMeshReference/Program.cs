using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
using CUE4Parse;
using CUE4Parse.FileProvider;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Assets.Exports.StaticMesh;
using CUE4Parse.UE4.Versions;

// Deliberately geometry-only: no material inference, texture decoding or Nanite reconstruction.
if (args.Length != 5)
    throw new ArgumentException("Usage: VoyageMeshReference <fingerprint.json> <mapping-manifest.json> <mapping.usmap> </Game/exact/mesh> <fresh-output.json>");
string Hash(string path) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)));
var fingerprint = JsonDocument.Parse(File.ReadAllText(args[0])).RootElement;
var mappingManifest = File.ReadAllText(args[1]);
var exeHash = fingerprint.GetProperty("executable").GetProperty("sha256").GetString()!;
var build = fingerprint.GetProperty("steam").GetProperty("buildId").ToString();
var root = fingerprint.GetProperty("gameRoot").GetString()!;
var exe = Path.Combine(root, "Voyage", "Binaries", "Win64", fingerprint.GetProperty("executable").GetProperty("name").GetString()!);
if (Hash(exe) != exeHash || !mappingManifest.Contains(exeHash, StringComparison.OrdinalIgnoreCase)
    || !mappingManifest.Contains(Hash(args[2]), StringComparison.OrdinalIgnoreCase))
    throw new InvalidDataException("Fingerprint/mapping mismatch; use Get-VoyageMappings.ps1 first.");
if (!args[3].StartsWith("/Game/") || args[3].Contains("..") || args[3].Contains('.'))
    throw new ArgumentException("Use one exact /Game mesh package, without object suffix.");
if (File.Exists(args[4])) throw new IOException("Output already exists.");
var bundleRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "CUE4Parse"));
var bundle = JsonDocument.Parse(File.ReadAllText(Path.Combine(bundleRoot, "publish-manifest.json"))).RootElement;
foreach (var entry in bundle.GetProperty("files").EnumerateArray())
    if (Hash(Path.Combine(bundleRoot, entry.GetProperty("path").GetString()!)) != entry.GetProperty("sha256").GetString())
        throw new InvalidDataException("Canonical parser bundle hash mismatch.");
using var provider = new StockProvider(new DirectoryInfo(Path.Combine(root, "Voyage", "Content", "Paks")));
provider.MappingsContainer = new FileUsmapTypeMappingsProvider(args[2]);
provider.Initialize();
provider.Mount();
provider.PostMount();
provider.LoadVirtualPaths();
var mesh = provider.LoadPackage(args[3]).GetExports().OfType<UStaticMesh>().Single();
var lod = mesh.RenderData?.LODs?.FirstOrDefault() ?? throw new InvalidDataException("No LOD0");
if (lod.SkipLod) throw new InvalidDataException("LOD0 has no ordinary render geometry");
var positions = lod.PositionVertexBuffer!.Verts;
var indices = lod.IndexBuffer!.Buffer ?? throw new InvalidDataException("No index data");
var uv = lod.VertexBuffer!.UV;
var vertices = positions.Select(p => new[] { p.X, p.Y, p.Z }).ToArray();
var normals = uv.Select(v => new[] { v.Normal[2].X, v.Normal[2].Y, v.Normal[2].Z }).ToArray();
var texcoords = uv.Select(v => v.UV.Select(t => new[] { t.U, t.V }).ToArray()).ToArray();
if (vertices.Length != normals.Length || indices.Any(i => i >= vertices.Length))
    throw new InvalidDataException("Invalid vertex/index buffers");
var sections = lod.Sections.Select(s => new { material = s.MaterialIndex, firstIndex = s.FirstIndex, triangles = s.NumTriangles }).ToArray();
if (sections.Any(s => s.firstIndex + s.triangles * 3 > indices.Length))
    throw new InvalidDataException("Invalid mesh section");
var result = new {
    source = args[3], steamBuild = build, executableSha256 = exeHash,
    mappingSha256 = Hash(args[2]), parserSha256 = Hash(Path.Combine(bundleRoot, "CUE4Parse.dll")),
    geometry = "Cooked ordinary render LOD0; not high-resolution Nanite data",
    units = "centimeters", coordinates = "Unreal source coordinates, original pivot",
    vertices, normals, texcoords, indices, sections,
    materials = mesh.StaticMaterials.Select(m => m.MaterialSlotName.ToString()).ToArray()
};
Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[4]))!);
File.WriteAllText(args[4], JsonSerializer.Serialize(result));
Console.WriteLine(JsonSerializer.Serialize(new { output = Path.GetFullPath(args[4]), sha256 = Hash(args[4]), vertices = vertices.Length,
    triangles = sections.Sum(s => s.triangles), sections = sections.Length, source = args[3] }));

sealed class StockProvider : DefaultFileProvider
{
    private readonly DirectoryInfo sourceDirectory;
    public StockProvider(DirectoryInfo directory) : base(directory,
        SearchOption.TopDirectoryOnly, new VersionContainer(EGame.GAME_UE5_8), StringComparer.OrdinalIgnoreCase)
    {
        sourceDirectory = directory;
    }
    public override void Initialize()
    {
        foreach (var file in sourceDirectory.EnumerateFiles("*.utoc").Where(f =>
            f.Name.Equals("global.utoc", StringComparison.OrdinalIgnoreCase) ||
            Regex.IsMatch(f.Name, @"^pakchunk\d+(?:optional)?-Windows\.utoc$", RegexOptions.IgnoreCase)))
            RegisterVfs(file);
    }
}

using CUE4Parse;
using CUE4Parse.FileProvider;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Objects.UObject;
using CUE4Parse.UE4.Versions;
using Newtonsoft.Json;
using Serilog;

if (args.Length < 3)
{
    Console.Error.WriteLine("Usage: VoyageAssetInspector <PaksDir> <asset-name-fragment> <output-dir> [mappings.usmap]");
    return 2;
}

var paksDirectory = Path.GetFullPath(args[0]);
var fragment = args[1];
var outputDirectory = Path.GetFullPath(args[2]);
var mappingsPath = args.Length >= 4 ? Path.GetFullPath(args[3]) : null;

Directory.CreateDirectory(outputDirectory);
Log.Logger = new LoggerConfiguration()
    .MinimumLevel.Information()
    .WriteTo.Console()
    .CreateLogger();
CUE4ParseLog.UseLogger(Log.Logger);

var provider = new DefaultFileProvider(
    paksDirectory,
    SearchOption.TopDirectoryOnly,
    true,
    new VersionContainer(EGame.GAME_UE5_7));
provider.ReadScriptData = true;

if (mappingsPath is not null)
{
    provider.MappingsContainer = new FileUsmapTypeMappingsProvider(mappingsPath);
}

if (fragment.StartsWith("mappings-property:", StringComparison.OrdinalIgnoreCase))
{
    var propertyFragment = fragment["mappings-property:".Length..];
    var mappings = provider.MappingsForGame
        ?? throw new InvalidOperationException("A mappings file is required for mappings-property: mode.");
    var mappingMatches = mappings.Types.Values
        .Select(type => new
        {
            Type = type,
            Properties = type.Properties
                .Where(pair => pair.Value.Name.Contains(propertyFragment, StringComparison.OrdinalIgnoreCase))
                .OrderBy(pair => pair.Key)
                .ToArray()
        })
        .Where(item => item.Properties.Length > 0)
        .OrderBy(item => item.Type.Name, StringComparer.OrdinalIgnoreCase)
        .ToArray();
    var lines = new List<string>();
    foreach (var item in mappingMatches)
    {
        lines.Add($"TYPE {item.Type.Name} : {item.Type.SuperType ?? "<none>"} ({item.Type.PropertyCount} own properties)");
        foreach (var pair in item.Properties)
        {
            var property = pair.Value;
            lines.Add($"  [{pair.Key}] {Describe(property.MappingType)} {property.Name} array={property.ArraySize?.ToString() ?? "?"}");
        }
        lines.Add(string.Empty);
    }
    var resultPath = Path.Combine(outputDirectory, "mapping-property-types.txt");
    File.WriteAllLines(resultPath, lines);
    Console.WriteLine($"Found {mappingMatches.Length} mapped type(s) containing property '{propertyFragment}'.");
    Console.WriteLine(resultPath);
    return mappingMatches.Length == 0 ? 1 : 0;
}

if (fragment.StartsWith("mappings:", StringComparison.OrdinalIgnoreCase))
{
    var typeFragment = fragment["mappings:".Length..];
    var mappings = provider.MappingsForGame
        ?? throw new InvalidOperationException("A mappings file is required for mappings: mode.");
    var mappingMatches = mappings.Types.Values
        .Where(type => type.Name.Contains(typeFragment, StringComparison.OrdinalIgnoreCase))
        .OrderBy(type => type.Name, StringComparer.OrdinalIgnoreCase)
        .ToArray();
    var lines = new List<string>();
    foreach (var type in mappingMatches)
    {
        lines.Add($"TYPE {type.Name} : {type.SuperType ?? "<none>"} ({type.PropertyCount} own properties)");
        foreach (var pair in type.Properties.OrderBy(pair => pair.Key))
        {
            var property = pair.Value;
            lines.Add($"  [{pair.Key}] {Describe(property.MappingType)} {property.Name} array={property.ArraySize?.ToString() ?? "?"}");
        }
        lines.Add(string.Empty);
    }
    var resultPath = Path.Combine(outputDirectory, "mapping-types.txt");
    File.WriteAllLines(resultPath, lines);
    Console.WriteLine($"Found {mappingMatches.Length} mapped type(s) matching '{typeFragment}'.");
    Console.WriteLine(resultPath);
    return mappingMatches.Length == 0 ? 1 : 0;
}

provider.Initialize();
var mountedFileCount = provider.Mount();
Console.WriteLine($"Mounted {mountedFileCount} file(s) from containers.");
provider.PostMount();
provider.LoadVirtualPaths();

Console.WriteLine($"Indexed {provider.Files.Count} virtual file(s).");

var listOnly = fragment.StartsWith("list:", StringComparison.OrdinalIgnoreCase);
var assetFragment = listOnly ? fragment["list:".Length..] : fragment;
var matches = provider.Files.Values
    .Where(file => file.IsUePackage && file.Path.Contains(assetFragment, StringComparison.OrdinalIgnoreCase))
    .OrderBy(file => file.Path, StringComparer.OrdinalIgnoreCase)
    .ToArray();

File.WriteAllLines(
    Path.Combine(outputDirectory, "matches.txt"),
    matches.Select(file => file.Path));

Console.WriteLine($"Found {matches.Length} package(s) matching '{assetFragment}'.");
if (listOnly)
{
    foreach (var file in matches) Console.WriteLine(file.Path);
    return matches.Length == 0 ? 1 : 0;
}
foreach (var file in matches)
{
    Console.WriteLine(file.Path);
    try
    {
        var package = provider.LoadPackage(file.Path);
        var exports = package.GetExports();
        var safeName = file.Path.Replace('/', '_').Replace('\\', '_').Replace('.', '_');
        File.WriteAllText(
            Path.Combine(outputDirectory, safeName + ".json"),
            JsonConvert.SerializeObject(exports, Formatting.Indented));

        foreach (var unrealClass in exports.OfType<UClass>())
        {
            File.WriteAllText(
                Path.Combine(outputDirectory, safeName + "." + unrealClass.Name + ".pseudo.cpp"),
                unrealClass.DecompileBlueprintToPseudo());
        }
    }
    catch (Exception exception)
    {
        File.AppendAllText(
            Path.Combine(outputDirectory, "errors.txt"),
            $"{file.Path}{Environment.NewLine}{exception}{Environment.NewLine}{Environment.NewLine}");
        Console.Error.WriteLine($"Failed: {file.Path}: {exception.Message}");
    }
}

return matches.Length == 0 ? 1 : 0;

static string Describe(CUE4Parse.MappingsProvider.PropertyType type)
{
    var detail = type.StructType ?? type.EnumName;
    if (type.InnerType is not null)
    {
        detail = Describe(type.InnerType);
    }
    if (type.ValueType is not null)
    {
        detail = $"{detail ?? "?"}, {Describe(type.ValueType)}";
    }
    return detail is null ? type.Type : $"{type.Type}<{detail}>";
}

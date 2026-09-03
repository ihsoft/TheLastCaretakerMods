using CUE4Parse;
using CUE4Parse.FileProvider;
using CUE4Parse.MappingsProvider.Usmap;
using CUE4Parse.UE4.Objects.UObject;
using CUE4Parse.UE4.Versions;
using Newtonsoft.Json;
using Serilog;
using System.Text.RegularExpressions;

try
{
if (args.Length < 3)
{
    Console.Error.WriteLine("Usage: VoyageAssetInspector <PaksDir> <asset-name-fragment> <output-dir> [-|mappings.usmap] [UE5_7|UE5_8] [-|extra-paks-dir] [All|Game|Mod|ModOnly] [-|mod-container.utoc]");
    return 2;
}

var paksDirectory = Path.GetFullPath(args[0]);
var fragment = args[1];
var outputDirectory = Path.GetFullPath(args[2]);
var mappingsPath = args.Length >= 4 &&
    !string.IsNullOrWhiteSpace(args[3]) &&
    args[3] != "-"
    ? Path.GetFullPath(args[3])
    : null;
var gameVersion = args.Length >= 5 ? ParseGameVersion(args[4]) : EGame.GAME_UE5_7;
var extraPaksDirectories = args.Length >= 6 && !string.IsNullOrWhiteSpace(args[5])
    && args[5] != "-"
    ? new[] { new DirectoryInfo(Path.GetFullPath(args[5])) }
    : Array.Empty<DirectoryInfo>();
var containerSelection = args.Length >= 7 ? ParseContainerSelection(args[6]) : ContainerSelection.All;
var modContainerPath = args.Length >= 8 && !string.IsNullOrWhiteSpace(args[7]) && args[7] != "-"
    ? ResolveModContainerPath(paksDirectory, args[7])
    : null;

if (containerSelection is ContainerSelection.Mod or ContainerSelection.ModOnly && modContainerPath is null)
{
    throw new ArgumentException($"Container selection {containerSelection} requires a mod-container .utoc path.");
}
if (containerSelection is not (ContainerSelection.Mod or ContainerSelection.ModOnly) && modContainerPath is not null)
{
    throw new ArgumentException($"A mod-container path is not valid with container selection {containerSelection}.");
}

Directory.CreateDirectory(outputDirectory);
Log.Logger = new LoggerConfiguration()
    .MinimumLevel.Information()
    .WriteTo.Console()
    .CreateLogger();
CUE4ParseLog.UseLogger(Log.Logger);

var provider = CreateProvider(
    paksDirectory,
    extraPaksDirectories,
    gameVersion,
    containerSelection,
    modContainerPath);
provider.ReadScriptData = true;

if (mappingsPath is not null)
{
    provider.MappingsContainer = new FileUsmapTypeMappingsProvider(mappingsPath);
}

if (fragment.StartsWith("mappings-enum:", StringComparison.OrdinalIgnoreCase))
{
    var enumFragment = fragment["mappings-enum:".Length..];
    var mappings = provider.MappingsForGame
        ?? throw new InvalidOperationException("A mappings file is required for mappings-enum: mode.");
    var enumMatches = mappings.Enums
        .Where(pair => pair.Key.Contains(enumFragment, StringComparison.OrdinalIgnoreCase))
        .OrderBy(pair => pair.Key, StringComparer.OrdinalIgnoreCase)
        .ToArray();
    var lines = new List<string>();
    foreach (var pair in enumMatches)
    {
        lines.Add($"ENUM {pair.Key}");
        foreach (var value in pair.Value.OrderBy(value => value.Key))
        {
            lines.Add($"  [{value.Key}] {value.Value}");
        }
        lines.Add(string.Empty);
    }
    var resultPath = Path.Combine(outputDirectory, "mapping-enums.txt");
    File.WriteAllLines(resultPath, lines);
    Console.WriteLine($"Found {enumMatches.Length} mapped enum(s) matching '{enumFragment}'.");
    Console.WriteLine(resultPath);
    return enumMatches.Length == 0 ? 1 : 0;
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

if (fragment.StartsWith("parent:", StringComparison.OrdinalIgnoreCase))
{
    var query = fragment["parent:".Length..];
    var separatorIndex = query.IndexOf('|');
    var parentFragment = separatorIndex >= 0 ? query[..separatorIndex] : query;
    var pathFragment = separatorIndex >= 0 ? query[(separatorIndex + 1)..] : string.Empty;
    if (string.IsNullOrWhiteSpace(parentFragment))
    {
        throw new ArgumentException("parent: mode requires a direct-parent name fragment.");
    }

    var candidates = provider.Files.Values
        .Where(file => file.IsUePackage &&
            file.Path.Contains(pathFragment, StringComparison.OrdinalIgnoreCase))
        .OrderBy(file => file.Path, StringComparer.OrdinalIgnoreCase)
        .ToArray();
    var parentMatches = new List<string>();
    foreach (var file in candidates)
    {
        try
        {
            var package = provider.LoadPackage(file.Path);
            foreach (var unrealClass in package.GetExports().OfType<UClass>())
            {
                var directParent = unrealClass.SuperStruct.Load<UStruct>();
                if (directParent?.Name.Contains(parentFragment, StringComparison.OrdinalIgnoreCase) == true)
                {
                    parentMatches.Add($"{file.Path} | {unrealClass.Name} : {directParent.Name}");
                }
            }
        }
        catch (Exception exception)
        {
            File.AppendAllText(
                Path.Combine(outputDirectory, "parent-search-errors.txt"),
                $"{file.Path}{Environment.NewLine}{exception.Message}{Environment.NewLine}{Environment.NewLine}");
        }
    }
    var resultPath = Path.Combine(outputDirectory, "parent-matches.txt");
    File.WriteAllLines(resultPath, parentMatches);
    Console.WriteLine(
        $"Found {parentMatches.Count} class(es) with direct parent matching '{parentFragment}' " +
        $"among {candidates.Length} package(s) matching '{pathFragment}'.");
    foreach (var match in parentMatches) Console.WriteLine(match);
    return parentMatches.Count == 0 ? 1 : 0;
}

if (fragment.StartsWith("references:", StringComparison.OrdinalIgnoreCase))
{
    var query = fragment["references:".Length..];
    var separatorIndex = query.IndexOf('|');
    var referenceFragment = separatorIndex >= 0 ? query[..separatorIndex] : query;
    var pathFragment = separatorIndex >= 0 ? query[(separatorIndex + 1)..] : string.Empty;
    if (string.IsNullOrWhiteSpace(referenceFragment))
    {
        throw new ArgumentException("references: mode requires a serialized-reference fragment.");
    }

    var candidates = provider.Files.Values
        .Where(file => file.IsUePackage &&
            file.Path.Contains(pathFragment, StringComparison.OrdinalIgnoreCase))
        .OrderBy(file => file.Path, StringComparer.OrdinalIgnoreCase)
        .ToArray();
    var referenceMatches = new List<string>();
    foreach (var file in candidates)
    {
        try
        {
            var package = provider.LoadPackage(file.Path);
            var serializedExports = JsonConvert.SerializeObject(package.GetExports(), Formatting.None);
            if (serializedExports.Contains(referenceFragment, StringComparison.OrdinalIgnoreCase))
            {
                referenceMatches.Add(file.Path);
            }
        }
        catch (Exception exception)
        {
            File.AppendAllText(
                Path.Combine(outputDirectory, "reference-search-errors.txt"),
                $"{file.Path}{Environment.NewLine}{exception.Message}{Environment.NewLine}{Environment.NewLine}");
        }
    }
    var resultPath = Path.Combine(outputDirectory, "reference-matches.txt");
    File.WriteAllLines(resultPath, referenceMatches);
    Console.WriteLine(
        $"Found {referenceMatches.Count} package(s) whose serialized exports reference '{referenceFragment}' " +
        $"among {candidates.Length} package(s) matching '{pathFragment}'.");
    foreach (var match in referenceMatches) Console.WriteLine(match);
    return referenceMatches.Count == 0 ? 1 : 0;
}

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
}
catch (Exception exception)
{
    Console.Error.WriteLine($"{exception.GetType().FullName}: {exception.Message}");
    return 1;
}

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

static EGame ParseGameVersion(string value) => value switch
{
    "UE5_7" => EGame.GAME_UE5_7,
    "UE5_8" => EGame.GAME_UE5_8,
    _ => throw new ArgumentException($"Unsupported engine version '{value}'. Expected UE5_7 or UE5_8.")
};

static ContainerSelection ParseContainerSelection(string value) => value switch
{
    "All" => ContainerSelection.All,
    "Game" => ContainerSelection.Game,
    "Mod" => ContainerSelection.Mod,
    "ModOnly" => ContainerSelection.ModOnly,
    _ => throw new ArgumentException(
        $"Unsupported container selection '{value}'. Expected All, Game, Mod, or ModOnly.")
};

static string ResolveModContainerPath(string paksDirectory, string value)
{
    var candidate = Path.IsPathRooted(value)
        ? Path.GetFullPath(value)
        : Path.GetFullPath(Path.Combine(paksDirectory, value));
    if (!candidate.EndsWith(".utoc", StringComparison.OrdinalIgnoreCase))
    {
        candidate += ".utoc";
    }
    if (!File.Exists(candidate))
    {
        throw new FileNotFoundException("The selected mod container was not found.", candidate);
    }
    if (IsGameContainer(new FileInfo(candidate)))
    {
        throw new ArgumentException($"'{candidate}' is a stock game container, not a mod container.");
    }
    return candidate;
}

static DefaultFileProvider CreateProvider(
    string paksDirectory,
    DirectoryInfo[] extraPaksDirectories,
    EGame gameVersion,
    ContainerSelection selection,
    string? modContainerPath)
{
    var versions = new VersionContainer(gameVersion);
    if (selection == ContainerSelection.All)
    {
        return new DefaultFileProvider(
            new DirectoryInfo(paksDirectory),
            extraPaksDirectories,
            SearchOption.TopDirectoryOnly,
            versions,
            StringComparer.OrdinalIgnoreCase);
    }

    if (extraPaksDirectories.Length > 0)
    {
        throw new ArgumentException("Extra Paks directories are supported only with All container selection.");
    }

    var gameContainers = new DirectoryInfo(paksDirectory)
        .EnumerateFiles("*.utoc", SearchOption.TopDirectoryOnly)
        .Where(IsGameContainer)
        .OrderBy(file => file.Name, StringComparer.Ordinal)
        .ToList();
    if (gameContainers.Count == 0)
    {
        throw new FileNotFoundException($"No stock game containers were found in '{paksDirectory}'.");
    }

    var selectedContainers = selection == ContainerSelection.ModOnly
        ? new List<FileInfo>()
        : gameContainers;
    if (modContainerPath is not null)
    {
        selectedContainers.Add(new FileInfo(modContainerPath));
    }
    return new SelectedContainerFileProvider(
        new DirectoryInfo(paksDirectory),
        selectedContainers,
        versions,
        StringComparer.OrdinalIgnoreCase);
}

static bool IsGameContainer(FileInfo file) =>
    file.Name.Equals("global.utoc", StringComparison.OrdinalIgnoreCase) ||
    Regex.IsMatch(
        file.Name,
        "^pakchunk\\d+(?:optional)?-Windows\\.utoc$",
        RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

enum ContainerSelection
{
    All,
    Game,
    Mod,
    ModOnly
}

sealed class SelectedContainerFileProvider : DefaultFileProvider
{
    private readonly DirectoryInfo _selectedWorkingDirectory;
    private readonly IReadOnlyList<FileInfo> _containers;

    public SelectedContainerFileProvider(
        DirectoryInfo workingDirectory,
        IReadOnlyList<FileInfo> containers,
        VersionContainer versions,
        StringComparer pathComparer)
        : base(workingDirectory, SearchOption.TopDirectoryOnly, versions, pathComparer)
    {
        _selectedWorkingDirectory = workingDirectory;
        _containers = containers;
    }

    public override void Initialize()
    {
        if (!_selectedWorkingDirectory.Exists)
        {
            throw new DirectoryNotFoundException(
                $"The Paks directory was not found: {_selectedWorkingDirectory.FullName}");
        }
        foreach (var container in _containers)
        {
            RegisterVfs(container);
        }
    }
}

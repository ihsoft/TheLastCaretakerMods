using UAssetAPI;
using UAssetAPI.ExportTypes;
using UAssetAPI.PropertyTypes.Objects;
using UAssetAPI.UnrealTypes;
using UAssetAPI.Unversioned;

const string BreakBottomFilter = "break-bottom-action-filter";
const string SwapHornToExit = "swap-forklift-horn-to-exit";
const string BottomWidgetName = "BP_DynamicPlayerInputHorizontalWidget_Bottom";
const string FilterPropertyName = "bFilterByActionType";
const string ForkliftCdoName = "Default__BP_Forklift_Possesable_C";

if (args.Length != 4 || args[0] is not (BreakBottomFilter or SwapHornToExit))
{
    Console.Error.WriteLine(
        "Usage: VoyageAssetPatcher <operation> <input.uasset> <mappings.usmap> <output.uasset>");
    Console.Error.WriteLine($"Operations: {BreakBottomFilter}, {SwapHornToExit}");
    return 2;
}

string operation = args[0];
string inputPath = Path.GetFullPath(args[1]);
string mappingsPath = Path.GetFullPath(args[2]);
string outputPath = Path.GetFullPath(args[3]);

if (!File.Exists(inputPath))
{
    throw new FileNotFoundException("Input asset was not found.", inputPath);
}
if (!File.Exists(mappingsPath))
{
    throw new FileNotFoundException("Mappings file was not found.", mappingsPath);
}
if (StringComparer.OrdinalIgnoreCase.Equals(inputPath, outputPath))
{
    throw new InvalidOperationException("Refusing to overwrite the extracted source asset.");
}

Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);

var mappings = new Usmap(mappingsPath);
var asset = new UAsset(inputPath, EngineVersion.VER_UE5_7, mappings);

if (operation == SwapHornToExit)
{
    WriteSurgicalForkliftHornSwap(asset, inputPath, outputPath, mappings);
    Console.WriteLine($"Patched: {ForkliftCdoName}.HornInputAction=ExitAction");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}

switch (operation)
{
    case BreakBottomFilter:
        BreakBottomActionFilter(asset);
        break;
}

asset.Write(outputPath);

var written = new UAsset(outputPath, EngineVersion.VER_UE5_7, mappings);
switch (operation)
{
    case BreakBottomFilter:
        VerifyBrokenBottomActionFilter(written);
        Console.WriteLine($"Patched: {BottomWidgetName}.{FilterPropertyName}=false");
        break;
}

Console.WriteLine($"Output: {outputPath}");
return 0;

void BreakBottomActionFilter(UAsset target)
{
    Export bottomWidget = target.Exports
        .Single(export => export.ObjectName.ToString() == BottomWidgetName);

    if (bottomWidget is NormalExport normalBottom)
    {
        if (normalBottom[FilterPropertyName] is not BoolPropertyData filterProperty)
        {
            throw new InvalidDataException(
                $"'{BottomWidgetName}.{FilterPropertyName}' is absent or is not a BoolPropertyData.");
        }
        if (!filterProperty.Value)
        {
            throw new InvalidDataException(
                $"'{BottomWidgetName}.{FilterPropertyName}' was already false; refusing an ambiguous patch.");
        }

        filterProperty.Value = false;
        return;
    }

    if (bottomWidget is RawExport rawBottom)
    {
        const string ExpectedVoyage574BottomData = "01044B039FFFFFFF010D00000000000000";
        string actualData = Convert.ToHexString(rawBottom.Data);
        if (actualData != ExpectedVoyage574BottomData)
        {
            throw new InvalidDataException(
                $"Unexpected Voyage 5.7.4 bottom-row bytes: {actualData}. Refusing a version-ambiguous raw patch.");
        }

        // The unversioned-property header occupies bytes 0..7. Byte 8 is the
        // serialized true value of bFilterByActionType. The following four
        // bytes include the CanvasPanelSlot package index; do not touch them.
        rawBottom.Data[8] = 0;
        return;
    }

    throw new InvalidDataException(
        $"Unsupported export representation for '{BottomWidgetName}': {bottomWidget.GetType().Name}.");
}

void VerifyBrokenBottomActionFilter(UAsset target)
{
    Export writtenBottom = target.Exports
        .Single(export => export.ObjectName.ToString() == BottomWidgetName);
    bool patchSurvived = writtenBottom switch
    {
        NormalExport writtenNormal =>
            writtenNormal[FilterPropertyName] is BoolPropertyData writtenFilter && !writtenFilter.Value,
        RawExport writtenRaw =>
            writtenRaw.Data.Length == 17 && writtenRaw.Data[8] == 0,
        _ => false
    };
    if (!patchSurvived)
    {
        throw new InvalidDataException("The written asset did not preserve the false filter value.");
    }
}

void WriteSurgicalForkliftHornSwap(
    UAsset target,
    string sourceUasset,
    string destinationUasset,
    Usmap targetMappings)
{
    NormalExport cdo = target.Exports
        .OfType<NormalExport>()
        .Single(export => export.ObjectName.ToString() == ForkliftCdoName);
    ObjectPropertyData horn = RequireObjectProperty(cdo, "HornInputAction");
    ObjectPropertyData exit = RequireObjectProperty(cdo, "ExitAction");

    AssertImportName(target, horn, "IAV_VehicleHorn");
    AssertImportName(target, exit, "IAV_VehicleExit");
    if (horn.Value.Index == exit.Value.Index)
    {
        throw new InvalidDataException("HornInputAction already matches ExitAction.");
    }

    string tempRoot = Path.Combine(Path.GetTempPath(), $"VoyageAssetPatcher-{Guid.NewGuid():N}");
    Directory.CreateDirectory(tempRoot);
    try
    {
        string baselineUasset = Path.Combine(tempRoot, "baseline.uasset");
        string changedUasset = Path.Combine(tempRoot, "changed.uasset");
        target.Write(baselineUasset);

        int hornIndex = horn.Value.Index;
        int exitIndex = exit.Value.Index;
        horn.Value = FPackageIndex.FromRawIndex(exitIndex);
        target.Write(changedUasset);

        byte[] baselineHeader = File.ReadAllBytes(baselineUasset);
        byte[] changedHeader = File.ReadAllBytes(changedUasset);
        if (!baselineHeader.SequenceEqual(changedHeader))
        {
            throw new InvalidDataException(
                "Horn-to-exit mutation unexpectedly changed the reserialized .uasset header.");
        }

        string baselineUexp = Path.ChangeExtension(baselineUasset, ".uexp");
        string changedUexp = Path.ChangeExtension(changedUasset, ".uexp");
        byte[] baselineData = File.ReadAllBytes(baselineUexp);
        byte[] changedData = File.ReadAllBytes(changedUexp);
        if (baselineData.Length != changedData.Length)
        {
            throw new InvalidDataException("Horn-to-exit mutation changed the .uexp length.");
        }

        byte[] hornBytes = BitConverter.GetBytes(hornIndex);
        byte[] exitBytes = BitConverter.GetBytes(exitIndex);
        List<int> candidates = FindReplacementOffsets(baselineData, changedData, hornBytes, exitBytes);
        if (candidates.Count != 1)
        {
            throw new InvalidDataException(
                $"Expected exactly one horn-index replacement in the reserialized .uexp; found {candidates.Count}.");
        }

        int replacementOffset = candidates[0];
        for (int index = 0; index < baselineData.Length; index++)
        {
            if (baselineData[index] != changedData[index] &&
                (index < replacementOffset || index >= replacementOffset + sizeof(int)))
            {
                throw new InvalidDataException(
                    $"Horn-to-exit mutation changed an unrelated .uexp byte at 0x{index:X}.");
            }
        }

        string sourceUexp = Path.ChangeExtension(sourceUasset, ".uexp");
        string destinationUexp = Path.ChangeExtension(destinationUasset, ".uexp");
        byte[] exactOriginalData = File.ReadAllBytes(sourceUexp);
        if (replacementOffset + sizeof(int) > exactOriginalData.Length ||
            !exactOriginalData.AsSpan(replacementOffset, sizeof(int)).SequenceEqual(hornBytes))
        {
            throw new InvalidDataException(
                $"The exact original .uexp does not contain the expected horn index at 0x{replacementOffset:X}.");
        }

        File.Copy(sourceUasset, destinationUasset, true);
        Array.Copy(exitBytes, 0, exactOriginalData, replacementOffset, sizeof(int));
        File.WriteAllBytes(destinationUexp, exactOriginalData);

        var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
        VerifyForkliftHornMatchesExit(written);
        Console.WriteLine(
            $"Surgical .uexp replacement: offset=0x{replacementOffset:X}, " +
            $"hornIndex={hornIndex}, exitIndex={exitIndex}");
    }
    finally
    {
        Directory.Delete(tempRoot, true);
    }
}

void VerifyForkliftHornMatchesExit(UAsset target)
{
    NormalExport cdo = target.Exports
        .OfType<NormalExport>()
        .Single(export => export.ObjectName.ToString() == ForkliftCdoName);
    ObjectPropertyData horn = RequireObjectProperty(cdo, "HornInputAction");
    ObjectPropertyData exit = RequireObjectProperty(cdo, "ExitAction");
    if (horn.Value.Index != exit.Value.Index)
    {
        throw new InvalidDataException("The written HornInputAction does not match ExitAction.");
    }
    AssertImportName(target, horn, "IAV_VehicleExit");
}

List<int> FindReplacementOffsets(
    byte[] baseline,
    byte[] changed,
    byte[] expectedBefore,
    byte[] expectedAfter)
{
    var offsets = new List<int>();
    for (int offset = 0; offset <= baseline.Length - sizeof(int); offset++)
    {
        if (baseline.AsSpan(offset, sizeof(int)).SequenceEqual(expectedBefore) &&
            changed.AsSpan(offset, sizeof(int)).SequenceEqual(expectedAfter))
        {
            offsets.Add(offset);
        }
    }
    return offsets;
}

ObjectPropertyData RequireObjectProperty(NormalExport export, string name)
{
    return export[name] as ObjectPropertyData
        ?? throw new InvalidDataException($"'{export.ObjectName}.{name}' is absent or not an ObjectPropertyData.");
}

void AssertImportName(UAsset target, ObjectPropertyData property, string expectedName)
{
    if (!property.IsImport())
    {
        throw new InvalidDataException($"'{property.Name}' does not reference an import.");
    }
    string actualName = property.ToImport(target).ObjectName.ToString();
    if (actualName != expectedName)
    {
        throw new InvalidDataException(
            $"'{property.Name}' expected import '{expectedName}', found '{actualName}'.");
    }
}

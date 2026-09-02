using UAssetAPI;
using UAssetAPI.ExportTypes;
using UAssetAPI.PropertyTypes.Objects;
using UAssetAPI.UnrealTypes;
using UAssetAPI.Unversioned;

const string BreakBottomFilter = "break-bottom-action-filter";
const string SwapHornToExit = "swap-forklift-horn-to-exit";
const string SwapHudIndicatorSubclass = "swap-hud-indicator-subclass";
const string RoundtripUnchanged = "roundtrip-unchanged";
const string ExportJson = "export-json";
const string SetCableUpdaterTickInterval = "set-cable-updater-tick-interval";
const string BreakCableUpdaterSuperIndex = "break-cable-updater-super-index";
const string SwapHudIndicatorExistingControl = "swap-hud-indicator-existing-control";
const string SwapDieselSocketComponentClass = "swap-diesel-socket-component-class";
const string BottomWidgetName = "BP_DynamicPlayerInputHorizontalWidget_Bottom";
const string FilterPropertyName = "bFilterByActionType";
const string ForkliftCdoName = "Default__BP_Forklift_Possesable_C";
const string VoyageHudCdoName = "Default__BP_VoyageIngameHud_C";
const string IndicatorSubclassPropertyName = "IndicatorSubClass";
const string StockIndicatorPackageName = "/Game/UI/Game/Interact/WBP_InteractIndicator";
const string StockIndicatorClassName = "WBP_InteractIndicator_C";
const string MarkerIndicatorPackageName = "/Game/Mods/BoatProbe/WBP_InteractIndicator_M";
const string MarkerIndicatorClassName = "WBP_InteractIndicator_C";
const string ControlWidgetPackageName = "/Game/UI/Game/HUD/BP_VoyageWeaponHolsterWidget";
const string ControlWidgetClassName = "BP_VoyageWeaponHolsterWidget_C";
const string StockSocketComponentPackageName = "/Script/Voyage";
const string StockSocketComponentClassName = "VoyageModuleSocketViewComponent";
const string MarkerSocketComponentPackageName =
    "/Game/Mods/BoatSocket/BP_BoatDieselSocketTotal";
const string MarkerSocketComponentClassName = "BP_BoatDieselSocketTotal_C";
const string CableUpdaterCdoName = "Default__BP_VoyageCableUpdater_C";
const string CableUpdaterGeneratedClassName = "BP_VoyageCableUpdater_C";
const string CableUpdaterNativeClassName = "VoyageCableUpdater";
const string CableUpdaterNativePackageName = "/Script/Voyage";
const float CableUpdaterProbeTickIntervalSeconds = 1.0f;
const int CableUpdaterProbeTickIntervalDataOffset = 7;
const int CableUpdaterCrashMarkerInvalidSuperExportIndex = 66558;
byte[] OriginalCableUpdaterCdoData =
[
    0x02, 0x03,                   // PrimaryActorTick: skip 2, serialize 1, last.
    0x00, 0x03,                   // TickGroup: skip 0, serialize 1, last.
    0x05,                         // TG_PostUpdateWork (byte-backed enum).
    0x00, 0x00, 0x00, 0x00      // No object GUID follows the properties.
];
byte[] IntervalProbeCableUpdaterCdoData =
[
    0x02, 0x03,                   // PrimaryActorTick: skip 2, serialize 1, last.
    0x00, 0x02,                   // TickGroup: skip 0, serialize 1, more fragments.
    0x05, 0x03,                   // TickInterval: skip 5, serialize 1, last.
    0x05,                         // TG_PostUpdateWork (byte-backed enum).
    0x00, 0x00, 0x80, 0x3F,     // TickInterval = 1.0 seconds.
    0x00, 0x00, 0x00, 0x00      // No object GUID follows the properties.
];
string[] DieselSocketTemplateNames =
[
    "VoyageModuleSocketView_GEN_VARIABLE",
    "VoyageModuleSocketView1_GEN_VARIABLE"
];

try
{
if (args.Length != 4 ||
    args[0] is not (
        BreakBottomFilter or SwapHornToExit or SwapHudIndicatorSubclass or
        RoundtripUnchanged or ExportJson or SetCableUpdaterTickInterval or
        BreakCableUpdaterSuperIndex or SwapHudIndicatorExistingControl or
        SwapDieselSocketComponentClass))
{
    Console.Error.WriteLine(
        "Usage: VoyageAssetPatcher <operation> <input.uasset> <mappings.usmap> <output.uasset>");
    Console.Error.WriteLine(
        $"Operations: {BreakBottomFilter}, {SwapHornToExit}, " +
        $"{SwapHudIndicatorSubclass}, {RoundtripUnchanged}, {ExportJson}, " +
        $"{SetCableUpdaterTickInterval}, {BreakCableUpdaterSuperIndex}, " +
        $"{SwapHudIndicatorExistingControl}, " +
        $"{SwapDieselSocketComponentClass}");
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

if (operation == ExportJson)
{
    if (!StringComparer.OrdinalIgnoreCase.Equals(Path.GetExtension(outputPath), ".json"))
    {
        throw new InvalidOperationException("The export-json output must have a .json extension.");
    }
    File.WriteAllText(outputPath, asset.SerializeJson(true));
    Console.WriteLine("Exported parsed asset JSON without modifying the source package.");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}

if (operation == SwapHornToExit)
{
    WriteSurgicalForkliftHornSwap(asset, inputPath, outputPath, mappings);
    Console.WriteLine($"Patched: {ForkliftCdoName}.HornInputAction=ExitAction");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapHudIndicatorSubclass)
{
    WriteHudIndicatorSubclassSwap(asset, outputPath, mappings);
    Console.WriteLine(
        $"Patched: {VoyageHudCdoName}.{IndicatorSubclassPropertyName}=" +
        $"{MarkerIndicatorPackageName}.{MarkerIndicatorClassName}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == RoundtripUnchanged)
{
    asset.Write(outputPath);
    _ = new UAsset(outputPath, EngineVersion.VER_UE5_7, mappings);
    Console.WriteLine("Reopened unchanged UAssetAPI roundtrip output.");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SetCableUpdaterTickInterval)
{
    WriteCableUpdaterTickIntervalProbe(asset, inputPath, outputPath, mappings);
    Console.WriteLine(
        $"Patched: {CableUpdaterCdoName}.PrimaryActorTick.TickInterval=" +
        $"{CableUpdaterProbeTickIntervalSeconds:F1}s");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == BreakCableUpdaterSuperIndex)
{
    WriteCableUpdaterBadSuperIndexProbe(asset, inputPath, outputPath, mappings);
    Console.WriteLine(
        $"Broken intentionally: {CableUpdaterGeneratedClassName}.SuperIndex=" +
        $"{FPackageIndex.FromExport(CableUpdaterCrashMarkerInvalidSuperExportIndex).Index}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapHudIndicatorExistingControl)
{
    WriteHudIndicatorExistingControl(asset, outputPath, mappings);
    Console.WriteLine(
        $"Control patched: {VoyageHudCdoName}.{IndicatorSubclassPropertyName}=" +
        $"{ControlWidgetPackageName}.{ControlWidgetClassName}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapDieselSocketComponentClass)
{
    WriteDieselSocketComponentClassSwap(asset, outputPath, mappings);
    Console.WriteLine(
        $"Patched: {StockSocketComponentPackageName}.{StockSocketComponentClassName}=" +
        $"{MarkerSocketComponentPackageName}.{MarkerSocketComponentClassName}");
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
}
catch (Exception exception)
{
    Console.Error.WriteLine($"{exception.GetType().FullName}: {exception.Message}");
    return 1;
}

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

void WriteHudIndicatorSubclassSwap(
    UAsset target,
    string destinationUasset,
    Usmap targetMappings)
{
    Export cdoExport = target.Exports
        .Single(export => export.ObjectName.ToString() == VoyageHudCdoName);
    var stockClassCandidates = target.Imports
        .Select((import, index) => (Import: import, Index: FPackageIndex.FromImport(index)))
        .Where(candidate => candidate.Import.ObjectName.ToString() == StockIndicatorClassName)
        .ToArray();
    if (stockClassCandidates.Length != 1)
    {
        string relatedImports = string.Join(", ", target.Imports
            .Select((import, index) => $"{index}:{import.ObjectName}")
            .Where(description => description.Contains("InteractIndicator")));
        string importSample = string.Join(", ", target.Imports
            .Select((import, index) => $"{index}:{import.ObjectName}")
            .TakeLast(20));
        throw new InvalidDataException(
            $"Expected one stock indicator class import; found {stockClassCandidates.Length}. " +
            $"Import count={target.Imports.Count}. Related imports: {relatedImports}. " +
            $"Tail: {importSample}");
    }
    (Import stockClassImport, FPackageIndex stockClassIndex) = stockClassCandidates.Single();
    PrintDependencies(target, "SerializationBeforeSerialization", cdoExport.SerializationBeforeSerializationDependencies);
    PrintDependencies(target, "CreateBeforeSerialization", cdoExport.CreateBeforeSerializationDependencies);
    PrintDependencies(target, "SerializationBeforeCreate", cdoExport.SerializationBeforeCreateDependencies);
    PrintDependencies(target, "CreateBeforeCreate", cdoExport.CreateBeforeCreateDependencies);
    foreach ((Import relatedImport, int relatedIndex) in target.Imports
        .Select((import, index) => (Import: import, Index: index))
        .Where(candidate =>
            candidate.Import.ObjectName.ToString().Contains("InteractIndicator") ||
            candidate.Import.ObjectName.ToString().Contains("BoatProbe")))
    {
        Console.WriteLine(
            $"Related import {relatedIndex}: class={relatedImport.ClassPackage}." +
            $"{relatedImport.ClassName}, object={relatedImport.ObjectName}, " +
            $"packageName={relatedImport.PackageName}, outer={relatedImport.OuterIndex.Index}");
    }
    if (target.Imports.Any(import =>
        import.ObjectName.ToString() == MarkerIndicatorPackageName))
    {
        throw new InvalidDataException("The HUD already contains the marker import; refusing duplication.");
    }

    ObjectPropertyData? indicatorSubclass = null;
    RawExport? rawCdo = null;
    switch (cdoExport)
    {
        case NormalExport normalCdo:
            indicatorSubclass = RequireObjectProperty(normalCdo, IndicatorSubclassPropertyName);
            if (indicatorSubclass.Value.Index != stockClassIndex.Index)
            {
                throw new InvalidDataException(
                    "IndicatorSubClass does not use the expected stock class import.");
            }
            break;
        case RawExport raw:
            rawCdo = raw;
            byte[] stockIndexBytes = BitConverter.GetBytes(stockClassIndex.Index);
            List<int> stockIndexOffsets = FindSequenceOffsets(raw.Data, stockIndexBytes);
            if (stockIndexOffsets.Count != 1)
            {
                throw new InvalidDataException(
                    $"Expected exactly one stock indicator class index in the raw HUD CDO; " +
                    $"found {stockIndexOffsets.Count}.");
            }
            break;
        default:
            throw new InvalidDataException(
                $"Unsupported HUD CDO export representation: {cdoExport.GetType().Name}.");
    }

    if (!stockClassImport.OuterIndex.IsImport())
    {
        throw new InvalidDataException(
            $"'{IndicatorSubclassPropertyName}' class import has no package-import outer.");
    }
    Import stockPackageImport = stockClassImport.OuterIndex.ToImport(target);
    if (stockPackageImport.ObjectName.ToString() != StockIndicatorPackageName ||
        !stockPackageImport.OuterIndex.IsNull())
    {
        throw new InvalidDataException(
            $"Unexpected stock indicator package import: '{stockPackageImport.ObjectName}', " +
            $"outer={stockPackageImport.OuterIndex.Index}.");
    }
    Console.WriteLine(
        $"Stock package import: class={stockPackageImport.ClassPackage}." +
        $"{stockPackageImport.ClassName}, object={stockPackageImport.ObjectName}, " +
        $"packageName={stockPackageImport.PackageName}, outer={stockPackageImport.OuterIndex.Index}");
    Console.WriteLine(
        $"Stock class import: class={stockClassImport.ClassPackage}." +
        $"{stockClassImport.ClassName}, object={stockClassImport.ObjectName}, " +
        $"packageName={stockClassImport.PackageName}, outer={stockClassImport.OuterIndex.Index}");

    int embeddedWidgetClassReferences = target.Exports.Count(export =>
        export.ClassIndex.IsImport() &&
        export.ClassIndex.Index == stockClassIndex.Index);
    if (embeddedWidgetClassReferences != 1)
    {
        throw new InvalidDataException(
            "Expected exactly one embedded widget export using the stock indicator class; " +
            $"found {embeddedWidgetClassReferences}.");
    }

    Import markerPackageImport = CloneImport(
        stockPackageImport,
        target,
        FPackageIndex.FromRawIndex(0),
        MarkerIndicatorPackageName);
    FPackageIndex markerPackageIndex = target.AddImport(markerPackageImport);
    Import markerClassImport = CloneImport(
        stockClassImport,
        target,
        markerPackageIndex,
        MarkerIndicatorClassName);
    FPackageIndex markerClassIndex = target.AddImport(markerClassImport);
    int stockCdoDependencyCount = cdoExport.CreateBeforeSerializationDependencies.Count(
        dependency => dependency.Index == stockClassIndex.Index);
    if (stockCdoDependencyCount != 1)
    {
        throw new InvalidDataException(
            "Expected exactly one stock indicator class in the HUD CDO " +
            $"CreateBeforeSerialization dependencies; found {stockCdoDependencyCount}.");
    }
    cdoExport.CreateBeforeSerializationDependencies = cdoExport
        .CreateBeforeSerializationDependencies
        .Select(dependency => dependency.Index == stockClassIndex.Index
            ? markerClassIndex
            : dependency)
        .ToList();
    if (indicatorSubclass is not null)
    {
        indicatorSubclass.Value = markerClassIndex;
    }
    else
    {
        byte[] stockIndexBytes = BitConverter.GetBytes(stockClassIndex.Index);
        byte[] markerIndexBytes = BitConverter.GetBytes(markerClassIndex.Index);
        int rawOffset = FindSequenceOffsets(rawCdo!.Data, stockIndexBytes).Single();
        Array.Copy(markerIndexBytes, 0, rawCdo.Data, rawOffset, sizeof(int));
        Console.WriteLine(
            $"Raw CDO index replacement: offset=0x{rawOffset:X}, " +
            $"stockIndex={stockClassIndex.Index}, markerIndex={markerClassIndex.Index}");
    }

    target.Write(destinationUasset);

    var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
    Import writtenMarkerClass = written.Imports
        .Where(import => import.ObjectName.ToString() == MarkerIndicatorClassName)
        .Where(import => import.OuterIndex.IsImport())
        .Single(import => import.OuterIndex.ToImport(written).ObjectName.ToString() ==
            MarkerIndicatorPackageName);
    Import writtenMarkerPackage = writtenMarkerClass.OuterIndex.ToImport(written);
    if (writtenMarkerPackage.ObjectName.ToString() != MarkerIndicatorPackageName)
    {
        throw new InvalidDataException(
            "The written IndicatorSubClass does not use the marker package import.");
    }

    Export writtenCdoExport = written.Exports
        .Single(export => export.ObjectName.ToString() == VoyageHudCdoName);
    FPackageIndex writtenMarkerClassIndex = FPackageIndex.FromImport(
        written.Imports.IndexOf(writtenMarkerClass));
    switch (writtenCdoExport)
    {
        case NormalExport writtenNormalCdo:
            ObjectPropertyData writtenIndicatorSubclass = RequireObjectProperty(
                writtenNormalCdo,
                IndicatorSubclassPropertyName);
            if (writtenIndicatorSubclass.Value.Index != writtenMarkerClassIndex.Index)
            {
                throw new InvalidDataException(
                    "The written IndicatorSubClass does not use the marker class import.");
            }
            break;
        case RawExport writtenRawCdo:
            int markerOccurrences = FindSequenceOffsets(
                writtenRawCdo.Data,
                BitConverter.GetBytes(writtenMarkerClassIndex.Index)).Count;
            int stockOccurrences = FindSequenceOffsets(
                writtenRawCdo.Data,
                BitConverter.GetBytes(stockClassIndex.Index)).Count;
            if (markerOccurrences != 1 || stockOccurrences != 0)
            {
                throw new InvalidDataException(
                    "The written raw HUD CDO did not preserve the exact one-index replacement.");
            }
            break;
        default:
            throw new InvalidDataException(
                $"Unsupported written HUD CDO representation: {writtenCdoExport.GetType().Name}.");
    }
    int writtenMarkerDependencies = writtenCdoExport.CreateBeforeSerializationDependencies.Count(
        dependency => dependency.Index == writtenMarkerClassIndex.Index);
    int writtenStockDependencies = writtenCdoExport.CreateBeforeSerializationDependencies.Count(
        dependency => dependency.Index == stockClassIndex.Index);
    if (writtenMarkerDependencies != 1 || writtenStockDependencies != 0)
    {
        throw new InvalidDataException(
            "The written HUD CDO dependency list did not follow the class replacement.");
    }

    int writtenEmbeddedStockReferences = written.Exports.Count(export =>
        export.ClassIndex.IsImport() &&
        export.ClassIndex.ToImport(written).ObjectName.ToString() == StockIndicatorClassName);
    if (writtenEmbeddedStockReferences != embeddedWidgetClassReferences)
    {
        throw new InvalidDataException(
            "The embedded stock indicator widget class reference changed unexpectedly.");
    }

    Console.WriteLine(
        $"Appended imports: package={markerPackageIndex.Index}, class={markerClassIndex.Index}; " +
        $"embedded stock widget exports preserved={writtenEmbeddedStockReferences}");
}

void WriteHudIndicatorExistingControl(
    UAsset target,
    string destinationUasset,
    Usmap targetMappings)
{
    Export cdo = target.Exports
        .Single(export => export.ObjectName.ToString() == VoyageHudCdoName);
    if (cdo is not RawExport rawCdo)
    {
        throw new InvalidDataException("The current HUD CDO is no longer a RawExport.");
    }

    FPackageIndex stockClassIndex = FindClassImportIndex(
        target,
        StockIndicatorPackageName,
        StockIndicatorClassName);
    FPackageIndex controlClassIndex = FindClassImportIndex(
        target,
        ControlWidgetPackageName,
        ControlWidgetClassName);
    int rawOffset = FindSequenceOffsets(
        rawCdo.Data,
        BitConverter.GetBytes(stockClassIndex.Index)).Single();
    Array.Copy(
        BitConverter.GetBytes(controlClassIndex.Index),
        0,
        rawCdo.Data,
        rawOffset,
        sizeof(int));

    int dependencyCount = cdo.CreateBeforeSerializationDependencies.Count(
        dependency => dependency.Index == stockClassIndex.Index);
    if (dependencyCount != 1)
    {
        throw new InvalidDataException(
            $"Expected one stock IndicatorSubClass dependency; found {dependencyCount}.");
    }
    cdo.CreateBeforeSerializationDependencies = cdo.CreateBeforeSerializationDependencies
        .Select(dependency => dependency.Index == stockClassIndex.Index
            ? controlClassIndex
            : dependency)
        .ToList();
    target.Write(destinationUasset);

    var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
    RawExport writtenCdo = written.Exports
        .OfType<RawExport>()
        .Single(export => export.ObjectName.ToString() == VoyageHudCdoName);
    if (FindSequenceOffsets(
            writtenCdo.Data,
            BitConverter.GetBytes(controlClassIndex.Index)).Count != 1 ||
        FindSequenceOffsets(
            writtenCdo.Data,
            BitConverter.GetBytes(stockClassIndex.Index)).Count != 0)
    {
        throw new InvalidDataException("The existing-import control CDO replacement was not exact.");
    }
    if (writtenCdo.CreateBeforeSerializationDependencies.Count(
            dependency => dependency.Index == controlClassIndex.Index) != 1 ||
        writtenCdo.CreateBeforeSerializationDependencies.Any(
            dependency => dependency.Index == stockClassIndex.Index))
    {
        throw new InvalidDataException(
            "The existing-import control dependency replacement was not exact.");
    }
    Console.WriteLine(
        $"Existing-import control replacement: offset=0x{rawOffset:X}, " +
        $"stockIndex={stockClassIndex.Index}, controlIndex={controlClassIndex.Index}");
}

void WriteDieselSocketComponentClassSwap(
    UAsset target,
    string destinationUasset,
    Usmap targetMappings)
{
    var stockCandidates = target.Imports
        .Select((import, index) => (Import: import, Index: FPackageIndex.FromImport(index)))
        .Where(candidate =>
            candidate.Import.ObjectName.ToString() == StockSocketComponentClassName)
        .Where(candidate => candidate.Import.OuterIndex.IsImport())
        .Where(candidate =>
            candidate.Import.OuterIndex.ToImport(target).ObjectName.ToString() ==
            StockSocketComponentPackageName)
        .ToArray();
    if (stockCandidates.Length != 1)
    {
        string relatedImports = string.Join(", ", target.Imports
            .Select((import, index) => (Import: import, Index: index))
            .Where(candidate =>
                candidate.Import.ObjectName.ToString().Contains("ModuleSocketView"))
            .Select(candidate =>
                $"{candidate.Index}:{candidate.Import.ObjectName}, " +
                $"package={candidate.Import.PackageName}, outer={candidate.Import.OuterIndex.Index}"));
        string importSample = string.Join(", ", target.Imports
            .Select((import, index) =>
                $"{index}:{import.ObjectName}, package={import.PackageName}, " +
                $"class={import.ClassPackage}.{import.ClassName}, outer={import.OuterIndex.Index}"));
        throw new InvalidDataException(
            $"Expected one stock socket-view class import; found {stockCandidates.Length}. " +
            $"Related imports: {relatedImports}. Imports: {importSample}");
    }

    (Import stockClassImport, FPackageIndex stockClassIndex) = stockCandidates.Single();
    string[] stockTemplateNames = target.Exports
        .Where(export => export.ClassIndex.Index == stockClassIndex.Index)
        .Select(export => export.ObjectName.ToString())
        .Order(StringComparer.Ordinal)
        .ToArray();
    if (!stockTemplateNames.SequenceEqual(
            DieselSocketTemplateNames.Order(StringComparer.Ordinal),
            StringComparer.Ordinal))
    {
        throw new InvalidDataException(
            "The stock socket-view class is not limited to the two expected Diesel " +
            $"component templates: {string.Join(", ", stockTemplateNames)}");
    }
    if (target.Imports.Any(import =>
        import.ObjectName.ToString() == MarkerSocketComponentPackageName ||
        import.ObjectName.ToString() == MarkerSocketComponentClassName))
    {
        throw new InvalidDataException(
            "The Diesel container already contains the marker component imports.");
    }

    Import stockPackageImport = stockClassImport.OuterIndex.ToImport(target);
    Import markerPackageImport = CloneImport(
        stockPackageImport,
        target,
        FPackageIndex.FromRawIndex(0),
        MarkerSocketComponentPackageName);
    FPackageIndex markerPackageIndex = target.AddImport(markerPackageImport);

    stockClassImport.OuterIndex = markerPackageIndex;
    stockClassImport.ObjectName = new FName(target, MarkerSocketComponentClassName);
    stockClassImport.PackageName = stockClassImport.ObjectName;

    target.Write(destinationUasset);

    var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
    var writtenMarkerClasses = written.Imports
        .Where(import => import.ObjectName.ToString() == MarkerSocketComponentClassName)
        .Where(import => import.OuterIndex.IsImport())
        .Where(import =>
            import.OuterIndex.ToImport(written).ObjectName.ToString() ==
            MarkerSocketComponentPackageName)
        .ToArray();
    if (writtenMarkerClasses.Length != 1)
    {
        throw new InvalidDataException(
            "The written package did not preserve the unique marker component import.");
    }
    if (written.Imports.Any(import =>
        import.ObjectName.ToString() == StockSocketComponentClassName))
    {
        throw new InvalidDataException(
            "The written package still contains the stock socket-view class import.");
    }

    Import writtenMarkerClass = writtenMarkerClasses.Single();
    FPackageIndex writtenMarkerIndex = FPackageIndex.FromImport(
        written.Imports.IndexOf(writtenMarkerClass));
    string[] writtenTemplateNames = written.Exports
        .Where(export => export.ClassIndex.Index == writtenMarkerIndex.Index)
        .Select(export => export.ObjectName.ToString())
        .Order(StringComparer.Ordinal)
        .ToArray();
    if (!writtenTemplateNames.SequenceEqual(
            DieselSocketTemplateNames.Order(StringComparer.Ordinal),
            StringComparer.Ordinal))
    {
        throw new InvalidDataException(
            "The written marker class is not used by exactly the two Diesel socket templates.");
    }

    Console.WriteLine(
        $"Redirected import index {stockClassIndex.Index}; Diesel templates: " +
        string.Join(", ", writtenTemplateNames));
}

void WriteCableUpdaterTickIntervalProbe(
    UAsset target,
    string sourceUasset,
    string destinationUasset,
    Usmap targetMappings)
{
    RawExport cdo = target.Exports
        .OfType<RawExport>()
        .Single(export => export.ObjectName.ToString() == CableUpdaterCdoName);
    if (!cdo.ObjectFlags.HasFlag(EObjectFlags.RF_ClassDefaultObject))
    {
        throw new InvalidDataException(
            $"'{CableUpdaterCdoName}' is not marked as a class default object.");
    }
    if (!cdo.Data.SequenceEqual(OriginalCableUpdaterCdoData))
    {
        throw new InvalidDataException(
            $"Unexpected cable-updater CDO bytes: {Convert.ToHexString(cdo.Data)}.");
    }
    byte[] encodedInterval = BitConverter.GetBytes(CableUpdaterProbeTickIntervalSeconds);
    if (!BitConverter.IsLittleEndian ||
        !IntervalProbeCableUpdaterCdoData.AsSpan(
            CableUpdaterProbeTickIntervalDataOffset,
            sizeof(float)).SequenceEqual(encodedInterval))
    {
        throw new PlatformNotSupportedException(
            "The asserted TickInterval encoding requires little-endian IEEE-754 floats.");
    }

    string sourceUexp = Path.ChangeExtension(sourceUasset, ".uexp");
    if (!File.Exists(sourceUexp))
    {
        throw new FileNotFoundException("Input companion export file was not found.", sourceUexp);
    }
    byte[] sourceExportData = File.ReadAllBytes(sourceUexp);
    long sourceHeaderLength = new FileInfo(sourceUasset).Length;
    int cdoOffset = checked((int)(cdo.SerialOffset - sourceHeaderLength));
    if (cdoOffset < 0 || cdoOffset + cdo.Data.Length > sourceExportData.Length ||
        !sourceExportData.AsSpan(cdoOffset, cdo.Data.Length).SequenceEqual(cdo.Data))
    {
        throw new InvalidDataException(
            "The cable-updater CDO did not occupy its asserted companion-file range.");
    }

    byte[] expectedExportData = new byte[
        sourceExportData.Length - cdo.Data.Length + IntervalProbeCableUpdaterCdoData.Length];
    sourceExportData.AsSpan(0, cdoOffset).CopyTo(expectedExportData);
    IntervalProbeCableUpdaterCdoData.CopyTo(expectedExportData, cdoOffset);
    sourceExportData.AsSpan(cdoOffset + cdo.Data.Length).CopyTo(
        expectedExportData.AsSpan(cdoOffset + IntervalProbeCableUpdaterCdoData.Length));

    cdo.Data = IntervalProbeCableUpdaterCdoData.ToArray();
    target.Write(destinationUasset);

    string destinationUexp = Path.ChangeExtension(destinationUasset, ".uexp");
    byte[] writtenExportData = File.ReadAllBytes(destinationUexp);
    if (!writtenExportData.SequenceEqual(expectedExportData))
    {
        throw new InvalidDataException(
            "The written companion file contains changes outside the exact CDO splice.");
    }

    var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
    RawExport writtenCdo = written.Exports
        .OfType<RawExport>()
        .Single(export => export.ObjectName.ToString() == CableUpdaterCdoName);
    if (!writtenCdo.Data.SequenceEqual(IntervalProbeCableUpdaterCdoData))
    {
        throw new InvalidDataException("The written TickInterval CDO bytes did not reopen exactly.");
    }
    if (written.GetNameMapIndexList().Count != target.GetNameMapIndexList().Count ||
        written.Imports.Count != target.Imports.Count ||
        written.Exports.Count != target.Exports.Count)
    {
        throw new InvalidDataException(
            "The TickInterval patch unexpectedly changed package table cardinality.");
    }

    Console.WriteLine(
        $"Exact CDO splice: uexp offset=0x{cdoOffset:X}, " +
        $"{OriginalCableUpdaterCdoData.Length}->{IntervalProbeCableUpdaterCdoData.Length} bytes; " +
        "all other export bytes preserved.");
}

void WriteCableUpdaterBadSuperIndexProbe(
    UAsset target,
    string sourceUasset,
    string destinationUasset,
    Usmap targetMappings)
{
    Export generatedClass = target.Exports.Single(
        export => export.ObjectName.ToString() == CableUpdaterGeneratedClassName);
    if (!generatedClass.SuperIndex.IsImport())
    {
        throw new InvalidDataException(
            "The cable-updater generated class no longer has an imported native superclass.");
    }
    Import nativeClass = generatedClass.SuperIndex.ToImport(target);
    if (nativeClass.ObjectName.ToString() != CableUpdaterNativeClassName ||
        !nativeClass.OuterIndex.IsImport() ||
        nativeClass.OuterIndex.ToImport(target).ObjectName.ToString() !=
            CableUpdaterNativePackageName)
    {
        throw new InvalidDataException(
            "The cable-updater generated class superclass identity changed.");
    }
    int originalSuperIndex = generatedClass.SuperIndex.Index;
    FPackageIndex invalidSuperIndex = FPackageIndex.FromExport(
        CableUpdaterCrashMarkerInvalidSuperExportIndex);
    if (invalidSuperIndex.Index <= target.Exports.Count)
    {
        throw new InvalidOperationException(
            "The intentional bad superclass index unexpectedly resolves inside the package.");
    }

    byte[] sourceHeader = File.ReadAllBytes(sourceUasset);
    string sourceUexp = Path.ChangeExtension(sourceUasset, ".uexp");
    byte[] sourceExportData = File.ReadAllBytes(sourceUexp);
    generatedClass.SuperIndex = invalidSuperIndex;
    target.Write(destinationUasset);

    string destinationUexp = Path.ChangeExtension(destinationUasset, ".uexp");
    if (!File.ReadAllBytes(destinationUexp).SequenceEqual(sourceExportData))
    {
        throw new InvalidDataException(
            "The bad-superclass marker unexpectedly changed serialized export data.");
    }
    byte[] writtenHeader = File.ReadAllBytes(destinationUasset);
    if (writtenHeader.Length != sourceHeader.Length)
    {
        throw new InvalidDataException(
            "The bad-superclass marker unexpectedly changed the package-header length.");
    }
    byte[] originalIndexBytes = BitConverter.GetBytes(originalSuperIndex);
    byte[] invalidIndexBytes = BitConverter.GetBytes(invalidSuperIndex.Index);
    int replacementOffset = FindReplacementOffsets(
        sourceHeader,
        writtenHeader,
        originalIndexBytes,
        invalidIndexBytes).Single();
    int[] differingOffsets = Enumerable.Range(0, sourceHeader.Length)
        .Where(offset => sourceHeader[offset] != writtenHeader[offset])
        .ToArray();
    int[] expectedDifferingOffsets = Enumerable.Range(replacementOffset, sizeof(int)).ToArray();
    if (!differingOffsets.SequenceEqual(expectedDifferingOffsets))
    {
        throw new InvalidDataException(
            "The bad-superclass marker changed bytes outside the one package index.");
    }

    var written = new UAsset(destinationUasset, EngineVersion.VER_UE5_7, targetMappings);
    Export writtenClass = written.Exports.Single(
        export => export.ObjectName.ToString() == CableUpdaterGeneratedClassName);
    if (writtenClass.SuperIndex.Index != invalidSuperIndex.Index ||
        written.Imports.Count != target.Imports.Count ||
        written.Exports.Count != target.Exports.Count)
    {
        throw new InvalidDataException(
            "The intentional bad superclass index did not reopen exactly.");
    }

    Console.WriteLine(
        $"Exact package-header replacement: offset=0x{replacementOffset:X}, " +
        $"SuperIndex {originalSuperIndex}->{invalidSuperIndex.Index}; .uexp preserved.");
}

FPackageIndex FindClassImportIndex(UAsset target, string packageName, string className)
{
    return target.Imports
        .Select((import, index) => (Import: import, Index: FPackageIndex.FromImport(index)))
        .Where(candidate => candidate.Import.ObjectName.ToString() == className)
        .Where(candidate => candidate.Import.OuterIndex.IsImport())
        .Single(candidate => candidate.Import.OuterIndex.ToImport(target).ObjectName.ToString() ==
            packageName)
        .Index;
}

void PrintDependencies(
    UAsset target,
    string label,
    IEnumerable<FPackageIndex> dependencies)
{
    string values = string.Join(", ", dependencies.Select(dependency =>
    {
        string name = dependency.IsImport()
            ? dependency.ToImport(target).ObjectName.ToString()
            : dependency.IsExport()
                ? dependency.ToExport(target).ObjectName.ToString()
                : "null";
        return $"{dependency.Index}:{name}";
    }));
    Console.WriteLine($"HUD CDO {label}: [{values}]");
}

List<int> FindSequenceOffsets(byte[] data, byte[] sequence)
{
    var offsets = new List<int>();
    for (int offset = 0; offset <= data.Length - sequence.Length; offset++)
    {
        if (data.AsSpan(offset, sequence.Length).SequenceEqual(sequence))
        {
            offsets.Add(offset);
        }
    }
    return offsets;
}

Import CloneImport(
    Import source,
    UAsset target,
    FPackageIndex outerIndex,
    string objectName)
{
    return new Import
    {
        ClassPackage = source.ClassPackage,
        ClassName = source.ClassName,
        OuterIndex = outerIndex,
        ObjectName = new FName(target, objectName),
        PackageName = new FName(target, objectName),
        bImportOptional = source.bImportOptional
    };
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

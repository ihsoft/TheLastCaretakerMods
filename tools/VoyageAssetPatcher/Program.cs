using UAssetAPI;
using UAssetAPI.ExportTypes;
using UAssetAPI.PropertyTypes.Objects;
using UAssetAPI.PropertyTypes.Structs;
using UAssetAPI.UnrealTypes;
using UAssetAPI.Unversioned;
using System.Text.Json;
using System.Text.Json.Serialization;

const string BreakBottomFilter = "break-bottom-action-filter";
const string SwapHornToExit = "swap-forklift-horn-to-exit";
const string SwapHudIndicatorSubclass = "swap-hud-indicator-subclass";
const string RoundtripUnchanged = "roundtrip-unchanged";
const string ExportJson = "export-json";
const string SetCableUpdaterTickInterval = "set-cable-updater-tick-interval";
const string BreakCableUpdaterSuperIndex = "break-cable-updater-super-index";
const string SwapHudIndicatorExistingControl = "swap-hud-indicator-existing-control";
const string SwapDieselSocketComponentClass = "swap-diesel-socket-component-class";
const string PatchItemDataAsset = "patch-item-data-asset";
const string MatchPackageSerialization = "match-package-serialization";
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
bool isItemPatch = args.Length > 0 && args[0] == PatchItemDataAsset;
bool isSerializationMatch = args.Length > 0 && args[0] == MatchPackageSerialization;
if ((!isItemPatch && !isSerializationMatch && args.Length != 4 && args.Length != 5) ||
    ((isItemPatch || isSerializationMatch) && args.Length != 6) ||
    args[0] is not (
        BreakBottomFilter or SwapHornToExit or SwapHudIndicatorSubclass or
        RoundtripUnchanged or ExportJson or SetCableUpdaterTickInterval or
        BreakCableUpdaterSuperIndex or SwapHudIndicatorExistingControl or
        SwapDieselSocketComponentClass or PatchItemDataAsset or
        MatchPackageSerialization))
{
    Console.Error.WriteLine(
        "Usage: VoyageAssetPatcher <operation> <input.uasset> <mappings.usmap> " +
        "<output.uasset> [UE5_7|UE5_8] [donor.uasset|item-patch.json]");
    Console.Error.WriteLine(
        $"Operations: {BreakBottomFilter}, {SwapHornToExit}, " +
        $"{SwapHudIndicatorSubclass}, {RoundtripUnchanged}, {ExportJson}, " +
        $"{SetCableUpdaterTickInterval}, {BreakCableUpdaterSuperIndex}, " +
        $"{SwapHudIndicatorExistingControl}, " +
        $"{SwapDieselSocketComponentClass}, {PatchItemDataAsset}, " +
        $"{MatchPackageSerialization}");
    return 2;
}

string operation = args[0];
string inputPath = Path.GetFullPath(args[1]);
string mappingsPath = Path.GetFullPath(args[2]);
string outputPath = Path.GetFullPath(args[3]);
EngineVersion engineVersion = args.Length >= 5
    ? args[4] switch
    {
        "UE5_7" => EngineVersion.VER_UE5_7,
        "UE5_8" => EngineVersion.VER_UE5_8,
        _ => throw new ArgumentException("Engine version must be UE5_7 or UE5_8.")
    }
    : EngineVersion.VER_UE5_7;

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
var asset = new UAsset(inputPath, engineVersion, mappings);

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
    WriteSurgicalForkliftHornSwap(asset, inputPath, outputPath, mappings, engineVersion);
    Console.WriteLine($"Patched: {ForkliftCdoName}.HornInputAction=ExitAction");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapHudIndicatorSubclass)
{
    WriteHudIndicatorSubclassSwap(asset, outputPath, mappings, engineVersion);
    Console.WriteLine(
        $"Patched: {VoyageHudCdoName}.{IndicatorSubclassPropertyName}=" +
        $"{MarkerIndicatorPackageName}.{MarkerIndicatorClassName}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == RoundtripUnchanged)
{
    asset.Write(outputPath);
    _ = new UAsset(outputPath, engineVersion, mappings);
    Console.WriteLine("Reopened unchanged UAssetAPI roundtrip output.");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SetCableUpdaterTickInterval)
{
    WriteCableUpdaterTickIntervalProbe(asset, inputPath, outputPath, mappings, engineVersion);
    Console.WriteLine(
        $"Patched: {CableUpdaterCdoName}.PrimaryActorTick.TickInterval=" +
        $"{CableUpdaterProbeTickIntervalSeconds:F1}s");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == BreakCableUpdaterSuperIndex)
{
    WriteCableUpdaterBadSuperIndexProbe(asset, inputPath, outputPath, mappings, engineVersion);
    Console.WriteLine(
        $"Broken intentionally: {CableUpdaterGeneratedClassName}.SuperIndex=" +
        $"{FPackageIndex.FromExport(CableUpdaterCrashMarkerInvalidSuperExportIndex).Index}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapHudIndicatorExistingControl)
{
    WriteHudIndicatorExistingControl(asset, outputPath, mappings, engineVersion);
    Console.WriteLine(
        $"Control patched: {VoyageHudCdoName}.{IndicatorSubclassPropertyName}=" +
        $"{ControlWidgetPackageName}.{ControlWidgetClassName}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == SwapDieselSocketComponentClass)
{
    WriteDieselSocketComponentClassSwap(asset, outputPath, mappings, engineVersion);
    Console.WriteLine(
        $"Patched: {StockSocketComponentPackageName}.{StockSocketComponentClassName}=" +
        $"{MarkerSocketComponentPackageName}.{MarkerSocketComponentClassName}");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == PatchItemDataAsset)
{
    string specificationPath = Path.GetFullPath(args[5]);
    if (!File.Exists(specificationPath))
        throw new FileNotFoundException("Item patch specification was not found.", specificationPath);
    ItemPatchSpecification specification = JsonSerializer.Deserialize<ItemPatchSpecification>(
        File.ReadAllText(specificationPath), new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow
        })
        ?? throw new InvalidDataException("Item patch specification is empty.");
    WriteItemDataAsset(asset, outputPath, mappings, engineVersion, specification);
    Console.WriteLine($"Patched item data asset '{specification.ItemObjectName}' from its owned specification.");
    Console.WriteLine($"Output: {outputPath}");
    return 0;
}
if (operation == MatchPackageSerialization)
{
    string donorPath = Path.GetFullPath(args[5]);
    if (!File.Exists(donorPath))
        throw new FileNotFoundException("Serialization donor asset was not found.", donorPath);
    WritePackageSerializationMatch(
        asset, inputPath, donorPath, outputPath, mappings, engineVersion);
    Console.WriteLine("Matched package serialization metadata to the donor.");
    Console.WriteLine($"Donor: {donorPath}");
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

var written = new UAsset(outputPath, engineVersion, mappings);
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

void WritePackageSerializationMatch(
    UAsset target,
    string sourceUasset,
    string donorUasset,
    string destinationUasset,
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
{
    UAsset donor = new(donorUasset, targetEngineVersion, targetMappings);
    if (target.IsUnversioned || target.HasUnversionedProperties)
        throw new InvalidDataException(
            "Serialization target is already unversioned; refusing an ambiguous conversion.");
    if (!donor.IsUnversioned || !donor.HasUnversionedProperties)
        throw new InvalidDataException(
            "Serialization donor must use unversioned properties.");
    if (donor.CustomVersionContainer is null || donor.CustomVersionContainer.Count == 0)
        throw new InvalidDataException(
            "Serialization donor has no resolved custom-version metadata.");

    string sourceFolderName = target.FolderName.Value;
    int sourceExportCount = target.Exports.Count;
    int sourceImportCount = target.Imports.Count;
    string[] sourceExportNames = target.Exports
        .Select(export => export.ObjectName.ToString())
        .ToArray();
    string[][] sourcePropertyNames = target.Exports
        .Select(export => export is NormalExport normal
            ? normal.Data.Select(property => property.Name.ToString()).ToArray()
            : [])
        .ToArray();

    target.LegacyFileVersion = donor.LegacyFileVersion;
    target.IsUnversioned = donor.IsUnversioned;
    target.ObjectVersion = donor.ObjectVersion;
    target.ObjectVersionUE5 = donor.ObjectVersionUE5;
    target.FileVersionLicenseeUE = donor.FileVersionLicenseeUE;
    target.PackageGuid = donor.PackageGuid;
    target.PackageFlags = donor.PackageFlags;
    target.PackageSource = donor.PackageSource;
    target.CustomVersionContainer = donor.CustomVersionContainer
        .Select(version => (CustomVersion)version.Clone())
        .ToList();

    foreach (NormalExport normal in target.Exports.OfType<NormalExport>())
        foreach (PropertyData property in normal.Data)
            NormalizeEnumValuesForUnversioned(target, property);

    target.SetSerializationEngineVersion(targetEngineVersion);
    target.Write(destinationUasset);

    UAsset written = new(destinationUasset, targetEngineVersion, targetMappings);
    UAsset original = new(sourceUasset, targetEngineVersion, targetMappings);
    if (!written.IsUnversioned || !written.HasUnversionedProperties ||
        written.LegacyFileVersion != donor.LegacyFileVersion ||
        written.ObjectVersion != donor.ObjectVersion ||
        written.ObjectVersionUE5 != donor.ObjectVersionUE5 ||
        written.FileVersionLicenseeUE != donor.FileVersionLicenseeUE ||
        written.PackageGuid != donor.PackageGuid ||
        written.PackageFlags != donor.PackageFlags ||
        written.PackageSource != donor.PackageSource)
        throw new InvalidDataException(
            "Written package serialization metadata does not match the donor.");
    if (written.FolderName.Value != sourceFolderName ||
        written.Exports.Count != sourceExportCount ||
        written.Imports.Count != sourceImportCount)
        throw new InvalidDataException(
            "Serialization conversion changed package identity or object counts.");

    string[] writtenExportNames = written.Exports
        .Select(export => export.ObjectName.ToString())
        .ToArray();
    if (!sourceExportNames.SequenceEqual(writtenExportNames, StringComparer.Ordinal))
        throw new InvalidDataException(
            "Serialization conversion changed export identities.");
    for (int index = 0; index < written.Exports.Count; index++)
    {
        if (written.Exports[index] is NormalExport writtenNormal &&
            !sourcePropertyNames[index].SequenceEqual(
                writtenNormal.Data.Select(property => property.Name.ToString()),
                StringComparer.Ordinal))
            throw new InvalidDataException(
                $"Serialization conversion changed properties of export '{writtenExportNames[index]}'.");
        if (written.Exports[index].ClassIndex.Index != original.Exports[index].ClassIndex.Index)
            throw new InvalidDataException(
                $"Serialization conversion changed the class of export '{writtenExportNames[index]}'.");
    }

    string[] donorVersions = donor.CustomVersionContainer
        .Select(version => $"{version.Key:B}:{version.Version}")
        .ToArray();
    string[] writtenVersions = written.CustomVersionContainer
        .Select(version => $"{version.Key:B}:{version.Version}")
        .ToArray();
    if (!donorVersions.SequenceEqual(writtenVersions, StringComparer.Ordinal))
        throw new InvalidDataException(
            "Written custom-version container does not match the donor.");
}

void NormalizeEnumValuesForUnversioned(UAsset target, PropertyData property)
{
    if (property is EnumPropertyData enumProperty &&
        enumProperty.EnumType?.Value?.Value is string enumType &&
        enumProperty.Value?.Value?.Value is string enumValue &&
        enumValue.StartsWith(enumType + "::", StringComparison.Ordinal))
    {
        enumProperty.Value = FName.DefineDummy(
            target, enumValue.Substring(enumType.Length + 2));
    }

    switch (property)
    {
        case StructPropertyData structure:
            foreach (PropertyData child in structure.Value)
                NormalizeEnumValuesForUnversioned(target, child);
            break;
        case ArrayPropertyData array:
            foreach (PropertyData child in array.Value)
                NormalizeEnumValuesForUnversioned(target, child);
            break;
        case MapPropertyData map:
            foreach (KeyValuePair<PropertyData, PropertyData> entry in map.Value)
            {
                NormalizeEnumValuesForUnversioned(target, entry.Key);
                NormalizeEnumValuesForUnversioned(target, entry.Value);
            }
            break;
    }
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

        var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

    var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

    var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

    var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

    var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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
    Usmap targetMappings,
    EngineVersion targetEngineVersion)
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

    var written = new UAsset(destinationUasset, targetEngineVersion, targetMappings);
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

void WriteItemDataAsset(UAsset target, string destinationUasset,
    Usmap targetMappings, EngineVersion targetEngineVersion, ItemPatchSpecification specification)
{
    if (specification.SchemaVersion != 1)
        throw new InvalidDataException("Unsupported item patch specification version.");
    if (target.Exports.Count != specification.ExpectedExportCount ||
        target.Exports.Count(export => export.ObjectName.ToString() == specification.ItemObjectName) != 1)
        throw new InvalidDataException("Item package does not match the expected export contract.");

    NormalExport item = target.Exports.OfType<NormalExport>()
        .Single(export => export.ObjectName.ToString() == specification.ItemObjectName);
    FloatPropertyData weight = item["Weight"] as FloatPropertyData
        ?? throw new InvalidDataException("Item weight is missing.");
    FloatPropertyData craftTime = item["CraftTime"] as FloatPropertyData
        ?? throw new InvalidDataException("Item craft time is missing.");
    FloatPropertyData craftEnergy = item["CraftElectricityCost"] as FloatPropertyData
        ?? throw new InvalidDataException("Item craft energy is missing.");
    IntPropertyData? craftAmount = item["CraftAmount"] as IntPropertyData;
    if ((specification.Expected.CraftAmount.HasValue || specification.Patch.CraftAmount.HasValue) &&
        craftAmount is null)
        throw new InvalidDataException("Item craft amount is missing.");
    MapPropertyData components = item["Components"] as MapPropertyData
        ?? throw new InvalidDataException("Item component recipe is missing.");
    TextPropertyData name = item["Name"] as TextPropertyData
        ?? throw new InvalidDataException("Item name is missing.");
    TextPropertyData description = item["Description"] as TextPropertyData
        ?? throw new InvalidDataException("Item description is missing.");
    ObjectPropertyData icon = RequireObjectProperty(item, "Icon");

    if (weight.Value != specification.Expected.Weight ||
        craftTime.Value != specification.Expected.CraftTime ||
        craftEnergy.Value != specification.Expected.CraftElectricityCost ||
        (specification.Expected.CraftAmount.HasValue &&
            craftAmount!.Value != specification.Expected.CraftAmount.Value) ||
        components.Value.Count != specification.Expected.Components.Count ||
        name.HistoryType != TextHistoryType.StringTableEntry ||
        description.HistoryType != TextHistoryType.StringTableEntry ||
        name.Value?.Value != specification.Expected.NameStringTableKey ||
        description.Value?.Value != specification.Expected.DescriptionStringTableKey)
        throw new InvalidDataException("Item scalar, text, or recipe preconditions changed.");

    Import FindImport(AssetReference reference)
    {
        Import[] packageCandidates = target.Imports.Where(import =>
            import.ObjectName.ToString() == reference.Package).ToArray();
        if (packageCandidates.Length != 1)
            throw new InvalidDataException(
                $"Expected one package import '{reference.Package}', found {packageCandidates.Length}.");
        Import package = packageCandidates[0];
        Import[] assetCandidates = target.Imports.Where(import =>
            import.ObjectName.ToString() == reference.Name &&
            import.OuterIndex.Index == FPackageIndex.FromImport(target.Imports.IndexOf(package)).Index)
            .ToArray();
        if (assetCandidates.Length != 1)
            throw new InvalidDataException(
                $"Expected one asset import '{reference.Package}.{reference.Name}', " +
                $"found {assetCandidates.Length}.");
        Import asset = assetCandidates[0];
        if (asset.ClassName.ToString() != reference.ClassName)
            throw new InvalidDataException($"Import '{reference.Name}' has an unexpected class.");
        return asset;
    }

    Import expectedIcon = FindImport(specification.Expected.Icon);
    AssertImportName(target, icon, expectedIcon.ObjectName.ToString());

    Dictionary<string, (ObjectPropertyData Property, IntPropertyData Amount)> componentEntries = new();
    foreach (KeyValuePair<PropertyData, PropertyData> entry in components.Value)
    {
        if (entry.Key is not ObjectPropertyData material || !material.IsImport() ||
            entry.Value is not IntPropertyData amount)
            throw new InvalidDataException("Item recipe map has an unexpected entry type.");
        string materialName = material.ToImport(target).ObjectName.ToString();
        if (!componentEntries.TryAdd(materialName, (material, amount)))
            throw new InvalidDataException($"Item recipe contains duplicate material '{materialName}'.");
    }
    foreach (RecipeComponent expected in specification.Expected.Components)
    {
        Import expectedImport = FindImport(expected.Material);
        if (!componentEntries.TryGetValue(expectedImport.ObjectName.ToString(), out var entry) ||
            entry.Amount.Value != expected.Amount)
            throw new InvalidDataException($"Item recipe precondition failed for '{expected.Material.Name}'.");
    }

    if (componentEntries.Count != specification.Expected.Components.Count)
        throw new InvalidDataException("Item recipe contains an unexpected material.");
    if (specification.Patch.Components is not null &&
        specification.Patch.ComponentReplacements.Count != 0)
        throw new InvalidDataException(
            "Patch components and componentReplacements are mutually exclusive.");
    if (specification.Patch.Components is { Count: 0 })
        throw new InvalidDataException("A replacement recipe must contain at least one component.");
    if (specification.Expected.Components.Select(component => component.Material.Name)
        .Distinct(StringComparer.Ordinal).Count() != specification.Expected.Components.Count)
        throw new InvalidDataException("Expected recipe contains duplicate material names.");
    if (specification.Patch.Components is not null &&
        specification.Patch.Components.Select(component => component.Material.Name)
            .Distinct(StringComparer.Ordinal).Count() != specification.Patch.Components.Count)
        throw new InvalidDataException("Replacement recipe contains duplicate material names.");
    if (specification.Expected.Components.Any(component => component.Amount <= 0) ||
        specification.Patch.Components?.Any(component => component.Amount <= 0) == true)
        throw new InvalidDataException("Recipe component amounts must be positive.");

    Dictionary<string, int> expectedFinalComponents =
        (specification.Patch.Components ?? specification.Expected.Components).ToDictionary(
            component => component.Material.Name, component => component.Amount);
    if (specification.Patch.Components is null)
    {
        foreach (ComponentReplacement replacement in specification.Patch.ComponentReplacements)
        {
            if (!expectedFinalComponents.Remove(replacement.From.Name, out int amount))
                throw new InvalidDataException(
                    $"Recipe replacement source '{replacement.From.Name}' is absent or duplicated.");
            if (!expectedFinalComponents.TryAdd(replacement.To.Name, amount))
                throw new InvalidDataException(
                    $"Recipe replacement target '{replacement.To.Name}' is duplicated.");
        }
    }

    var replacementRecipe = new List<(ObjectPropertyData Material, IntPropertyData Amount)>();
    if (specification.Patch.Components is not null)
    {
        ObjectPropertyData materialTemplate = componentEntries.Values.First().Property;
        IntPropertyData amountTemplate = componentEntries.Values.First().Amount;
        foreach (RecipeComponent replacement in specification.Patch.Components)
        {
            Import replacementImport = FindImport(replacement.Material);
            var material = (ObjectPropertyData)materialTemplate.Clone();
            var amount = (IntPropertyData)amountTemplate.Clone();
            material.Value = FPackageIndex.FromImport(target.Imports.IndexOf(replacementImport));
            amount.Value = replacement.Amount;
            replacementRecipe.Add((material, amount));
        }
    }

    var softObjectMutations = new List<(SoftObjectPropertyData Property,
        SoftObjectReferenceMutation Mutation)>();
    foreach (SoftObjectReferenceMutation mutation in specification.Patch.SoftObjectReferences)
    {
        if (softObjectMutations.Any(candidate =>
            candidate.Mutation.Property == mutation.Property))
            throw new InvalidDataException(
                $"Soft-object property '{mutation.Property}' is targeted more than once.");
        SoftObjectPropertyData property = item[mutation.Property] as SoftObjectPropertyData
            ?? throw new InvalidDataException(
                $"'{specification.ItemObjectName}.{mutation.Property}' is absent or not a " +
                "SoftObjectPropertyData.");
        AssertSoftObjectIdentity(mutation.Expected, mutation.Property, "expected");
        AssertSoftObjectIdentity(mutation.Value, mutation.Property, "replacement");
        AssertSoftObjectReference(property, mutation.Expected, mutation.Property);
        softObjectMutations.Add((property, mutation));
    }

    var mapClearMutations = new List<(MapPropertyData Property, MapClearMutation Mutation)>();
    foreach (MapClearMutation mutation in specification.Patch.ClearNameObjectMaps)
    {
        if (mapClearMutations.Any(candidate =>
            candidate.Mutation.Property == mutation.Property))
            throw new InvalidDataException(
                $"Map property '{mutation.Property}' is targeted more than once.");
        if (mutation.KeyType != "NameProperty" || mutation.ValueType != "ObjectProperty")
            throw new InvalidDataException(
                $"Map clear for '{mutation.Property}' must declare NameProperty/ObjectProperty types.");
        if (string.IsNullOrWhiteSpace(mutation.Property))
            throw new InvalidDataException("Map clear property name is empty.");
        MapPropertyData property = item[mutation.Property] as MapPropertyData
            ?? throw new InvalidDataException(
                $"'{specification.ItemObjectName}.{mutation.Property}' is absent or not a MapPropertyData.");
        AssertNameObjectMap(target, property, mutation);
        mapClearMutations.Add((property, mutation));
    }

    foreach (ComponentReplacement replacement in specification.Patch.ComponentReplacements)
    {
        _ = FindImport(replacement.From);
        if (string.IsNullOrWhiteSpace(replacement.To.Package) ||
            string.IsNullOrWhiteSpace(replacement.To.Name) ||
            string.IsNullOrWhiteSpace(replacement.To.ClassName))
            throw new InvalidDataException("Recipe replacement target identity is incomplete.");
    }
    if (string.IsNullOrWhiteSpace(specification.Patch.Icon.Package) ||
        string.IsNullOrWhiteSpace(specification.Patch.Icon.Name) ||
        string.IsNullOrWhiteSpace(specification.Patch.Icon.ClassName))
        throw new InvalidDataException("Replacement icon identity is incomplete.");

    FPackageIndex AddReplacementImport(AssetReference source, AssetReference replacement)
    {
        Import sourceAsset = FindImport(source);
        Import sourcePackage = sourceAsset.OuterIndex.ToImport(target);
        FPackageIndex packageIndex = target.AddImport(CloneImport(
            sourcePackage, target, FPackageIndex.FromRawIndex(0), replacement.Package));
        return target.AddImport(CloneImport(sourceAsset, target, packageIndex, replacement.Name));
    }

    icon.Value = AddReplacementImport(specification.Expected.Icon, specification.Patch.Icon);
    if (specification.Patch.Components is not null)
    {
        components.Value.Clear();
        foreach (var replacement in replacementRecipe)
            components.Value.Add(replacement.Material, replacement.Amount);
    }
    else
    {
        foreach (ComponentReplacement replacement in specification.Patch.ComponentReplacements)
        {
            Import source = FindImport(replacement.From);
            if (!componentEntries.TryGetValue(source.ObjectName.ToString(), out var entry))
                throw new InvalidDataException(
                    $"Recipe replacement source '{replacement.From.Name}' is absent.");
            entry.Property.Value = AddReplacementImport(replacement.From, replacement.To);
        }
    }

    foreach (var mutation in softObjectMutations)
        mutation.Property.Value = CreateSoftObjectPath(target, mutation.Mutation.Value);
    foreach (var mutation in mapClearMutations)
    {
        mutation.Property.Value.Clear();
        mutation.Property.KeyType = new FName(target, mutation.Mutation.KeyType);
        mutation.Property.ValueType = new FName(target, mutation.Mutation.ValueType);
    }

    craftTime.Value = specification.Patch.CraftTime;
    if (specification.Patch.CraftAmount.HasValue)
        craftAmount!.Value = specification.Patch.CraftAmount.Value;
    foreach ((TextPropertyData property, ItemTextPatch text) in new[]
    {
        (name, specification.Patch.Name),
        (description, specification.Patch.Description)
    })
    {
        property.HistoryType = TextHistoryType.Base;
        property.TableId = null;
        property.Namespace = new FString("");
        property.Value = new FString(text.Key);
        property.CultureInvariantString = new FString(text.Text);
    }

    target.Write(destinationUasset);
    UAsset written = new(destinationUasset, targetEngineVersion, targetMappings);
    if (written.Exports.Count != specification.ExpectedExportCount ||
        written.Imports.Count != target.Imports.Count)
        throw new InvalidDataException("Patched item changed export/import counts on reopening.");
    NormalExport writtenItem = written.Exports.OfType<NormalExport>()
        .Single(export => export.ObjectName.ToString() == specification.ItemObjectName);
    AssertImportName(written, RequireObjectProperty(writtenItem, "Icon"), specification.Patch.Icon.Name);
    if (writtenItem["CraftTime"] is not FloatPropertyData writtenTime ||
        writtenTime.Value != specification.Patch.CraftTime ||
        (specification.Patch.CraftAmount.HasValue &&
            (writtenItem["CraftAmount"] is not IntPropertyData writtenAmount ||
                writtenAmount.Value != specification.Patch.CraftAmount.Value)) ||
        writtenItem["Weight"] is not FloatPropertyData writtenWeight ||
        writtenWeight.Value != specification.Expected.Weight ||
        writtenItem["CraftElectricityCost"] is not FloatPropertyData writtenEnergy ||
        writtenEnergy.Value != specification.Expected.CraftElectricityCost)
        throw new InvalidDataException("Patched item scalar values changed on reopening.");
    if (writtenItem["Name"] is not TextPropertyData writtenName ||
        writtenName.HistoryType != TextHistoryType.Base ||
        writtenName.CultureInvariantString?.Value != specification.Patch.Name.Text ||
        writtenItem["Description"] is not TextPropertyData writtenDescription ||
        writtenDescription.HistoryType != TextHistoryType.Base ||
        writtenDescription.CultureInvariantString?.Value != specification.Patch.Description.Text)
        throw new InvalidDataException("Patched item text changed on reopening.");

    MapPropertyData writtenComponents = writtenItem["Components"] as MapPropertyData
        ?? throw new InvalidDataException("Patched item recipe map was lost.");
    if (writtenComponents.Value.Count != expectedFinalComponents.Count)
        throw new InvalidDataException("Patched item recipe map has the wrong length.");
    foreach (KeyValuePair<PropertyData, PropertyData> entry in writtenComponents.Value)
    {
        if (entry.Key is not ObjectPropertyData material || !material.IsImport() ||
            entry.Value is not IntPropertyData amount ||
            !expectedFinalComponents.Remove(
                material.ToImport(written).ObjectName.ToString(), out int expectedAmount) ||
            amount.Value != expectedAmount)
            throw new InvalidDataException("Patched item recipe map changed on reopening.");
    }

    foreach (SoftObjectReferenceMutation mutation in specification.Patch.SoftObjectReferences)
    {
        SoftObjectPropertyData property = writtenItem[mutation.Property] as SoftObjectPropertyData
            ?? throw new InvalidDataException(
                $"Patched soft-object property '{mutation.Property}' was lost or changed type.");
        AssertSoftObjectReference(property, mutation.Value, mutation.Property);
    }
    foreach (MapClearMutation mutation in specification.Patch.ClearNameObjectMaps)
    {
        MapPropertyData property = writtenItem[mutation.Property] as MapPropertyData
            ?? throw new InvalidDataException(
                $"Patched map property '{mutation.Property}' was lost or changed type.");
        if (property.Value.Count != 0 || property.KeyType.ToString() != mutation.KeyType ||
            property.ValueType.ToString() != mutation.ValueType)
            throw new InvalidDataException(
                $"Patched map property '{mutation.Property}' did not reopen as the requested empty map.");
    }

    void AssertSoftObjectReference(
        SoftObjectPropertyData property,
        SoftObjectReference expected,
        string propertyName)
    {
        FSoftObjectPath value = property.Value;
        string? subPath = value.SubPathString?.Value;
        if (value.AssetPath.PackageName.ToString() != expected.PackageName ||
            value.AssetPath.AssetName.ToString() != expected.AssetName ||
            subPath != expected.SubPathString)
            throw new InvalidDataException(
                $"Soft-object property '{propertyName}' does not match its expected value.");
    }

    void AssertSoftObjectIdentity(
        SoftObjectReference value,
        string propertyName,
        string role)
    {
        if (string.IsNullOrWhiteSpace(value.PackageName) ||
            string.IsNullOrWhiteSpace(value.AssetName))
            throw new InvalidDataException(
                $"Soft-object property '{propertyName}' has an incomplete {role} identity.");
    }

    FSoftObjectPath CreateSoftObjectPath(UAsset asset, SoftObjectReference value)
    {
        return new FSoftObjectPath(
            new FTopLevelAssetPath(
                new FName(asset, value.PackageName),
                new FName(asset, value.AssetName)),
            value.SubPathString is null ? null! : new FString(value.SubPathString));
    }

    void AssertNameObjectMap(UAsset asset, MapPropertyData property, MapClearMutation mutation)
    {
        if (property.Value.Count != mutation.ExpectedEntries.Count)
            throw new InvalidDataException(
                $"Map '{mutation.Property}' does not have the expected entry count.");
        var actual = new Dictionary<string, (string Kind, string Name)>(StringComparer.Ordinal);
        foreach (KeyValuePair<PropertyData, PropertyData> entry in property.Value)
        {
            if (entry.Key is not NamePropertyData key ||
                entry.Value is not ObjectPropertyData objectReference)
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' is not a NameProperty/ObjectProperty map.");
            string kind;
            string objectName;
            if (objectReference.IsImport())
            {
                kind = "import";
                objectName = objectReference.ToImport(asset).ObjectName.ToString();
            }
            else if (objectReference.IsExport())
            {
                kind = "export";
                objectName = objectReference.ToExport(asset).ObjectName.ToString();
            }
            else
            {
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' contains a null object reference.");
            }
            if (!actual.TryAdd(key.Value.ToString(), (kind, objectName)))
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' contains a duplicate key '{key.Value}'.");
        }
        foreach (NameObjectMapEntry expectedEntry in mutation.ExpectedEntries)
        {
            if (string.IsNullOrWhiteSpace(expectedEntry.Key) ||
                string.IsNullOrWhiteSpace(expectedEntry.ObjectName))
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' contains an incomplete expected entry.");
            if (expectedEntry.ReferenceKind is not ("import" or "export"))
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' has unsupported reference kind " +
                    $"'{expectedEntry.ReferenceKind}'.");
            if (!actual.Remove(expectedEntry.Key, out var actualEntry) ||
                actualEntry.Kind != expectedEntry.ReferenceKind ||
                actualEntry.Name != expectedEntry.ObjectName)
                throw new InvalidDataException(
                    $"Map '{mutation.Property}' entry '{expectedEntry.Key}' does not match.");
        }
        if (actual.Count != 0)
            throw new InvalidDataException($"Map '{mutation.Property}' contains unexpected entries.");
    }
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

sealed class ItemPatchSpecification
{
    public int SchemaVersion { get; init; }
    public required string ItemObjectName { get; init; }
    public int ExpectedExportCount { get; init; }
    public required ExpectedItemState Expected { get; init; }
    public required ItemMutation Patch { get; init; }
}

sealed class ExpectedItemState
{
    public float Weight { get; init; }
    public float CraftTime { get; init; }
    public float CraftElectricityCost { get; init; }
    public int? CraftAmount { get; init; }
    public required string NameStringTableKey { get; init; }
    public required string DescriptionStringTableKey { get; init; }
    public required AssetReference Icon { get; init; }
    public required List<RecipeComponent> Components { get; init; }
}

sealed class ItemMutation
{
    public float CraftTime { get; init; }
    public int? CraftAmount { get; init; }
    public required ItemTextPatch Name { get; init; }
    public required ItemTextPatch Description { get; init; }
    public required AssetReference Icon { get; init; }
    public required List<ComponentReplacement> ComponentReplacements { get; init; }
    public List<RecipeComponent>? Components { get; init; }
    public List<SoftObjectReferenceMutation> SoftObjectReferences { get; init; } = [];
    public List<MapClearMutation> ClearNameObjectMaps { get; init; } = [];
}

sealed class ItemTextPatch
{
    public required string Key { get; init; }
    public required string Text { get; init; }
}

sealed class RecipeComponent
{
    public required AssetReference Material { get; init; }
    public int Amount { get; init; }
}

sealed class ComponentReplacement
{
    public required AssetReference From { get; init; }
    public required AssetReference To { get; init; }
}

sealed class AssetReference
{
    public required string Package { get; init; }
    public required string Name { get; init; }
    public required string ClassName { get; init; }
}

sealed class SoftObjectReferenceMutation
{
    public required string Property { get; init; }
    public required SoftObjectReference Expected { get; init; }
    public required SoftObjectReference Value { get; init; }
}

sealed class SoftObjectReference
{
    public required string PackageName { get; init; }
    public required string AssetName { get; init; }
    public string? SubPathString { get; init; }
}

sealed class MapClearMutation
{
    public required string Property { get; init; }
    public required string KeyType { get; init; }
    public required string ValueType { get; init; }
    public required List<NameObjectMapEntry> ExpectedEntries { get; init; }
}

sealed class NameObjectMapEntry
{
    public required string Key { get; init; }
    public required string ReferenceKind { get; init; }
    public required string ObjectName { get; init; }
}

using System.Collections;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text.Json;
using UAssetAPI;
using UAssetAPI.ExportTypes;
using UAssetAPI.UnrealTypes;
using UAssetAPI.Unversioned;

internal static class Program
{
    private const string ForkliftClassName = "BP_Forklift_Possesable_C";
    private static readonly string[] FunctionNames = ["ReceiveBeginPlay", "GetProvidedActionsBP"];

    private static int Main(string[] args)
    {
        try
        {
            Dictionary<string, string> options = ParseOptions(args);
            string stockPath = Required(options, "stock");
            string donorPath = Required(options, "donor");
            string donorParentPath = Required(options, "donor-parent");
            string mappingsPath = Required(options, "mappings");
            string outputPath = Required(options, "output");
            string reportPath = Required(options, "report");

            Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
            Directory.CreateDirectory(Path.GetDirectoryName(reportPath)!);

            var stockMappings = new Usmap(mappingsPath);
            // The source is freshly extracted with the reviewed UE5_8 profile,
            // so the canonical reader and writer retain its expanded imports.
            var stock = new UAsset(stockPath, EngineVersion.VER_UE5_8, stockMappings);
            Console.WriteLine("Stock package parsed.");
            stock.SetSerializationEngineVersion(EngineVersion.VER_UE5_8);
            RequireBinaryEquality(stock, "Stock");
            Console.WriteLine("Direct stock binary-equality gate passed.");

            // UAssetAPI augments a Usmap while parsing generated classes. The
            // stock and donor intentionally share a class name but not a
            // schema, so each asset must receive an isolated mapping instance.
            var donorMappings = new Usmap(mappingsPath);
            _ = new UAsset(donorParentPath, EngineVersion.VER_UE5_8, donorMappings);
            Console.WriteLine("Donor parent schema parsed into isolated context.");
            var donor = new UAsset(donorPath, EngineVersion.VER_UE5_8, donorMappings);
            Console.WriteLine("Donor package parsed.");
            donor.SetSerializationEngineVersion(EngineVersion.VER_UE5_8);
            int donorClassIndex = FindUniqueClass(donor);
            Dictionary<string, int> donorFunctions = FunctionNames.ToDictionary(
                name => name,
                name => FindUniqueFunction(donor, name),
                StringComparer.Ordinal);
            foreach (int functionIndex in donorFunctions.Values) RequireExportBinaryEquality(donor, donorPath, functionIndex);
            Console.WriteLine("Donor function payload binary-equality gates passed.");

            int originalExportCount = stock.Exports.Count;
            int originalImportCount = stock.Imports.Count;
            string[] originalNames = stock.GetNameMapIndexList().Select(value => value.Value).ToArray();
            string[] originalExports = stock.Exports.Select(ExportIdentity).ToArray();
            (long Offset, long Size)[] originalPayloadRanges = stock.Exports.Select(export => (export.SerialOffset, export.SerialSize)).ToArray();

            int stockClassIndex = FindUniqueClass(stock);
            string[] stockRawExports = stock.Exports.Where(export => export is RawExport).Select(export => export.ObjectName.ToString()).ToArray();

            var assembler = new FunctionAssembler(stock, donor, stockClassIndex, donorClassIndex, donorFunctions.Values);
            IReadOnlyDictionary<string, int> appended = assembler.AppendFunctions(donorFunctions);
            Console.WriteLine("Function graphs and dependencies remapped.");

            var stockClass = (ClassExport)stock.Exports[stockClassIndex];
            var children = stockClass.Children.ToList();
            foreach (string name in FunctionNames)
            {
                int exportIndex = appended[name];
                var packageIndex = FPackageIndex.FromExport(exportIndex);
                children.Add(packageIndex);
                stockClass.FuncMap.Add(new FName(stock, name), packageIndex);
                int donorFunctionIndex = donorFunctions[name];
                var donorClass = (ClassExport)donor.Exports[donorClassIndex];
                Require(donorClass.CreateBeforeSerializationDependencies.Any(dependency => dependency.Index == donorFunctionIndex + 1),
                    $"Donor class lacks the required CreateBeforeSerialization edge to {name}.");
                stockClass.CreateBeforeSerializationDependencies.Add(packageIndex);

                var function = (FunctionExport)stock.Exports[exportIndex];
                Require(!function.FunctionFlags.HasFlag(EFunctionFlags.FUNC_Native), $"{name} unexpectedly has FUNC_Native.");
                Require(function.CreateBeforeCreateDependencies.Any(dependency => dependency.Index == stockClassIndex + 1),
                    $"{name} does not retain class-owner CreateBeforeCreate dependency.");
            }
            stockClass.Children = children.ToArray();

            Require(stock.Exports.Count == originalExportCount + FunctionNames.Length, "Unexpected merged export count.");
            Require(stock.Exports.Take(originalExportCount).Select(ExportIdentity).SequenceEqual(originalExports),
                "An original export identity changed in memory.");
            Require(stock.GetNameMapIndexList().Take(originalNames.Length).Select(value => value.Value).SequenceEqual(originalNames),
                "An original name-map entry changed or moved.");
            Require(stockClass.LoadedProperties.Length == 54, "Stock class own-property count changed.");
            Require(!stock.GetNameMapIndexList().Any(name => name.Value.Contains("UberGraph", StringComparison.OrdinalIgnoreCase)), "Merged name map contains an UberGraph symbol.");
            Require(!stock.Imports.Select((_, index) => ImportPath(stock, index)).Any(path => path.Contains("/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original", StringComparison.Ordinal)),
                "Merged package retains a relocated-parent import.");

            stock.Write(outputPath);
            Console.WriteLine("Merged package written.");
            var reopened = new UAsset(outputPath, EngineVersion.VER_UE5_8, new Usmap(mappingsPath));
            reopened.SetSerializationEngineVersion(EngineVersion.VER_UE5_8);
            Require(reopened.VerifyBinaryEquality(), "Merged package does not losslessly reopen/write.");
            ValidateReopened(reopened, originalExportCount, originalImportCount, originalNames, originalExports, stockRawExports);
            RequireOriginalPayloadEquality(stock, stockPath, originalPayloadRanges, reopened, outputPath, stockClassIndex, originalExportCount);

            var report = new
            {
                schemaVersion = 1,
                stock = new { path = Path.GetFullPath(stockPath), length = new FileInfo(stockPath).Length, exports = originalExportCount, imports = originalImportCount, names = originalNames.Length, rawExportCount = stockRawExports.Length, binaryEquality = true },
                donor = new { path = Path.GetFullPath(donorPath), length = new FileInfo(donorPath).Length, functionPayloadBinaryEquality = true },
                output = new { path = Path.GetFullPath(outputPath), length = new FileInfo(outputPath).Length, uexpLength = new FileInfo(Path.ChangeExtension(outputPath, "uexp")).Length },
                originalPrefix = new { exportIdentityPreserved = true, nameOrderPreserved = true, unchangedExportPayloadsBinaryEqual = true },
                merged = new
                {
                    exports = reopened.Exports.Count,
                    imports = reopened.Imports.Count,
                    names = reopened.GetNameMapIndexList().Count,
                    classIndex = stockClassIndex,
                    classOwnProperties = ((ClassExport)reopened.Exports[stockClassIndex]).LoadedProperties.Length,
                    rawExportCount = reopened.Exports.Count(export => export is RawExport),
                    functions = appended.OrderBy(pair => pair.Key).ToDictionary(pair => pair.Key, pair => pair.Value + 1),
                    eventDrivenLoader = appended.OrderBy(pair => pair.Key).ToDictionary(
                        pair => pair.Key,
                        pair => new
                        {
                            classCreateBeforeSerialization = ((ClassExport)reopened.Exports[stockClassIndex]).CreateBeforeSerializationDependencies.Any(dependency => dependency.Index == pair.Value + 1),
                            functionOwnerCreateBeforeCreate = reopened.Exports[pair.Value].CreateBeforeCreateDependencies.Any(dependency => dependency.Index == stockClassIndex + 1)
                        }),
                    copiedImports = assembler.CopiedImportCount
                }
            };
            File.WriteAllText(reportPath, JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true }));
            Console.WriteLine(reportPath);
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 1;
        }
    }

    private static void ValidateReopened(UAsset asset, int originalExportCount, int originalImportCount, string[] originalNames, string[] originalExports, string[] originalRawExports)
    {
        Require(asset.Exports.Count == originalExportCount + FunctionNames.Length, "Reopened export count mismatch.");
        Require(asset.Imports.Count >= originalImportCount, "Reopened import table lost stock entries.");
        Require(asset.Exports.Take(originalExportCount).Select(ExportIdentity).SequenceEqual(originalExports), "Reopened original export identities changed.");
        Require(asset.GetNameMapIndexList().Take(originalNames.Length).Select(value => value.Value).SequenceEqual(originalNames), "Reopened original name-map order changed.");
        string[] reopenedRawExports = asset.Exports.Where(export => export is RawExport).Select(export => export.ObjectName.ToString()).ToArray();
        Require(reopenedRawExports.SequenceEqual(originalRawExports), $"RawExport set changed; stock=[{string.Join(", ", originalRawExports)}], merged=[{string.Join(", ", reopenedRawExports)}].");
        Require(!asset.Imports.Select((_, index) => ImportPath(asset, index)).Any(path => path.Contains("/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original", StringComparison.Ordinal)),
            "Reopened package retains a relocated-parent import.");
        int classIndex = FindUniqueClass(asset);
        var classExport = (ClassExport)asset.Exports[classIndex];
        Require(classExport.LoadedProperties.Length == 54, "Reopened stock class own-property count changed.");
        foreach (string name in FunctionNames)
        {
            int functionIndex = FindUniqueFunction(asset, name);
            Require(classExport.Children.Any(index => index.Index == functionIndex + 1), $"Class Children omits {name}.");
            Require(classExport.FuncMap.Any(pair => pair.Key.ToString() == name && pair.Value.Index == functionIndex + 1), $"Class FuncMap omits {name}.");
            var function = (FunctionExport)asset.Exports[functionIndex];
            Require(function.OuterIndex.Index == classIndex + 1, $"{name} is not owned by the stock class.");
            Require(function.ScriptBytecode is { Length: > 0 }, $"{name} has no parsed bytecode.");
            Require(!function.FunctionFlags.HasFlag(EFunctionFlags.FUNC_Native), $"{name} unexpectedly has FUNC_Native.");
            Require(classExport.CreateBeforeSerializationDependencies.Any(dependency => dependency.Index == functionIndex + 1), $"Class lacks CreateBeforeSerialization edge to {name}.");
            Require(function.CreateBeforeCreateDependencies.Any(dependency => dependency.Index == classIndex + 1), $"{name} lacks owner-class CreateBeforeCreate edge.");
        }
    }

    private static void RequireOriginalPayloadEquality(UAsset stock, string stockPath, (long Offset, long Size)[] originalRanges, UAsset merged, string mergedPath, int changedClassIndex, int originalExportCount)
    {
        byte[] stockBytes = ReadCombined(stockPath);
        byte[] mergedBytes = ReadCombined(mergedPath);
        for (int index = 0; index < originalExportCount; index++)
        {
            if (index == changedClassIndex) continue;
            Export stockExport = stock.Exports[index];
            Export mergedExport = merged.Exports[index];
            ReadOnlySpan<byte> stockPayload = stockBytes.AsSpan(checked((int)originalRanges[index].Offset), checked((int)originalRanges[index].Size));
            ReadOnlySpan<byte> mergedPayload = mergedBytes.AsSpan(checked((int)mergedExport.SerialOffset), checked((int)mergedExport.SerialSize));
            Require(stockPayload.SequenceEqual(mergedPayload), $"Original export payload changed at index {index + 1} ({stockExport.ObjectName}).");
        }
    }

    private static string ImportPath(UAsset asset, int importIndex)
    {
        var segments = new List<string>();
        var seen = new HashSet<int>();
        int current = importIndex;
        while (current >= 0)
        {
            Require(seen.Add(current), $"Import outer cycle at {current}.");
            Require(current < asset.Imports.Count, $"Import outer index {current} is out of range.");
            Import item = asset.Imports[current];
            segments.Add(item.ObjectName.ToString());
            if (!item.OuterIndex.IsImport()) break;
            current = -item.OuterIndex.Index - 1;
        }
        segments.Reverse();
        return string.Join('.', segments);
    }

    private static int FindUniqueClass(UAsset asset)
    {
        int[] matches = asset.Exports.Select((export, index) => (export, index))
            .Where(item => item.export is ClassExport && item.export.ObjectName.ToString() == ForkliftClassName)
            .Select(item => item.index).ToArray();
        Require(matches.Length == 1, $"Expected one {ForkliftClassName} class export; found {matches.Length}.");
        return matches[0];
    }

    private static int FindUniqueFunction(UAsset asset, string name)
    {
        int[] matches = asset.Exports.Select((export, index) => (export, index))
            .Where(item => item.export is FunctionExport && item.export.ObjectName.ToString() == name)
            .Select(item => item.index).ToArray();
        Require(matches.Length == 1, $"Expected one {name} FunctionExport; found {matches.Length}.");
        return matches[0];
    }

    private static string ExportIdentity(Export export) => $"{export.GetType().FullName}|{export.ObjectName}|{export.OuterIndex.Index}|{export.ClassIndex.Index}|{export.SuperIndex.Index}|{export.TemplateIndex.Index}|{(uint)export.ObjectFlags}|{(uint)export.PackageFlags}|{export.GeneratePublicHash}";
    private static void RequireBinaryEquality(UAsset asset, string label)
    {
        if (asset.VerifyBinaryEquality()) return;
        BinaryEqualityFailure failure = asset.LastBinaryEqualityFailure;
        throw new InvalidDataException($"{label} package is not losslessly writable: originalLength={failure.OriginalLength}, serializedLength={failure.SerializedLength}, offset={failure.FirstDifferentOffset}, exportIndex={failure.ExportIndex}, export={failure.ExportObjectName ?? "<header>"}, originalByte={failure.OriginalByte?.ToString() ?? "<none>"}, serializedByte={failure.SerializedByte?.ToString() ?? "<none>"}.");
    }

    private static void RequireExportBinaryEquality(UAsset asset, string assetPath, int exportIndex)
    {
        Export export = asset.Exports[exportIndex];
        using var stream = new MemoryStream();
        using (var writer = new AssetBinaryWriter(stream, asset))
        {
            export.Write(writer);
            writer.Write(export.Extras);
        }
        byte[] serialized = stream.ToArray();
        byte[] combined = ReadCombined(assetPath);
        byte[] original = combined.AsSpan(checked((int)export.SerialOffset), checked((int)export.SerialSize)).ToArray();
        Require(serialized.SequenceEqual(original), $"Donor function {export.ObjectName} does not serialize byte-identically (original {original.Length}, serialized {serialized.Length}).");
    }

    private static byte[] ReadCombined(string assetPath)
    {
        byte[] header = File.ReadAllBytes(assetPath);
        byte[] payload = File.ReadAllBytes(Path.ChangeExtension(assetPath, "uexp"));
        byte[] combined = new byte[header.Length + payload.Length];
        Buffer.BlockCopy(header, 0, combined, 0, header.Length);
        Buffer.BlockCopy(payload, 0, combined, header.Length, payload.Length);
        return combined;
    }
    private static void Require(bool condition, string message) { if (!condition) throw new InvalidDataException(message); }

    private static Dictionary<string, string> ParseOptions(string[] args)
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        for (int index = 0; index < args.Length; index += 2)
        {
            Require(index + 1 < args.Length && args[index].StartsWith("--", StringComparison.Ordinal), "Arguments must be --name value pairs.");
            result.Add(args[index][2..], args[index + 1]);
        }
        return result;
    }

    private static string Required(IReadOnlyDictionary<string, string> options, string name) =>
        options.TryGetValue(name, out string? value) && !string.IsNullOrWhiteSpace(value) ? Path.GetFullPath(value) : throw new ArgumentException($"Missing --{name}.");
}

internal sealed class FunctionAssembler
{
    private readonly UAsset stock;
    private readonly UAsset donor;
    private readonly int stockClassIndex;
    private readonly int donorClassIndex;
    private readonly HashSet<int> donorFunctionIndices;
    private readonly Dictionary<int, int> exportMap = [];
    private readonly Dictionary<int, int> importMap = [];
    private readonly HashSet<object> visited = new(ReferenceEqualityComparer.Instance);
    private int copiedImportCount;

    public int CopiedImportCount => copiedImportCount;

    public FunctionAssembler(UAsset stock, UAsset donor, int stockClassIndex, int donorClassIndex, IEnumerable<int> donorFunctionIndices)
    {
        this.stock = stock;
        this.donor = donor;
        this.stockClassIndex = stockClassIndex;
        this.donorClassIndex = donorClassIndex;
        this.donorFunctionIndices = donorFunctionIndices.ToHashSet();
        exportMap[donorClassIndex] = stockClassIndex;
        int next = stock.Exports.Count;
        foreach (int index in this.donorFunctionIndices.Order()) exportMap[index] = next++;
    }

    public IReadOnlyDictionary<string, int> AppendFunctions(IReadOnlyDictionary<string, int> functions)
    {
        foreach (int donorIndex in donorFunctionIndices.Order())
        {
            var function = (FunctionExport)donor.Exports[donorIndex];
            RemapObject(function);
            function.Asset = stock;
            stock.Exports.Add(function);
        }
        return functions.ToDictionary(pair => pair.Key, pair => exportMap[pair.Value], StringComparer.Ordinal);
    }

    private object? RemapObject(object? value)
    {
        if (value is null) return null;
        if (value is FPackageIndex packageIndex) return RemapIndex(packageIndex);
        if (value is FName name) return name.Transfer(stock);
        Type type = value.GetType();
        if (type.IsPrimitive || type.IsEnum || value is string || value is decimal || value is Guid || value is FString || value is byte[]) return value;
        if (value is UAsset) return stock;
        if (!type.IsValueType && !visited.Add(value)) return value;

        if (value is Array array)
        {
            for (int index = 0; index < array.Length; index++) array.SetValue(RemapObject(array.GetValue(index)), index);
            return value;
        }
        if (value is IDictionary dictionary)
        {
            var entries = dictionary.Cast<DictionaryEntry>().Select(entry => (entry.Key, entry.Value)).ToArray();
            dictionary.Clear();
            foreach ((object key, object? itemValue) in entries) dictionary.Add(RemapObject(key)!, RemapObject(itemValue));
            return value;
        }
        if (value is IList list)
        {
            for (int index = 0; index < list.Count; index++) list[index] = RemapObject(list[index]);
            return value;
        }

        foreach (FieldInfo field in GetFields(type))
        {
            if (field.IsStatic || field.IsInitOnly || field.Name is "alreadySerialized") continue;
            object? current = field.GetValue(value);
            object? remapped = RemapObject(current);
            if (!ReferenceEquals(current, remapped)) field.SetValue(value, remapped);
        }
        return value;
    }

    private FPackageIndex RemapIndex(FPackageIndex index)
    {
        if (index.IsNull()) return new FPackageIndex();
        if (index.IsExport())
        {
            int donorIndex = index.Index - 1;
            if (!exportMap.TryGetValue(donorIndex, out int targetIndex))
            {
                string identity = donorIndex >= 0 && donorIndex < donor.Exports.Count ? donor.Exports[donorIndex].ObjectName.ToString() : "out-of-range";
                throw new InvalidDataException($"Function graph references unsupported donor export {index.Index} ({identity}).");
            }
            return FPackageIndex.FromExport(targetIndex);
        }
        int donorImportIndex = -index.Index - 1;
        return FPackageIndex.FromImport(MapImport(donorImportIndex));
    }

    private int MapImport(int donorImportIndex)
    {
        if (importMap.TryGetValue(donorImportIndex, out int mapped)) return mapped;
        RequireImportIndex(donorImportIndex);
        Import source = donor.Imports[donorImportIndex];
        FPackageIndex outer = source.OuterIndex.IsNull() ? new FPackageIndex() : RemapIndex(source.OuterIndex);
        string classPackage = source.ClassPackage.ToString();
        string className = source.ClassName.ToString();
        string objectName = source.ObjectName.ToString();
        string? packageName = source.PackageName?.ToString();

        for (int index = 0; index < stock.Imports.Count; index++)
        {
            Import candidate = stock.Imports[index];
            if (candidate.OuterIndex.Index == outer.Index && candidate.ClassPackage.ToString() == classPackage && candidate.ClassName.ToString() == className &&
                candidate.ObjectName.ToString() == objectName && candidate.PackageName?.ToString() == packageName && candidate.bImportOptional == source.bImportOptional)
            {
                importMap[donorImportIndex] = index;
                return index;
            }
        }

        var copy = new Import
        {
            ClassPackage = source.ClassPackage.Transfer(stock),
            ClassName = source.ClassName.Transfer(stock),
            ObjectName = source.ObjectName.Transfer(stock),
            PackageName = source.PackageName?.Transfer(stock),
            OuterIndex = outer,
            bImportOptional = source.bImportOptional
        };
        int newIndex = stock.Imports.Count;
        stock.Imports.Add(copy);
        copiedImportCount++;
        importMap[donorImportIndex] = newIndex;
        return newIndex;
    }

    private void RequireImportIndex(int index)
    {
        if (index < 0 || index >= donor.Imports.Count) throw new InvalidDataException($"Donor import index {index} is out of range.");
    }

    private static IEnumerable<FieldInfo> GetFields(Type type)
    {
        for (Type? current = type; current is not null; current = current.BaseType)
        {
            foreach (FieldInfo field in current.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly)) yield return field;
        }
    }
}

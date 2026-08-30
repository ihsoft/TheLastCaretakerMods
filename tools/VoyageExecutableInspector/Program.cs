using System.Buffers.Binary;
using System.Text;

if (args.Length < 3)
{
    Console.Error.WriteLine("Usage: VoyageExecutableInspector <exe> <output.txt> [--window=bytes] [--member-offsets=0x10,0x18] [--target-va=0x140001000] [query ...]");
    return 2;
}

var executablePath = Path.GetFullPath(args[0]);
var outputPath = Path.GetFullPath(args[1]);
var window = 0x800;
var queries = new List<string>();
var memberOffsets = new List<uint>();
var targetVirtualAddresses = new List<ulong>();
foreach (var argument in args[2..])
{
    if (argument.StartsWith("--window=", StringComparison.OrdinalIgnoreCase))
    {
        window = int.Parse(argument["--window=".Length..]);
    }
    else if (argument.StartsWith("--member-offsets=", StringComparison.OrdinalIgnoreCase))
    {
        foreach (var value in argument["--member-offsets=".Length..].Split(',', StringSplitOptions.RemoveEmptyEntries))
        {
            memberOffsets.Add(Convert.ToUInt32(value.Trim().Replace("0x", "", StringComparison.OrdinalIgnoreCase), 16));
        }
    }
    else if (argument.StartsWith("--target-va=", StringComparison.OrdinalIgnoreCase))
    {
        foreach (var value in argument["--target-va=".Length..].Split(',', StringSplitOptions.RemoveEmptyEntries))
        {
            targetVirtualAddresses.Add(Convert.ToUInt64(value.Trim().Replace("0x", "", StringComparison.OrdinalIgnoreCase), 16));
        }
    }
    else
    {
        queries.Add(argument);
    }
}
if (queries.Count == 0 && memberOffsets.Count == 0 && targetVirtualAddresses.Count == 0)
    throw new ArgumentException("At least one query, member offset, or target address is required.");

using var stream = new FileStream(
    executablePath,
    FileMode.Open,
    FileAccess.Read,
    FileShare.ReadWrite | FileShare.Delete);
if (stream.Length > int.MaxValue)
{
    throw new InvalidOperationException("Executables larger than 2 GiB are not supported.");
}

var image = new byte[stream.Length];
stream.ReadExactly(image);
var pe = PeImage.Parse(image);
var strings = ExtractAsciiStrings(image, minimumLength: 4)
    .Concat(ExtractUtf16Strings(image, minimumLength: 4))
    .OrderBy(item => item.Offset)
    .ToArray();
var matches = strings
    .Where(item => queries.Any(query => item.Value.Contains(query, StringComparison.OrdinalIgnoreCase)))
    .ToArray();

Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
using var writer = new StreamWriter(outputPath, false, new UTF8Encoding(false));
writer.WriteLine($"FILE\t{executablePath}");
writer.WriteLine($"SIZE\t0x{image.Length:X}");
writer.WriteLine($"IMAGE_BASE\t0x{pe.ImageBase:X}");
writer.WriteLine($"QUERIES\t{string.Join(" | ", queries)}");
writer.WriteLine($"STRING_WINDOW\t0x{window:X}");
writer.WriteLine($"MEMBER_OFFSETS\t{string.Join(" | ", memberOffsets.Select(value => $"0x{value:X}"))}");
writer.WriteLine($"TARGET_VIRTUAL_ADDRESSES\t{string.Join(" | ", targetVirtualAddresses.Select(value => $"0x{value:X}"))}");
writer.WriteLine($"MATCHES\t{matches.Length}");

foreach (var match in matches)
{
    var matchRva = pe.FileOffsetToRva(match.Offset);
    writer.WriteLine();
    writer.WriteLine($"MATCH\tFILE=0x{match.Offset:X}\tRVA=0x{matchRva:X}\tVA=0x{pe.ImageBase + matchRva:X}\tENCODING={match.Encoding}\t{match.Value}");

    writer.WriteLine("NEARBY_STRINGS");
    foreach (var item in strings.Where(item => Math.Abs((long)item.Offset - match.Offset) <= window))
    {
        writer.WriteLine($"  FILE=0x{item.Offset:X}\tRVA=0x{pe.FileOffsetToRva(item.Offset):X}\tENCODING={item.Encoding}\t{item.Value}");
    }

    var virtualAddress = pe.ImageBase + matchRva;
    var pointerBytes = BitConverter.GetBytes(virtualAddress);
    var pointerOffsets = FindAll(image, pointerBytes).ToArray();
    writer.WriteLine($"ABSOLUTE_POINTERS\t{pointerOffsets.Length}");
    foreach (var pointerOffset in pointerOffsets.Take(64))
    {
        writer.WriteLine($"  FILE=0x{pointerOffset:X}\tRVA=0x{pe.FileOffsetToRva(pointerOffset):X}");
        WriteHexWindow(writer, image, pointerOffset, 0x40);
    }

    var ripReferences = FindRipRelativeReferences(image, pe, matchRva).ToArray();
    writer.WriteLine($"RIP_RELATIVE_REFERENCES\t{ripReferences.Length}");
    foreach (var reference in ripReferences.Take(64))
    {
        writer.WriteLine($"  FILE=0x{reference:X}\tRVA=0x{pe.FileOffsetToRva(reference):X}");
        WriteHexWindow(writer, image, reference, 0x20);
    }
}

foreach (var virtualAddress in targetVirtualAddresses)
{
    if (virtualAddress < pe.ImageBase || virtualAddress - pe.ImageBase > uint.MaxValue)
        throw new ArgumentOutOfRangeException(nameof(targetVirtualAddresses), $"VA 0x{virtualAddress:X} is outside this image.");

    var targetRva = checked((uint)(virtualAddress - pe.ImageBase));
    writer.WriteLine();
    writer.WriteLine($"TARGET_VA\tVA=0x{virtualAddress:X}\tRVA=0x{targetRva:X}");

    var pointerOffsets = FindAll(image, BitConverter.GetBytes(virtualAddress)).ToArray();
    writer.WriteLine($"ABSOLUTE_POINTERS\t{pointerOffsets.Length}");
    foreach (var pointerOffset in pointerOffsets.Take(128))
    {
        writer.WriteLine($"  FILE=0x{pointerOffset:X}\tRVA=0x{pe.FileOffsetToRva(pointerOffset):X}");
        WriteHexWindow(writer, image, pointerOffset, 0x40);
    }

    var ripReferences = FindRipRelativeReferences(image, pe, targetRva).ToArray();
    writer.WriteLine($"RIP_RELATIVE_REFERENCES\t{ripReferences.Length}");
    foreach (var reference in ripReferences.Take(128))
    {
        writer.WriteLine($"  FILE=0x{reference:X}\tRVA=0x{pe.FileOffsetToRva(reference):X}");
        WriteHexWindow(writer, image, reference, 0x20);
    }
}

if (memberOffsets.Count > 0)
{
    var occurrences = FindMemberOffsetOccurrences(image, pe, memberOffsets).ToArray();
    writer.WriteLine();
    writer.WriteLine($"MEMBER_OFFSET_OCCURRENCES\t{occurrences.Length}");
    foreach (var group in occurrences
        .GroupBy(item => item.Rva >> 12)
        .Select(group => new { Group = group.OrderBy(item => item.Rva).ToArray(), Distinct = group.Select(item => item.MemberOffset).Distinct().Count() })
        .Where(item => item.Distinct >= 3)
        .OrderByDescending(item => item.Distinct)
        .ThenBy(item => item.Group[0].Rva))
    {
        writer.WriteLine($"GROUP\tPAGE_RVA=0x{(group.Group[0].Rva & 0xFFFFF000):X}\tDISTINCT={group.Distinct}\tCOUNT={group.Group.Length}");
        foreach (var item in group.Group)
        {
            writer.WriteLine($"  MEMBER=0x{item.MemberOffset:X}\tFILE=0x{item.FileOffset:X}\tRVA=0x{item.Rva:X}");
            WriteHexWindow(writer, image, item.FileOffset, 0x10);
        }
    }
}

Console.WriteLine($"Wrote {matches.Length} string match(es): {outputPath}");
return matches.Length == 0 && memberOffsets.Count == 0 && targetVirtualAddresses.Count == 0 ? 1 : 0;

static List<ExtractedString> ExtractAsciiStrings(byte[] image, int minimumLength)
{
    var result = new List<ExtractedString>();
    var start = -1;
    for (var index = 0; index <= image.Length; index++)
    {
        var printable = index < image.Length && image[index] is >= 0x20 and <= 0x7E;
        if (printable)
        {
            if (start < 0) start = index;
            continue;
        }

        if (start >= 0 && index - start >= minimumLength)
        {
            result.Add(new ExtractedString(start, Encoding.ASCII.GetString(image, start, index - start), "ASCII"));
        }
        start = -1;
    }
    return result;
}

static List<ExtractedString> ExtractUtf16Strings(byte[] image, int minimumLength)
{
    var result = new List<ExtractedString>();
    for (var parity = 0; parity < 2; parity++)
    {
        var start = -1;
        var length = 0;
        for (var offset = parity; offset + 1 <= image.Length; offset += 2)
        {
            var printable = offset + 1 < image.Length && image[offset] is >= 0x20 and <= 0x7E && image[offset + 1] == 0;
            if (printable)
            {
                if (start < 0) start = offset;
                length++;
                continue;
            }

            if (start >= 0 && length >= minimumLength)
            {
                result.Add(new ExtractedString(start, Encoding.Unicode.GetString(image, start, length * 2), "UTF16LE"));
            }
            start = -1;
            length = 0;
        }
    }
    return result;
}

static IEnumerable<int> FindAll(byte[] haystack, byte[] needle)
{
    for (var offset = 0; offset <= haystack.Length - needle.Length; offset++)
    {
        if (haystack.AsSpan(offset, needle.Length).SequenceEqual(needle)) yield return offset;
    }
}

static IEnumerable<int> FindRipRelativeReferences(byte[] image, PeImage pe, uint targetRva)
{
    foreach (var section in pe.Sections.Where(section => section.IsExecutable))
    {
        var start = checked((int)section.RawOffset);
        var end = checked((int)Math.Min((long)image.Length, section.RawOffset + section.RawSize));
        for (var offset = start; offset + 7 <= end; offset++)
        {
            var rex = image[offset];
            var opcode = image[offset + 1];
            var modRm = image[offset + 2];
            if (rex is < 0x40 or > 0x4F || (opcode != 0x8D && opcode != 0x8B) || (modRm & 0xC7) != 0x05)
            {
                continue;
            }

            var displacement = BinaryPrimitives.ReadInt32LittleEndian(image.AsSpan(offset + 3, 4));
            var instructionRva = pe.FileOffsetToRva(offset);
            var referencedRva = unchecked((uint)(instructionRva + 7 + displacement));
            if (referencedRva == targetRva) yield return offset;
        }
    }
}

static IEnumerable<MemberOffsetOccurrence> FindMemberOffsetOccurrences(byte[] image, PeImage pe, IEnumerable<uint> memberOffsets)
{
    foreach (var section in pe.Sections.Where(section => section.IsExecutable))
    {
        var start = checked((int)section.RawOffset);
        var end = checked((int)Math.Min((long)image.Length, section.RawOffset + section.RawSize));
        foreach (var memberOffset in memberOffsets.Distinct())
        {
            var needle = BitConverter.GetBytes(memberOffset);
            for (var offset = start; offset + 4 <= end; offset++)
            {
                if (image.AsSpan(offset, 4).SequenceEqual(needle) && LooksLikeMemoryDisplacement(image, offset))
                {
                    yield return new MemberOffsetOccurrence(offset, pe.FileOffsetToRva(offset), memberOffset);
                }
            }
        }
    }
}

static bool LooksLikeMemoryDisplacement(byte[] image, int displacementOffset)
{
    if (displacementOffset >= 1)
    {
        var modRm = image[displacementOffset - 1];
        if ((modRm & 0xC0) == 0x80 && (modRm & 0x07) != 0x04) return true;
    }
    if (displacementOffset >= 2)
    {
        var modRm = image[displacementOffset - 2];
        if ((modRm & 0xC0) == 0x80 && (modRm & 0x07) == 0x04) return true;
    }
    return false;
}

static void WriteHexWindow(TextWriter writer, byte[] image, int center, int radius)
{
    var start = Math.Max(0, center - radius);
    var end = Math.Min(image.Length, center + radius);
    for (var offset = start; offset < end; offset += 16)
    {
        var length = Math.Min(16, end - offset);
        writer.Write($"    {offset:X8}  ");
        for (var index = 0; index < 16; index++)
        {
            writer.Write(index < length ? $"{image[offset + index]:X2} " : "   ");
        }
        writer.Write(" ");
        for (var index = 0; index < length; index++)
        {
            var value = image[offset + index];
            writer.Write(value is >= 0x20 and <= 0x7E ? (char)value : '.');
        }
        writer.WriteLine();
    }
}

internal sealed record ExtractedString(int Offset, string Value, string Encoding);
internal sealed record MemberOffsetOccurrence(int FileOffset, uint Rva, uint MemberOffset);

internal sealed class PeImage
{
    private PeImage(ulong imageBase, IReadOnlyList<PeSection> sections)
    {
        ImageBase = imageBase;
        Sections = sections;
    }

    public ulong ImageBase { get; }
    public IReadOnlyList<PeSection> Sections { get; }

    public static PeImage Parse(byte[] image)
    {
        if (image.Length < 0x100 || image[0] != 'M' || image[1] != 'Z')
            throw new InvalidDataException("Not a PE executable.");
        var peOffset = BinaryPrimitives.ReadInt32LittleEndian(image.AsSpan(0x3C, 4));
        if (!image.AsSpan(peOffset, 4).SequenceEqual("PE\0\0"u8))
            throw new InvalidDataException("Invalid PE signature.");

        var coff = peOffset + 4;
        var sectionCount = BinaryPrimitives.ReadUInt16LittleEndian(image.AsSpan(coff + 2, 2));
        var optionalSize = BinaryPrimitives.ReadUInt16LittleEndian(image.AsSpan(coff + 16, 2));
        var optional = coff + 20;
        var magic = BinaryPrimitives.ReadUInt16LittleEndian(image.AsSpan(optional, 2));
        if (magic != 0x20B) throw new InvalidDataException("Only PE32+ executables are supported.");
        var imageBase = BinaryPrimitives.ReadUInt64LittleEndian(image.AsSpan(optional + 24, 8));

        var sectionTable = optional + optionalSize;
        var sections = new List<PeSection>(sectionCount);
        for (var index = 0; index < sectionCount; index++)
        {
            var entry = sectionTable + index * 40;
            var nameLength = 0;
            while (nameLength < 8 && image[entry + nameLength] != 0) nameLength++;
            var name = Encoding.ASCII.GetString(image, entry, nameLength);
            var virtualSize = BinaryPrimitives.ReadUInt32LittleEndian(image.AsSpan(entry + 8, 4));
            var virtualAddress = BinaryPrimitives.ReadUInt32LittleEndian(image.AsSpan(entry + 12, 4));
            var rawSize = BinaryPrimitives.ReadUInt32LittleEndian(image.AsSpan(entry + 16, 4));
            var rawOffset = BinaryPrimitives.ReadUInt32LittleEndian(image.AsSpan(entry + 20, 4));
            var characteristics = BinaryPrimitives.ReadUInt32LittleEndian(image.AsSpan(entry + 36, 4));
            sections.Add(new PeSection(name, virtualAddress, virtualSize, rawOffset, rawSize, (characteristics & 0x20000000) != 0));
        }
        return new PeImage(imageBase, sections);
    }

    public uint FileOffsetToRva(int fileOffset)
    {
        foreach (var section in Sections)
        {
            if (fileOffset >= section.RawOffset && fileOffset < section.RawOffset + section.RawSize)
                return section.VirtualAddress + checked((uint)(fileOffset - section.RawOffset));
        }
        return checked((uint)fileOffset);
    }
}

internal sealed record PeSection(
    string Name,
    uint VirtualAddress,
    uint VirtualSize,
    uint RawOffset,
    uint RawSize,
    bool IsExecutable);

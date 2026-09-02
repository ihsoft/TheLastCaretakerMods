param(
    [int]$TargetProcessId,

    [string]$ProcessName = 'VoyageSteam-Win64-Shipping'
)

$ErrorActionPreference = 'Stop'

$minimumObjectCount = 10000
$maximumObjectCount = 5000000
$maximumChunkCount = 1024
$elementsPerChunk = 65536
$processVmRead = 0x0010
$processQueryInformation = 0x0400

if (-not ('VoyageProcessMemory' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class VoyageProcessMemory
{
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(
        uint processAccess,
        bool inheritHandle,
        int processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool ReadProcessMemory(
        IntPtr process,
        IntPtr baseAddress,
        [Out] byte[] buffer,
        UIntPtr size,
        out UIntPtr bytesRead);

    [DllImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool CloseHandle(IntPtr handle);

    public static int[] FindChunkedObjectArrayOffsets(
        byte[] data,
        int minimumObjectCount,
        int maximumObjectCount,
        int maximumChunkCount,
        int elementsPerChunk)
    {
        var offsets = new List<int>();
        for (var offset = 0; offset <= data.Length - 0x20; offset += 8)
        {
            var objects = BitConverter.ToUInt64(data, offset);
            var numElements = BitConverter.ToInt32(data, offset + 8);
            var maxElements = BitConverter.ToInt32(data, offset + 12);
            var numChunks = BitConverter.ToInt32(data, offset + 16);
            var maxChunks = BitConverter.ToInt32(data, offset + 20);
            var preallocatedObjects = BitConverter.ToUInt64(data, offset + 24);

            if (numElements < minimumObjectCount || numElements > maximumObjectCount ||
                numChunks < 1 || numChunks > maximumChunkCount ||
                maxChunks < numChunks || maxChunks > maximumChunkCount ||
                maxElements != maxChunks * elementsPerChunk ||
                numChunks != (numElements + elementsPerChunk - 1) / elementsPerChunk ||
                !IsCanonicalPointer(objects) ||
                (preallocatedObjects != 0 && !IsCanonicalPointer(preallocatedObjects)))
            {
                continue;
            }

            offsets.Add(offset);
        }
        return offsets.ToArray();
    }

    private static bool IsCanonicalPointer(ulong value)
    {
        return value >= 0x10000 && value < 0x0000800000000000 && (value & 0x7) == 0;
    }
}
'@
}

function Read-RemoteBytes {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Handle,

        [Parameter(Mandatory = $true)]
        [UInt64]$Address,

        [Parameter(Mandatory = $true)]
        [int]$Length
    )

    $buffer = [byte[]]::new($Length)
    $bytesRead = [UIntPtr]::Zero
    $ok = [VoyageProcessMemory]::ReadProcessMemory(
        $Handle,
        [IntPtr]::new([Int64]$Address),
        $buffer,
        [UIntPtr]::new([UInt64]$Length),
        [ref]$bytesRead)
    if (-not $ok -or $bytesRead.ToUInt64() -ne [UInt64]$Length) {
        return $null
    }
    return ,$buffer
}

function Test-CanonicalPointer {
    param([UInt64]$Value)

    return $Value -ge 0x10000 -and $Value -lt 0x0000800000000000 -and
        ($Value -band 0x7) -eq 0
}

if ($TargetProcessId) {
    $target = Get-Process -Id $TargetProcessId -ErrorAction Stop
} else {
    $matches = @(Get-Process -Name $ProcessName -ErrorAction SilentlyContinue)
    if ($matches.Count -ne 1) {
        throw "Expected exactly one $ProcessName process, found $($matches.Count)."
    }
    $target = $matches[0]
}

$mainModule = $target.MainModule
$executable = $mainModule.FileName
$imageBase = [UInt64]$mainModule.BaseAddress.ToInt64()
$peBytes = [byte[]]::new(4096)
$peStream = [IO.File]::OpenRead($executable)
try {
    $peBytesRead = $peStream.Read($peBytes, 0, $peBytes.Length)
} finally {
    $peStream.Dispose()
}
if ($peBytesRead -lt $peBytes.Length) {
    [Array]::Resize([ref]$peBytes, $peBytesRead)
}
$peOffset = [BitConverter]::ToInt32($peBytes, 0x3c)
if ([Text.Encoding]::ASCII.GetString($peBytes, $peOffset, 4) -cne "PE`0`0") {
    throw "Invalid PE signature: $executable"
}

$sectionCount = [BitConverter]::ToUInt16($peBytes, $peOffset + 6)
$optionalHeaderSize = [BitConverter]::ToUInt16($peBytes, $peOffset + 20)
$sectionTableOffset = $peOffset + 24 + $optionalHeaderSize
if ($sectionTableOffset + ($sectionCount * 40) -gt $peBytes.Length) {
    throw "PE section table does not fit in the bounded header read: $executable"
}
$dataSection = $null
for ($index = 0; $index -lt $sectionCount; $index++) {
    $sectionOffset = $sectionTableOffset + ($index * 40)
    $name = [Text.Encoding]::ASCII.GetString($peBytes, $sectionOffset, 8).TrimEnd([char]0)
    if ($name -ceq '.data') {
        $virtualSize = [BitConverter]::ToUInt32($peBytes, $sectionOffset + 8)
        $virtualAddress = [BitConverter]::ToUInt32($peBytes, $sectionOffset + 12)
        $rawSize = [BitConverter]::ToUInt32($peBytes, $sectionOffset + 16)
        $dataSection = [pscustomobject]@{
            virtualAddress = [UInt64]$virtualAddress
            size = [int][Math]::Max($virtualSize, $rawSize)
        }
        break
    }
}
if (-not $dataSection) {
    throw "The executable has no .data section: $executable"
}

$handle = [VoyageProcessMemory]::OpenProcess(
    $processVmRead -bor $processQueryInformation,
    $false,
    $target.Id)
if ($handle -eq [IntPtr]::Zero) {
    $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
    throw "OpenProcess failed for PID $($target.Id) with Win32 error $code."
}

try {
    $sectionAddress = $imageBase + $dataSection.virtualAddress
    $sectionBytes = Read-RemoteBytes -Handle $handle -Address $sectionAddress -Length $dataSection.size
    if (-not $sectionBytes) {
        throw "Could not read the complete .data section at 0x$($sectionAddress.ToString('X'))."
    }

    $candidateOffsets = [VoyageProcessMemory]::FindChunkedObjectArrayOffsets(
        $sectionBytes,
        $minimumObjectCount,
        $maximumObjectCount,
        $maximumChunkCount,
        $elementsPerChunk)
    $candidates = [Collections.Generic.List[object]]::new()
    foreach ($offset in $candidateOffsets) {
        $objects = [BitConverter]::ToUInt64($sectionBytes, $offset)
        $numElements = [BitConverter]::ToInt32($sectionBytes, $offset + 8)
        $maxElements = [BitConverter]::ToInt32($sectionBytes, $offset + 12)
        $numChunks = [BitConverter]::ToInt32($sectionBytes, $offset + 16)
        $maxChunks = [BitConverter]::ToInt32($sectionBytes, $offset + 20)

        $chunkPointerBytes = Read-RemoteBytes -Handle $handle -Address $objects -Length 8
        if (-not $chunkPointerBytes) { continue }
        $firstChunk = [BitConverter]::ToUInt64($chunkPointerBytes, 0)
        if (-not (Test-CanonicalPointer $firstChunk)) { continue }

        $firstItemBytes = Read-RemoteBytes -Handle $handle -Address $firstChunk -Length 24
        if (-not $firstItemBytes) { continue }
        $firstObject = [BitConverter]::ToUInt64($firstItemBytes, 8)
        if (-not (Test-CanonicalPointer $firstObject)) { continue }

        $firstObjectBytes = Read-RemoteBytes -Handle $handle -Address $firstObject -Length 0x28
        if (-not $firstObjectBytes) { continue }
        $vtable = [BitConverter]::ToUInt64($firstObjectBytes, 0)
        $internalIndex = [BitConverter]::ToInt32($firstObjectBytes, 12)
        $classPrivate = [BitConverter]::ToUInt64($firstObjectBytes, 16)
        if (-not (Test-CanonicalPointer $vtable) -or
            -not (Test-CanonicalPointer $classPrivate) -or
            $internalIndex -ne 0) {
            continue
        }

        $candidates.Add([pscustomobject]@{
            processId = $target.Id
            executable = $executable
            imageBase = "0x$($imageBase.ToString('X'))"
            dataSectionAddress = "0x$($sectionAddress.ToString('X'))"
            guObjectArray = "0x$(($sectionAddress + [UInt64]$offset).ToString('X'))"
            objects = "0x$($objects.ToString('X'))"
            numElements = $numElements
            maxElements = $maxElements
            numChunks = $numChunks
            maxChunks = $maxChunks
            firstChunk = "0x$($firstChunk.ToString('X'))"
            firstObject = "0x$($firstObject.ToString('X'))"
        })
    }

    if ($candidates.Count -ne 1) {
        throw "Expected exactly one structurally valid GUObjectArray in .data, found $($candidates.Count)."
    }
    $candidates[0]
} finally {
    [void][VoyageProcessMemory]::CloseHandle($handle)
}

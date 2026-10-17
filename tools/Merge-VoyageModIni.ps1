[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$TemplatePath,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [string]$CurrentPath,
    [string[]]$RetiredKeys = @(),
    [string[]]$RetiredComments = @(),
    [string]$DefaultsComment = '# Defaults added by a newer mod build.'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$template = (Resolve-Path -LiteralPath $TemplatePath).Path
$output = [IO.Path]::GetFullPath($OutputPath)
if ($output.Equals($template, [StringComparison]::OrdinalIgnoreCase) -or
    ($CurrentPath -and $output.Equals([IO.Path]::GetFullPath($CurrentPath), [StringComparison]::OrdinalIgnoreCase))) {
    throw 'OutputPath must differ from both inputs.'
}
if (Test-Path -LiteralPath $output) { throw "Output already exists: $output" }
$null = New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force

if (-not $CurrentPath) {
    [IO.File]::WriteAllBytes($output, [IO.File]::ReadAllBytes($template))
    return [pscustomobject]@{ Created=$true; AddedKeys=@(); RemovedKeys=@(); Changed=$true; Sha256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash }
}
$current = (Resolve-Path -LiteralPath $CurrentPath).Path
$bytes = [IO.File]::ReadAllBytes($current)
$encoding = New-Object Text.UTF8Encoding($false, $true)
$preambleLength = 0
if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    $encoding = New-Object Text.UTF8Encoding($true, $true)
    $preambleLength = 3
} elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
    $encoding = New-Object Text.UnicodeEncoding($false, $true, $true)
    $preambleLength = 2
} elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
    $encoding = New-Object Text.UnicodeEncoding($true, $true, $true)
    $preambleLength = 2
}
$originalText = $encoding.GetString($bytes, $preambleLength, $bytes.Length - $preambleLength)
$newline = if ($originalText.Contains("`r`n")) { "`r`n" } elseif ($originalText.Contains("`n")) { "`n" } else { [Environment]::NewLine }
$retiredKeySet = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$retiredCommentSet = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($key in $RetiredKeys) { $null = $retiredKeySet.Add($key) }
foreach ($comment in $RetiredComments) { $null = $retiredCommentSet.Add($comment) }
$kept = New-Object 'System.Collections.Generic.List[string]'
$removed = New-Object 'System.Collections.Generic.List[string]'
$changed = $false
$headingSeen = $false
foreach ($line in [IO.File]::ReadAllLines($current)) {
    if ($retiredCommentSet.Contains($line.Trim())) { $changed = $true; continue }
    if ($line.Trim() -ceq $DefaultsComment) {
        if ($headingSeen) { $changed = $true; continue }
        $headingSeen = $true
    }
    if ($line -match '^\s*([^#;][^=]*?)\s*=' -and $retiredKeySet.Contains($matches[1].Trim())) {
        $key = $matches[1].Trim()
        if (-not $removed.Contains($key)) { $removed.Add($key) }
        $changed = $true
        continue
    }
    $kept.Add($line)
}
$existing = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($line in $kept) {
    if ($line -match '^\s*([^#;][^=]*?)\s*=') { $null = $existing.Add($matches[1].Trim()) }
}
$missing = New-Object 'System.Collections.Generic.List[string]'
$added = New-Object 'System.Collections.Generic.List[string]'
foreach ($line in [IO.File]::ReadAllLines($template)) {
    if ($line -match '^\s*([^#;][^=]*?)\s*=') {
        $key = $matches[1].Trim()
        if (-not $existing.Contains($key)) {
            $missing.Add($line)
            $added.Add($key)
            $null = $existing.Add($key)
        }
    }
}
if ($missing.Count -gt 0) { $changed = $true }
if (-not $changed) {
    [IO.File]::WriteAllBytes($output, [IO.File]::ReadAllBytes($current))
} else {
    $text = $kept -join $newline
    if ($kept.Count -gt 0) { $text += $newline }
    if ($missing.Count -gt 0) {
        $text += $newline
        if (-not $headingSeen) { $text += $DefaultsComment + $newline }
        $text += ($missing -join $newline) + $newline
    }
    [IO.File]::WriteAllText($output, $text, $encoding)
}
return [pscustomobject]@{
    Created=$false; AddedKeys=@($added); RemovedKeys=@($removed);
    Changed=$changed; Sha256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash
}

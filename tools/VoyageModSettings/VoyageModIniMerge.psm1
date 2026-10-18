Set-StrictMode -Version Latest

function Get-IniLineKey([string]$Line) {
    $trimmed = $Line.Trim()
    if (-not $trimmed -or $trimmed.StartsWith('#')) { return $null }
    $separator = $Line.IndexOf('=')
    if ($separator -lt 0) { return $null }
    $key = $Line.Substring(0, $separator).Trim()
    if ($key -cnotmatch '^[A-Za-z][A-Za-z0-9]*$') { return $null }
    return $key
}

function Invoke-MergeVoyageModIni {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$TemplatePath,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [string]$CurrentPath,
        [string[]]$RetiredKeys = @(),
        [string[]]$RetiredComments = @(),
        [string]$DefaultsComment = '# Defaults added by a newer mod build.'
    )
    $template = (Resolve-Path -LiteralPath $TemplatePath).Path
    $output = [IO.Path]::GetFullPath($OutputPath)
    if ($output.Equals($template, [StringComparison]::OrdinalIgnoreCase) -or
        ($CurrentPath -and $output.Equals([IO.Path]::GetFullPath($CurrentPath), [StringComparison]::OrdinalIgnoreCase))) {
        throw 'OutputPath must differ from both inputs.'
    }
    if (Test-Path -LiteralPath $output) { throw "Output already exists: $output" }
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($output))
    if (-not $CurrentPath) {
        [IO.File]::WriteAllBytes($output, [IO.File]::ReadAllBytes($template))
        return [pscustomobject]@{ Created=$true; AddedKeys=@(); RemovedKeys=@(); Changed=$true; Sha256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash }
    }
    $current = (Resolve-Path -LiteralPath $CurrentPath).Path
    $bytes = [IO.File]::ReadAllBytes($current)
    $encoding = [Text.UTF8Encoding]::new($false, $true)
    $preambleLength = 0
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $encoding = [Text.UTF8Encoding]::new($true, $true); $preambleLength = 3
    } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
        $encoding = [Text.UnicodeEncoding]::new($false, $true, $true); $preambleLength = 2
    } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
        $encoding = [Text.UnicodeEncoding]::new($true, $true, $true); $preambleLength = 2
    }
    $originalText = $encoding.GetString($bytes, $preambleLength, $bytes.Length - $preambleLength)
    $newline = if ($originalText.Contains("`r`n")) { "`r`n" } elseif ($originalText.Contains("`n")) { "`n" } else { [Environment]::NewLine }
    $retiredKeySet = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $retiredCommentSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($key in $RetiredKeys) { [void]$retiredKeySet.Add($key) }
    foreach ($comment in $RetiredComments) { [void]$retiredCommentSet.Add($comment) }
    $kept = [Collections.Generic.List[string]]::new()
    $removed = [Collections.Generic.List[string]]::new()
    $changed = $false
    $headingSeen = $false
    foreach ($line in [IO.File]::ReadAllLines($current)) {
        if ($retiredCommentSet.Contains($line.Trim())) { $changed = $true; continue }
        if ($line.Trim() -ceq $DefaultsComment) {
            if ($headingSeen) { $changed = $true; continue }
            $headingSeen = $true
        }
        $key = Get-IniLineKey $line
        if ($null -ne $key -and $retiredKeySet.Contains($key)) {
            if (-not $removed.Contains($key)) { $removed.Add($key) }
            $changed = $true; continue
        }
        $kept.Add($line)
    }
    $existing = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($line in $kept) { $key=Get-IniLineKey $line; if ($null -ne $key) { [void]$existing.Add($key) } }
    $missing = [Collections.Generic.List[string]]::new()
    $added = [Collections.Generic.List[string]]::new()
    foreach ($line in [IO.File]::ReadAllLines($template)) {
        $key = Get-IniLineKey $line
        if ($null -ne $key -and -not $existing.Contains($key)) {
            $missing.Add($line); $added.Add($key); [void]$existing.Add($key)
        }
    }
    if ($missing.Count -gt 0) { $changed = $true }
    if (-not $changed) { [IO.File]::WriteAllBytes($output,[IO.File]::ReadAllBytes($current)) }
    else {
        $text = $kept -join $newline
        if ($kept.Count -gt 0) { $text += $newline }
        if ($missing.Count -gt 0) {
            $text += $newline
            if (-not $headingSeen) { $text += $DefaultsComment + $newline }
            $text += ($missing -join $newline) + $newline
        }
        [IO.File]::WriteAllText($output,$text,$encoding)
    }
    [pscustomobject]@{ Created=$false; AddedKeys=@($added); RemovedKeys=@($removed); Changed=$changed; Sha256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash }
}

Export-ModuleMember -Function Invoke-MergeVoyageModIni

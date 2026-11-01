[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [Parameter(Mandatory)][string]$PreparedProjectRoot,
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if (-not $env:CODEX_UNREAL_QUEUE_LEASE) {
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Schema-1 pin default tests'; return
}
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/BlueprintText')).TrimEnd('\') + '\'
$root = [IO.Path]::GetFullPath($OutputRoot)
if (-not $root.StartsWith($owner, [StringComparison]::OrdinalIgnoreCase)) { throw "OutputRoot must be below $owner" }
if (Test-Path -LiteralPath $root) { throw "OutputRoot must be new: $root" }
[void](New-Item -ItemType Directory -Path $root)
$tool = Join-Path $PSScriptRoot 'Invoke-BlueprintText.ps1'
$project = (Resolve-Path $PreparedProjectRoot).Path
$engineArgs = @{ProjectRoot=$project; EngineRoot=$EngineRoot}
function WriteJson([string]$path, $value) {
    [IO.File]::WriteAllText($path, ($value | ConvertTo-Json -Depth 30), [Text.UTF8Encoding]::new($false))
}
function ReadNative([string]$path) {
    $envelope = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    Get-Content -LiteralPath (Join-Path (Split-Path $path -Parent) $envelope.graphs[0].bodyFile) -Raw
}
function FindPin([string]$text, [string]$name) {
    $pinMatches = @([regex]::Matches($text, ('(?m)^\s*CustomProperties Pin \(.*PinName="' + [regex]::Escape($name) + '",.*$')))
    if ($pinMatches.Count -ne 1) { throw "Expected one pin named $name, found $($pinMatches.Count)" }
    $pinMatches[0].Value
}
$checks = [ordered]@{}
foreach ($case in @(
    @{name='Object'; operation='/Script/Engine.KismetSystemLibrary::GetObjectName'; pin='Object'; field='DefaultObject'; value='"/Script/CoreUObject.Class''/Script/Engine.Actor''"'; marker='/Script/Engine.Actor'},
    @{name='Text'; operation='/Script/Engine.KismetTextLibrary::Conv_TextToString'; pin='InText'; field='DefaultTextValue'; value='NSLOCTEXT("BPTDefaultTest","Greeting","Hello from BPT")'; marker='Hello from BPT'},
    @{name='InvariantText'; operation='/Script/Engine.KismetTextLibrary::Conv_TextToString'; pin='InText'; field='DefaultTextValue'; value='INVTEXT("Invariant BPT value")'; marker='Invariant BPT value'},
    @{name='Target'; operation='/Script/Engine.Actor::K2_DestroyActor'; pin='self'; field='DefaultObject'; value='"/Script/Engine.Actor''/Script/Engine.Default__Actor''"'; marker='/Script/Engine.Default__Actor'}
)) {
    Write-Host ("Schema-1 pin defaults: " + $case.name)
    $asset = '/Game/BP_PinDefault' + $case.name
    $seed = Join-Path $root ($case.name + '-seed.json')
    WriteJson $seed ([ordered]@{schema=1; asset=$asset; parent='/Script/Engine.Actor'; variables=@(); graphs=@(@{
        name='EventGraph'; format='compact'; body=@{nodes=@(@{id='probe'; kind='call'; operation=$case.operation}); links=@()}
    })})
    [void](&$tool -Action Import -InputPath $seed @engineArgs)
    # Empty object/text defaults and the generated static-function self remain supported.
    [void](&$tool -Action Export -Asset $asset -Format compact -OutputPath (Join-Path $root ($case.name + '-empty.json')) @engineArgs)
    $native = Join-Path $root ($case.name + '-native.json')
    [void](&$tool -Action Export -Asset $asset -Format native -OutputPath $native @engineArgs)
    $text = ReadNative $native
    $pin = FindPin $text $case.pin
    if ($pin.Contains($case.field + '=')) { throw "Fixture seed unexpectedly has $($case.field): $pin" }
    $changed = $pin.Replace('CustomProperties Pin (', ('CustomProperties Pin (' + $case.field + '=' + $case.value + ','))
    $envelope = Get-Content -LiteralPath $native -Raw | ConvertFrom-Json
    [IO.File]::WriteAllText((Join-Path $root $envelope.graphs[0].bodyFile), $text.Replace($pin, $changed), [Text.UTF8Encoding]::new($false))
    [void](&$tool -Action Import -InputPath $native @engineArgs)
    $saved = Join-Path $root ($case.name + '-saved.json')
    [void](&$tool -Action Export -Asset $asset -Format native -OutputPath $saved @engineArgs)
    $savedPin = FindPin (ReadNative $saved) $case.pin
    if (-not $savedPin.Contains($case.field + '=') -or -not $savedPin.Contains($case.marker)) { throw "Native mode lost $($case.name) default: $savedPin" }
    $package = Join-Path $project ('Content/BP_PinDefault' + $case.name + '.uasset')
    $before = [IO.File]::ReadAllBytes($package)
    $rejectedOutput = Join-Path $root ($case.name + '-rejected.json')
    $rejected = $false
    try {
        &$tool -Action Export -Asset $asset -Format compact -OutputPath $rejectedOutput @engineArgs | Out-Null
    } catch {
        $expected = "cannot preserve $($case.field) on node 'probe' pin '$($case.pin)'"
        if ($_.Exception.Message.Contains($expected)) { $rejected = $true } else { throw }
    }
    if (-not $rejected -or (Test-Path -LiteralPath $rejectedOutput)) { throw "Lossy compact export succeeded for $($case.name)" }
    if ([Convert]::ToBase64String($before) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($package))) { throw 'Rejected export modified its package' }
    $checks[$case.name] = @{emptyAccepted=$true; nativeRoundTrip=$true; compactRejected=$true; packageUnchanged=$true; noOutput=$true}
}
$result = [ordered]@{status='passed'; projectRoot=$project; checks=$checks}
WriteJson (Join-Path $root 'result.json') $result
[pscustomobject]$result

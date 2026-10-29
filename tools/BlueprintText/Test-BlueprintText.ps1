[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,[string]$PreparedProjectRoot,[string]$EngineRoot='K:\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'; Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$owner=[IO.Path]::GetFullPath((Join-Path $repo 'Tmp/BlueprintText')).TrimEnd('\')+'\'
$root=[IO.Path]::GetFullPath($OutputRoot)
if(-not $root.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw "OutputRoot must be below $owner"}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintText.ps1'
if([string]::IsNullOrWhiteSpace($PreparedProjectRoot)){
    if(Test-Path $root){throw "OutputRoot must be new: $root"}
    $project=(&$tool -Action Prepare -OutputRoot $root -EngineRoot $EngineRoot).projectRoot
} else {$project=(Resolve-Path $PreparedProjectRoot).Path}
function WriteJson([string]$name,$value){$path=Join-Path $root $name;[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 50),[Text.UTF8Encoding]::new($false));$path}
function ReadJson([string]$path){Get-Content -LiteralPath $path -Raw|ConvertFrom-Json}
function Envelope([string]$asset,[string]$variable,[int]$left,[int]$right){
    [ordered]@{schema=1;asset=$asset;parent='/Script/Engine.Actor';variables=@([ordered]@{name=$variable;type='float'});graphs=@([ordered]@{name='EventGraph';format='compact';body=[ordered]@{
        nodes=@([ordered]@{id='begin';kind='event';event='/Script/Engine.Actor::ReceiveBeginPlay'},[ordered]@{id='divide';kind='call';operation=$script:divideDouble.identity;comment='catalog divide node';arguments=[ordered]@{left=[string]$left;right=[string]$right}},[ordered]@{id='sequence';kind='sequence';outputs=2},[ordered]@{id='set.result';kind='setVariable';variable=$variable})
        links=@([ordered]@{from='begin.then';to='sequence.exec'},[ordered]@{from='sequence.then0';to='set.result.exec'},[ordered]@{from='divide.result';to='set.result.value'})
    }})}
}
function Semantics([string]$path){$e=ReadJson $path;foreach($node in $e.graphs[0].body.nodes){$arguments=$node.PSObject.Properties['arguments'];if($null-ne$arguments){foreach($property in $arguments.Value.PSObject.Properties){$number=0.0;if([double]::TryParse([string]$property.Value,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number)){$property.Value=$number.ToString('R',[Globalization.CultureInfo]::InvariantCulture)}}}};$nodes=@($e.graphs[0].body.nodes|Sort-Object id|%{$_|ConvertTo-Json -Depth 20 -Compress});$links=@($e.graphs[0].body.links|%{"$($_.from)->$($_.to)"}|Sort-Object);$vars=@($e.variables|Sort-Object name|%{$_|ConvertTo-Json -Depth 10 -Compress});(@("parent=$($e.parent)","graph=$($e.graphs[0].name)","format=$($e.graphs[0].format)")+$vars+$nodes+$links)-join"`n"}
function Same([string]$a,[string]$b,[string]$message){if((Semantics $a)-cne(Semantics $b)){throw $message}}

function NativeBlocks([string]$text){
    $result=[Collections.Generic.List[string]]::new();$capture=[Collections.Generic.List[string]]::new();$depth=0
    foreach($line in ($text-split"`r?`n")){
        if($depth-eq0-and$line-match'^Begin Object '){$capture=[Collections.Generic.List[string]]::new()}
        if($depth-gt0-or$line-match'^Begin Object '){$capture.Add($line)}
        if($line-match'^\s*Begin Object '){$depth++}
        if($line-match'^\s*End Object\s*$'){$depth--;if($depth-eq0-and$capture.Count){$result.Add(($capture-join"`r`n"))}}
    }; @($result)
}
function NativeBlock([string]$text,[string]$className){$found=@(NativeBlocks $text|?{$_-match('Class=/Script/BlueprintGraph\.'+[regex]::Escape($className)+'\b')});if($found.Count-ne1){throw "Expected one $className block, found $($found.Count)"};$found[0]}
function NativeId([string]$block,[string]$id,[string]$comment=''){$value='BTP:id='+$id+$(if($comment){'\n'+$comment}else{''});if($block-match'(?m)^\s+NodeComment='){$block=[regex]::Replace($block,'(?m)^(\s+)NodeComment=.*$',('$1NodeComment="'+$value+'"'))}else{$block=[regex]::Replace($block,'(?m)^(\s+NodeGuid=.*)$',('$1'+"`r`n"+'   NodeComment="'+$value+'"'))};$block}
function NodeName([string]$block){if($block-notmatch'(?m)^Begin Object .* Name="([^"]+)"'){throw 'Node name missing'};$Matches[1]}
function PinBlocks([string]$block){@([regex]::Matches($block,'(?m)^\s+CustomProperties Pin \(.*\)\s*$')|%{$_.Value})}
function Pin([string]$block,[string]$name,[string]$direction){$found=@(PinBlocks $block|?{$_-match('PinName="'+[regex]::Escape($name)+'"')-and(($direction-eq'output')-eq($_-match'Direction="EGPD_Output"'))});if($found.Count-ne1){throw "Expected one $direction pin $name, found $($found.Count)"};$found[0]}
function PinId([string]$pin){if($pin-notmatch'PinId=([A-F0-9]+)'){throw 'PinId missing'};$Matches[1]}
function PinDefault([string]$block,[string]$name,[string]$value){$pin=Pin $block $name input;$matcher=[regex]'DefaultValue="[^"]*"';$changed=$matcher.Replace($pin,('DefaultValue="'+$value+'"'),1);if($changed-ceq$pin){throw "Default missing on $name"};$block.Replace($pin,$changed)}
function PinLink([string]$block,[string]$name,[string]$direction,[string]$otherNode,[string]$otherPin){$pin=Pin $block $name $direction;if($pin-match'LinkedTo='){throw "$name already linked"};$linked=$pin.Replace('PersistentGuid=',('LinkedTo=('+$otherNode+' '+$otherPin+',),PersistentGuid='));$block.Replace($pin,$linked)}
function Connect([ref]$from,[string]$fromPin,[ref]$to,[string]$toPin){$a=NodeName $from.Value;$b=NodeName $to.Value;$ap=PinId (Pin $from.Value $fromPin output);$bp=PinId (Pin $to.Value $toPin input);$from.Value=PinLink $from.Value $fromPin output $b $bp;$to.Value=PinLink $to.Value $toPin input $a $ap}

$catalogRefresh=&$tool -Action RefreshCatalog -ProjectRoot $project -EngineRoot $EngineRoot -Force
$divideSearch=&$tool -Action SearchCatalog -Query divide -Limit 50 -Format native
$sequenceSearch=&$tool -Action SearchCatalog -Query sequence -Format native
$script:divideDouble=@($divideSearch.entries|? identity -like '*::Divide_DoubleDouble')[0]
$divideInt=@($divideSearch.entries|? identity -like '*::Divide_IntInt');$sequence=@($sequenceSearch.entries|? identity -eq 'flow.sequence')
if($null-eq$script:divideDouble-or$divideInt.Count-ne1-or$sequence.Count-ne1){throw 'Catalog identities missing'}

$baseline=[ordered]@{schema=1;asset='/Game/BP_TextMathCompact';parent='/Script/Engine.Actor';variables=@([ordered]@{name='Result';type='float'});graphs=@([ordered]@{name='EventGraph';format='compact';body=[ordered]@{nodes=@([ordered]@{id='begin';kind='event';event='/Script/Engine.Actor::ReceiveBeginPlay'});links=@()}})}
$baselinePath=WriteJson 'baseline-compact.json' $baseline;$desiredPath=WriteJson 'edited-compact.json' (Envelope '/Game/BP_TextMathCompact' 'Result' 12000 2)
[void](&$tool -Action Import -ProjectRoot $project -InputPath $baselinePath -EngineRoot $EngineRoot)
$baselineNative=Join-Path $root 'baseline-native.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathCompact -Format native -OutputPath $baselineNative -EngineRoot $EngineRoot)
[void](&$tool -Action Import -ProjectRoot $project -InputPath $desiredPath -EngineRoot $EngineRoot)
$compactExport=Join-Path $root 'compact-export.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathCompact -Format compact -OutputPath $compactExport -EngineRoot $EngineRoot);Same $desiredPath $compactExport 'Compact export differs from source'
$nativeGenerated=Join-Path $root 'native-generated.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathCompact -Format native -OutputPath $nativeGenerated -EngineRoot $EngineRoot)
[void](&$tool -Action Import -ProjectRoot $project -InputPath $compactExport -EngineRoot $EngineRoot)
$compactSecond=Join-Path $root 'compact-second.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathCompact -Format compact -OutputPath $compactSecond -EngineRoot $EngineRoot);Same $compactExport $compactSecond 'Compact second import differs'

$baseEnvelope=ReadJson $baselineNative;$baseText=Get-Content (Join-Path $root $baseEnvelope.graphs[0].bodyFile) -Raw
$generatedEnvelope=ReadJson $nativeGenerated;$generatedText=Get-Content (Join-Path $root $generatedEnvelope.graphs[0].bodyFile) -Raw
$event=NativeBlock $baseText K2Node_Event;$divide=NativeBlock ([string]$script:divideDouble.nativeInsertion) K2Node_CallFunction;$flow=NativeBlock ([string]$sequence[0].nativeInsertion) K2Node_ExecutionSequence;$set=NativeBlock $generatedText K2Node_VariableSet
$set=[regex]::Replace($set,'LinkedTo=\([^)]*\),','');$set=[regex]::Replace($set,'MemberGuid=[A-F0-9]+','MemberGuid=00000000000000000000000000000000')
$event=NativeId $event begin;$divide=NativeId $divide divide 'catalog divide node';$flow=NativeId $flow sequence;$set=NativeId $set set.result
$left=[string](@($script:divideDouble.pins|? alias -eq left)[0].name);$right=[string](@($script:divideDouble.pins|? alias -eq right)[0].name);$resultPin=[string](@($script:divideDouble.pins|? alias -eq result)[0].name)
$divide=PinDefault $divide $left '12000.0';$divide=PinDefault $divide $right '2.0'
Connect ([ref]$event) then ([ref]$flow) execute;Connect ([ref]$flow) then_0 ([ref]$set) execute;Connect ([ref]$divide) $resultPin ([ref]$set) Result
$nativeBody=Join-Path $root 'edited-native.EventGraph.t3d';[IO.File]::WriteAllText($nativeBody,(@($event,$divide,$flow,$set)-join"`r`n"),[Text.UTF8Encoding]::new($false))
$nativePath=WriteJson 'edited-native.json' ([ordered]@{schema=1;asset='/Game/BP_TextMathNative';parent='/Script/Engine.Actor';variables=@([ordered]@{name='Result';type='float'});graphs=@([ordered]@{name='EventGraph';format='native';bodyFile='edited-native.EventGraph.t3d'})})
[void](&$tool -Action Import -ProjectRoot $project -InputPath $nativePath -EngineRoot $EngineRoot)
$nativeOne=Join-Path $root 'native-compact-one.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathNative -Format compact -OutputPath $nativeOne -EngineRoot $EngineRoot);Same $desiredPath $nativeOne 'Native direct insertion differs from source'
$nativeReexport=Join-Path $root 'native-reexport.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathNative -Format native -OutputPath $nativeReexport -EngineRoot $EngineRoot)
[void](&$tool -Action Import -ProjectRoot $project -InputPath $nativeReexport -EngineRoot $EngineRoot)
$nativeTwo=Join-Path $root 'native-compact-two.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathNative -Format compact -OutputPath $nativeTwo -EngineRoot $EngineRoot);Same $nativeOne $nativeTwo 'Native export second import differs'

$delete=ReadJson $desiredPath;$delete.graphs[0].body.nodes=@($delete.graphs[0].body.nodes|? id -ne sequence);$delete.graphs[0].body.links=@($delete.graphs[0].body.links|?{$_.from-notlike'sequence.*'-and$_.to-notlike'sequence.*'})
$deletePath=WriteJson 'delete.json' $delete;[void](&$tool -Action Import -ProjectRoot $project -InputPath $deletePath -EngineRoot $EngineRoot)
$deleteExport=Join-Path $root 'delete-export.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathCompact -Format compact -OutputPath $deleteExport -EngineRoot $EngineRoot);if(@((ReadJson $deleteExport).graphs[0].body.nodes|? id -eq sequence).Count){throw 'Omitted node remained'}

[void](&$tool -Action SimulateEdit -ProjectRoot $project -Asset /Game/BP_TextMathNative -Variable EngineAdded -EngineRoot $EngineRoot)
$editOne=Join-Path $root 'engine-edit.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathNative -Format compact -OutputPath $editOne -EngineRoot $EngineRoot);[void](&$tool -Action Import -ProjectRoot $project -InputPath $editOne -EngineRoot $EngineRoot)
$editTwo=Join-Path $root 'engine-edit-reopen.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_TextMathNative -Format compact -OutputPath $editTwo -EngineRoot $EngineRoot);$added=@((ReadJson $editTwo).variables|? name -eq EngineAdded);if($added.Count-ne1){throw 'Engine-added variable lost'}

$otherPath=WriteJson 'other.json' (Envelope /Game/BP_AlternateMath AlternateTotal 44 4);[void](&$tool -Action Import -ProjectRoot $project -InputPath $otherPath -EngineRoot $EngineRoot)
$otherOne=Join-Path $root 'other-export.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_AlternateMath -Format compact -OutputPath $otherOne -EngineRoot $EngineRoot);Same $otherPath $otherOne 'Other asset differs from source';[void](&$tool -Action Import -ProjectRoot $project -InputPath $otherOne -EngineRoot $EngineRoot)
$otherTwo=Join-Path $root 'other-reopen.json';[void](&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_AlternateMath -Format compact -OutputPath $otherTwo -EngineRoot $EngineRoot);Same $otherOne $otherTwo 'Other asset reopen differs'

$unknown=ReadJson $desiredPath;$unknown|Add-Member -NotePropertyName timelines -NotePropertyValue @();$unknownPath=WriteJson 'bad-unknown.json' $unknown;$unknownRejected=$false
try{&$tool -Action Import -ProjectRoot $project -InputPath $unknownPath -EngineRoot $EngineRoot|Out-Null}catch{if($_.Exception.Message-like"*Unsupported envelope field 'timelines'*"){$unknownRejected=$true}else{throw}}
if(-not$unknownRejected){throw 'Unknown field accepted'}
$bad=ReadJson $desiredPath;(@($bad.graphs[0].body.nodes|? id -eq sequence)[0]).outputs=1;$badPath=WriteJson 'bad-sequence.json' $bad;$rangeRejected=$false
try{&$tool -Action Import -ProjectRoot $project -InputPath $badPath -EngineRoot $EngineRoot|Out-Null}catch{if($_.Exception.Message-like'*Sequence outputs must be an integer from 2 through 16*'){$rangeRejected=$true}else{throw}}
if(-not$rangeRejected){throw 'Bad sequence accepted'}
$badDefault=ReadJson $desiredPath;(@($badDefault.variables)[0])|Add-Member -NotePropertyName default -NotePropertyValue '12';$badDefaultPath=WriteJson 'bad-variable-default.json' $badDefault;$defaultRejected=$false
try{&$tool -Action Import -ProjectRoot $project -InputPath $badDefaultPath -EngineRoot $EngineRoot|Out-Null}catch{if($_.Exception.Message.Contains("Unsupported variables[0] field 'default'")){$defaultRejected=$true}else{throw}}
if(-not$defaultRejected){throw 'Variable default silently accepted'}
[void](&$tool -Action SimulateUnsupportedGraph -ProjectRoot $project -Asset /Game/BP_AlternateMath -EngineRoot $EngineRoot);$savedRejected=$false
try{&$tool -Action Export -ProjectRoot $project -Asset /Game/BP_AlternateMath -Format compact -OutputPath (Join-Path $root 'bad-export.json') -EngineRoot $EngineRoot|Out-Null}catch{if($_.Exception.Message-like'*outside the schema-1 subset*'){$savedRejected=$true}else{throw}}
if(-not$savedRejected){throw 'Unsupported saved graph exported'}

# Check saved engine state, not merely unsupported fields in an input document.
$savedStateChecks = [ordered]@{}
foreach ($fixture in @(
    @{name='Default'; action='SimulateNonzeroDefault'; diagnostic='*nonzero saved CDO default*'},
    @{name='Construction'; action='SimulateConstructionLogic'; diagnostic='*Construction-script logic is outside*'}
)) {
    $fixtureAsset = '/Game/BP_Saved' + $fixture.name
    $fixturePath = WriteJson ('saved-' + $fixture.name + '.json') (Envelope $fixtureAsset 'Result' 12 2)
    [void](&$tool -Action Import -ProjectRoot $project -InputPath $fixturePath -EngineRoot $EngineRoot)
    [void](&$tool -Action $fixture.action -ProjectRoot $project -Asset $fixtureAsset -Variable Result -EngineRoot $EngineRoot)
    foreach ($operation in @('Export', 'Import')) {
        $rejected = $false
        try {
            if ($operation -eq 'Export') {
                &$tool -Action Export -ProjectRoot $project -Asset $fixtureAsset -Format compact -OutputPath (Join-Path $root ('rejected-' + $fixture.name + '.json')) -EngineRoot $EngineRoot | Out-Null
            } else {
                &$tool -Action Import -ProjectRoot $project -InputPath $fixturePath -EngineRoot $EngineRoot | Out-Null
            }
        } catch {
            if ($_.Exception.Message -like $fixture.diagnostic) { $rejected = $true } else { throw }
        }
        if (-not $rejected) { throw "Saved $($fixture.name) state was accepted for $operation" }
        $savedStateChecks[$fixture.name + $operation + 'Rejected'] = $true
    }
}

$result=[ordered]@{status='passed';projectRoot=$project;catalog=[ordered]@{path=$catalogRefresh.catalog;coverage=$divideSearch.coverage;refresh=$divideSearch.refresh;doubleDivide=$script:divideDouble.identity;intDivide=$divideInt[0].identity;sequence=$sequence[0].identity};sources=[ordered]@{compact=$desiredPath;nativeEnvelope=$nativePath;nativeBody=$nativeBody;other=$otherPath};outputs=[ordered]@{compactFirst=$compactExport;compactSecond=$compactSecond;nativeFirst=$nativeOne;nativeReexport=$nativeReexport;nativeSecond=$nativeTwo;engineEdit=$editOne;engineEditReopen=$editTwo;otherFirst=$otherOne;otherReopen=$otherTwo};checks=[ordered]@{compactSourceMatchesExport=$true;compactExportSecondImport=$true;nativeDirectTextInsertionMatchesSource=$true;nativeExportSecondImport=$true;omissionDeletes=$true;commentPreserved=$true;engineAddedVariablePreserved=$true;variableDefaultExplicitlyRejected=$defaultRejected;secondAssetExportReopen=$true;unknownFieldRejected=$unknownRejected;sequenceRangeRejected=$rangeRejected;unsupportedSavedGraphRejected=$savedRejected}}
foreach ($key in $savedStateChecks.Keys) { $result.checks[$key] = $savedStateChecks[$key] }
[IO.File]::WriteAllText((Join-Path $root 'result.json'),($result|ConvertTo-Json -Depth 20),[Text.UTF8Encoding]::new($false));[pscustomobject]$result

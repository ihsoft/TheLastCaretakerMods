[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Query,

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$MappingsPath,

    [string]$MappingsManifestPath,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [ValidateSet('Game', 'Mod')]
    [string]$Source = 'Game',

    [string]$ModContainer,

    [ValidateSet('Overview', 'Functions', 'Calls', 'Members', 'SoftObjects', 'Components', 'All')]
    [string]$Focus = 'Overview',

    [string]$FunctionName,

    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$summarySchemaVersion = 4
$getAssetScript = Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1'
$summaryRoot = Join-Path $PSScriptRoot '..\artifacts\asset-summaries'
$keySeparator = [char]31

function Get-OptionalPropertyValue {
    param(
        [Parameter(Mandatory = $true)]
        [AllowNull()]
        [object]$Object,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    $property.Value
}

function Convert-ObjectReference {
    param([object]$Reference)

    if ($null -eq $Reference) {
        return $null
    }
    if ($Reference -is [string]) {
        return [pscustomobject]@{
            objectName = [string]$Reference
            objectPath = $null
        }
    }

    $objectName = Get-OptionalPropertyValue -Object $Reference -Name 'ObjectName'
    $objectPath = Get-OptionalPropertyValue -Object $Reference -Name 'ObjectPath'
    if ($null -eq $objectName -and $null -eq $objectPath) {
        return $null
    }
    [pscustomobject]@{
        objectName = if ($null -eq $objectName) { $null } else { [string]$objectName }
        objectPath = if ($null -eq $objectPath) { $null } else { [string]$objectPath }
    }
}

function Get-ReferenceMemberName {
    param(
        [object]$Reference,
        [string]$Fallback
    )

    if ($Reference -is [string]) {
        return [string]$Reference
    }
    if ($null -ne $Reference) {
        $objectName = [string](Get-OptionalPropertyValue -Object $Reference -Name 'ObjectName')
        if ($objectName -match ":([^:']+)'?$") {
            return $Matches[1]
        }
        if ($objectName -match "^[^']+'([^']+)'$") {
            return $Matches[1]
        }
        if (-not [string]::IsNullOrWhiteSpace($objectName)) {
            return $objectName
        }
    }
    $Fallback
}

function Convert-ReferenceWithName {
    param(
        [object]$Reference,
        [string]$Fallback
    )

    if ($null -eq $Reference -and [string]::IsNullOrWhiteSpace($Fallback)) {
        return $null
    }
    $converted = Convert-ObjectReference -Reference $Reference
    [pscustomobject]@{
        name = Get-ReferenceMemberName -Reference $Reference -Fallback $Fallback
        objectName = if ($null -eq $converted) { $null } else { $converted.objectName }
        objectPath = if ($null -eq $converted) { $null } else { $converted.objectPath }
    }
}

function Add-ChildrenToStack {
    param(
        [Parameter(Mandatory = $true)]
        [Collections.Stack]$Stack,

        [object]$Node
    )

    if ($null -eq $Node -or $Node -is [string] -or $Node.GetType().IsValueType) {
        return
    }
    if ($Node -is [Collections.IDictionary]) {
        foreach ($value in $Node.Values) {
            if ($null -ne $value) { $Stack.Push($value) }
        }
        return
    }
    if ($Node -is [Collections.IEnumerable]) {
        foreach ($value in $Node) {
            if ($null -ne $value) { $Stack.Push($value) }
        }
        return
    }
    foreach ($property in $Node.PSObject.Properties) {
        if ($null -ne $property.Value) { $Stack.Push($property.Value) }
    }
}

function Merge-CallRecords {
    param([object[]]$Records)

    @($Records | Group-Object Key | ForEach-Object {
        $first = $_.Group[0]
        [pscustomobject]@{
            name = $first.name
            objectName = $first.objectName
            objectPath = $first.objectPath
            dispatch = @($_.Group.dispatch | Sort-Object -Unique)
            referencedByFunctions = @($_.Group.sourceFunction | Sort-Object -Unique)
        }
    } | Sort-Object objectPath, objectName, name)
}

function Merge-MemberRecords {
    param([object[]]$Records)

    @($Records | Group-Object Key | ForEach-Object {
        $first = $_.Group[0]
        [pscustomobject]@{
            ownerObjectName = $first.ownerObjectName
            ownerObjectPath = $first.ownerObjectPath
            memberName = $first.memberName
            memberType = $first.memberType
            propertyFlags = $first.propertyFlags
            referencedByFunctions = @($_.Group.sourceFunction | Sort-Object -Unique)
        }
    } | Sort-Object ownerObjectPath, memberName)
}

function Merge-SoftObjectRecords {
    param([object[]]$Records)

    @($Records | Group-Object Key | ForEach-Object {
        $first = $_.Group[0]
        [pscustomobject]@{
            token = $first.token
            objectName = $first.objectName
            objectPath = $first.objectPath
            referencedByFunctions = @($_.Group.sourceFunction | Sort-Object -Unique)
        }
    } | Sort-Object objectPath, objectName)
}

$assetArguments = @{
    Query = $Query
    GameRoot = $GameRoot
    EngineVersion = $EngineVersion
    Source = $Source
    AsJson = $true
}
if (-not [string]::IsNullOrWhiteSpace($MappingsPath)) {
    $assetArguments.MappingsPath = $MappingsPath
}
if (-not [string]::IsNullOrWhiteSpace($MappingsManifestPath)) {
    $assetArguments.MappingsManifestPath = $MappingsManifestPath
}
if (-not [string]::IsNullOrWhiteSpace($ModContainer)) {
    $assetArguments.ModContainer = $ModContainer
}

$assetResultText = (& $getAssetScript @assetArguments) -join [Environment]::NewLine
$assetResult = $assetResultText | ConvertFrom-Json
$jsonPath = (Resolve-Path -LiteralPath ([string]$assetResult.jsonPath)).Path
$jsonSha256 = (Get-FileHash -LiteralPath $jsonPath -Algorithm SHA256).Hash
if ($jsonSha256 -cne [string]$assetResult.jsonSha256) {
    throw "Asset JSON changed after retrieval: $jsonPath"
}

$summaryDirectory = if ($Source -eq 'Game') {
    Join-Path (Join-Path ([IO.Path]::GetFullPath($summaryRoot)) 'game') $jsonSha256
}
else {
    $runName = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ') + '-' +
        [guid]::NewGuid().ToString('N').Substring(0, 8)
    Join-Path (Join-Path ([IO.Path]::GetFullPath($summaryRoot)) 'mod') $runName
}
$summaryPath = Join-Path $summaryDirectory 'summary.json'

$summary = $null
if ($Source -eq 'Game' -and (Test-Path -LiteralPath $summaryPath -PathType Leaf)) {
    try {
        $candidate = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
        if ([int]$candidate.schemaVersion -eq $summarySchemaVersion -and
            [string]$candidate.source -ceq 'Game' -and
            [string]$candidate.virtualPath -ceq [string]$assetResult.virtualPath -and
            [string]$candidate.assetJson.sha256 -ceq $jsonSha256) {
            $summary = $candidate
        }
    }
    catch {
        $summary = $null
    }
}

if ($null -eq $summary) {
    [object[]]$exports = Get-Content -LiteralPath $jsonPath -Raw | ConvertFrom-Json
    if ($exports.Count -eq 0) {
        throw "Asset JSON contains no exports: $jsonPath"
    }

    $generatedClasses = @($exports | Where-Object { [string]$_.Type -ceq 'BlueprintGeneratedClass' } | ForEach-Object {
        $super = Get-OptionalPropertyValue -Object $_ -Name 'Super'
        if ($null -eq $super) {
            $super = Get-OptionalPropertyValue -Object $_ -Name 'SuperStruct'
        }
        [pscustomobject]@{
            name = [string]$_.Name
            package = [string](Get-OptionalPropertyValue -Object $_ -Name 'Package')
            super = Convert-ReferenceWithName -Reference $super -Fallback $null
            classFlags = [string](Get-OptionalPropertyValue -Object $_ -Name 'ClassFlags')
        }
    })
    $assetPackage = if ($generatedClasses.Count -gt 0) {
        [string]$generatedClasses[0].package
    }
    else {
        [IO.Path]::ChangeExtension(([string]$assetResult.virtualPath).Replace('Voyage/Content/', '/Game/'), $null)
    }

    $functions = [Collections.Generic.List[object]]::new()
    $callRecords = [Collections.Generic.List[object]]::new()
    $memberRecords = [Collections.Generic.List[object]]::new()
    $softObjectRecords = [Collections.Generic.List[object]]::new()
    foreach ($functionExport in @($exports | Where-Object { [string]$_.Type -ceq 'Function' })) {
        $exportFunctionName = [string]$functionExport.Name
        $super = Get-OptionalPropertyValue -Object $functionExport -Name 'Super'
        if ($null -eq $super) {
            $super = Get-OptionalPropertyValue -Object $functionExport -Name 'SuperStruct'
        }
        $childPropertyValue = Get-OptionalPropertyValue -Object $functionExport -Name 'ChildProperties'
        [object[]]$childProperties = if ($null -eq $childPropertyValue) {
            @()
        }
        else {
            $childPropertyValue
        }
        $parameters = @($childProperties | Where-Object {
            [string](Get-OptionalPropertyValue -Object $_ -Name 'PropertyFlags') -match '(^|\s\|\s)Parm($|\s\|\s)'
        })
        $functions.Add([pscustomobject]@{
            name = $exportFunctionName
            owner = Convert-ReferenceWithName -Reference (Get-OptionalPropertyValue -Object $functionExport -Name 'Outer') -Fallback $null
            super = Convert-ReferenceWithName -Reference $super -Fallback $null
            functionFlags = [string](Get-OptionalPropertyValue -Object $functionExport -Name 'FunctionFlags')
            parameterCount = $parameters.Count
            parameters = @($parameters | ForEach-Object {
                [pscustomobject]@{
                    name = [string]$_.Name
                    type = [string]$_.Type
                    flags = [string](Get-OptionalPropertyValue -Object $_ -Name 'PropertyFlags')
                }
            })
            scriptStatementCount = @(Get-OptionalPropertyValue -Object $functionExport -Name 'ScriptBytecode').Count
        })

        $stack = [Collections.Stack]::new()
        $scriptBytecode = Get-OptionalPropertyValue -Object $functionExport -Name 'ScriptBytecode'
        if ($null -ne $scriptBytecode) { $stack.Push($scriptBytecode) }
        while ($stack.Count -gt 0) {
            $node = $stack.Pop()
            if ($null -eq $node -or $node -is [string] -or $node.GetType().IsValueType) {
                continue
            }

            $token = [string](Get-OptionalPropertyValue -Object $node -Name 'Token')
            $functionReference = Get-OptionalPropertyValue -Object $node -Name 'Function'
            if ($token -in @('EX_FinalFunction', 'EX_VirtualFunction', 'EX_LocalFinalFunction',
                    'EX_LocalVirtualFunction', 'EX_CallMath') -and $null -ne $functionReference) {
                $call = Convert-ReferenceWithName -Reference $functionReference -Fallback '<unknown>'
                $callRecords.Add([pscustomobject]@{
                    Key = ('{0}{1}{2}{1}{3}' -f $call.objectPath, $keySeparator,
                        $call.objectName, $call.name)
                    name = $call.name
                    objectName = $call.objectName
                    objectPath = $call.objectPath
                    dispatch = $token
                    sourceFunction = $exportFunctionName
                })
            }

            $owner = Get-OptionalPropertyValue -Object $node -Name 'Owner'
            $propertyReference = Get-OptionalPropertyValue -Object $node -Name 'Property'
            if ($null -ne $owner -and $null -ne $propertyReference) {
                $convertedOwner = Convert-ObjectReference -Reference $owner
                $ownerPath = if ($null -eq $convertedOwner) { $null } else { [string]$convertedOwner.objectPath }
                $isExternalOwner = -not [string]::IsNullOrWhiteSpace($ownerPath) -and
                    -not $ownerPath.StartsWith($assetPackage + '.', [StringComparison]::Ordinal)
                if ($isExternalOwner) {
                    $memberName = [string](Get-OptionalPropertyValue -Object $propertyReference -Name 'Name')
                    $memberType = [string](Get-OptionalPropertyValue -Object $propertyReference -Name 'Type')
                    $propertyFlags = [string](Get-OptionalPropertyValue -Object $propertyReference -Name 'PropertyFlags')
                    $memberRecords.Add([pscustomobject]@{
                        Key = ('{0}{1}{2}{1}{3}{1}{4}' -f $ownerPath, $keySeparator,
                            $memberName, $memberType, $propertyFlags)
                        ownerObjectName = $convertedOwner.objectName
                        ownerObjectPath = $ownerPath
                        memberName = $memberName
                        memberType = $memberType
                        propertyFlags = if ([string]::IsNullOrWhiteSpace($propertyFlags)) { 'unknown' } else { $propertyFlags }
                        sourceFunction = $exportFunctionName
                    })
                }
            }

            if ($token -in @('EX_SoftObjectConst', 'EX_SoftClassConst')) {
                $value = Convert-ObjectReference -Reference (Get-OptionalPropertyValue -Object $node -Name 'Value')
                if ($null -ne $value) {
                    $softObjectRecords.Add([pscustomobject]@{
                        Key = ('{0}{1}{2}{1}{3}' -f $token, $keySeparator,
                            $value.objectPath, $value.objectName)
                        token = $token
                        objectName = $value.objectName
                        objectPath = $value.objectPath
                        sourceFunction = $exportFunctionName
                    })
                }
            }

            Add-ChildrenToStack -Stack $stack -Node $node
        }
    }

    $components = @($exports | Where-Object { [string]$_.Type -ceq 'SCS_Node' } | ForEach-Object {
        $properties = Get-OptionalPropertyValue -Object $_ -Name 'Properties'
        $componentClass = if ($null -eq $properties) { $null } else {
            Convert-ObjectReference -Reference (Get-OptionalPropertyValue -Object $properties -Name 'ComponentClass')
        }
        $componentTemplate = if ($null -eq $properties) { $null } else {
            Convert-ObjectReference -Reference (Get-OptionalPropertyValue -Object $properties -Name 'ComponentTemplate')
        }
        [pscustomobject]@{
            node = [string]$_.Name
            variableName = if ($null -eq $properties) { $null } else {
                [string](Get-OptionalPropertyValue -Object $properties -Name 'InternalVariableName')
            }
            componentClass = $componentClass
            componentTemplate = $componentTemplate
        }
    } | Sort-Object variableName, node)

    [object[]]$calledFunctions = @(Merge-CallRecords -Records @($callRecords))
    [object[]]$externalMembers = @(Merge-MemberRecords -Records @($memberRecords))
    [object[]]$softObjects = @(Merge-SoftObjectRecords -Records @($softObjectRecords))
    $summary = [pscustomobject][ordered]@{
        schemaVersion = $summarySchemaVersion
        kind = 'Voyage asset structure summary'
        source = $Source
        virtualPath = [string]$assetResult.virtualPath
        assetJson = [pscustomobject]@{
            path = $jsonPath
            sha256 = $jsonSha256
        }
        generatedClasses = $generatedClasses
        functions = @($functions | Sort-Object name)
        calledFunctions = $calledFunctions
        externalMemberReferences = $externalMembers
        softObjectConstants = $softObjects
        scsComponents = $components
        limitations = @(
            'This report summarizes serialized cooked-asset data; it is not a native runtime call graph or lifecycle model.',
            'Missing access flags are reported as unknown and must not be interpreted as writable.',
            'Calls are bytecode references grouped by serialized target; dispatch and runtime provider resolution are not inferred.'
        )
    }

    [IO.Directory]::CreateDirectory($summaryDirectory) | Out-Null
    $temporaryPath = $summaryPath + '.new-' + [guid]::NewGuid().ToString('N')
    [IO.File]::WriteAllText(
        $temporaryPath,
        (($summary | ConvertTo-Json -Depth 12) + [Environment]::NewLine))
    Move-Item -LiteralPath $temporaryPath -Destination $summaryPath -Force
}

$resolvedSummaryPath = (Resolve-Path -LiteralPath $summaryPath).Path
$counts = [pscustomobject]@{
    generatedClasses = @($summary.generatedClasses).Count
    functions = @($summary.functions).Count
    calledFunctions = @($summary.calledFunctions).Count
    externalMemberReferences = @($summary.externalMemberReferences).Count
    softObjectConstants = @($summary.softObjectConstants).Count
    scsComponents = @($summary.scsComponents).Count
}
$result = [ordered]@{
    status = 'summarized'
    source = $Source
    virtualPath = [string]$summary.virtualPath
    focus = $Focus
    summaryPath = $resolvedSummaryPath
    summarySha256 = (Get-FileHash -LiteralPath $resolvedSummaryPath -Algorithm SHA256).Hash
    counts = $counts
}
$focusData = $null
if (-not [string]::IsNullOrWhiteSpace($FunctionName)) {
    if ($Focus -notin @('Functions', 'Calls', 'Members', 'SoftObjects')) {
        throw '-FunctionName is valid only with -Focus Functions, Calls, Members, or SoftObjects.'
    }
    $knownFunction = @($summary.functions | Where-Object { [string]$_.name -ceq $FunctionName })
    if ($knownFunction.Count -eq 0) {
        throw "The asset has no serialized function named '$FunctionName'. Summary: $resolvedSummaryPath"
    }
}
switch ($Focus) {
    'Overview' { $focusData = @($summary.generatedClasses) }
    'Functions' {
        $selectedFunctions = @($summary.functions | Where-Object {
            [string]::IsNullOrWhiteSpace($FunctionName) -or [string]$_.name -ceq $FunctionName
        })
        $focusData = if ([string]::IsNullOrWhiteSpace($FunctionName)) {
            @($selectedFunctions | ForEach-Object {
                [pscustomobject]@{ name = [string]$_.name }
            })
        }
        else {
            $selectedFunctions
        }
    }
    'Calls' {
        $focusData = @($summary.calledFunctions | Where-Object {
            [string]::IsNullOrWhiteSpace($FunctionName) -or
                @($_.referencedByFunctions) -ccontains $FunctionName
        })
    }
    'Members' {
        $focusData = @($summary.externalMemberReferences | Where-Object {
            [string]::IsNullOrWhiteSpace($FunctionName) -or
                @($_.referencedByFunctions) -ccontains $FunctionName
        })
    }
    'SoftObjects' {
        $focusData = @($summary.softObjectConstants | Where-Object {
            [string]::IsNullOrWhiteSpace($FunctionName) -or
                @($_.referencedByFunctions) -ccontains $FunctionName
        })
    }
    'Components' { $focusData = @($summary.scsComponents) }
    'All' { $focusData = $summary }
}
$result.functionName = if ([string]::IsNullOrWhiteSpace($FunctionName)) { $null } else { $FunctionName }
$result.resultCount = if ($Focus -eq 'All') { $null } else { @($focusData).Count }
$result.data = $focusData

$resultObject = [pscustomobject]$result
if ($AsJson) {
    Write-Output ($resultObject | ConvertTo-Json -Depth 14 -Compress)
}
else {
    Write-Output $resultObject
}

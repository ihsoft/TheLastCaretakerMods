$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$root = Join-Path $repo ('Tmp/VortexArchiveTests/' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $root -Force
try {
    foreach ($extension in @('pak', 'ucas', 'utoc')) {
        [IO.File]::WriteAllText((Join-Path $root "Example_P.$extension"), "synthetic-$extension")
    }
    $ini = Join-Path $root 'Example.ini'
    [IO.File]::WriteAllText($ini, 'synthetic-ini')
    $plugin = Join-Path $root 'ExampleCatalogue.uplugin'
    [IO.File]::WriteAllText($plugin, '{}')
    $utoc = Join-Path $root 'Example_P.utoc'
    $producer = Join-Path $PSScriptRoot 'New-VoyageVortexArchive.ps1'
    $cases = @(
        @{ name = 'pak'; arguments = @{}; entries = @('Example_P.pak','Example_P.ucas','Example_P.utoc'); modType = $null },
        @{ name = 'pakalt'; arguments = @{ AdditionalFile = @($ini) }; entries = @('Example_P.pak','Example_P.ucas','Example_P.utoc','Example.ini','vortex_override_instructions.json'); modType = 'thelastcaretaker-pakalt' },
        @{ name = 'root'; arguments = @{ ContentPluginDescriptor = $plugin; AdditionalFile = @($ini) }; entries = @('Voyage/Content/Paks/Example_P.pak','Voyage/Content/Paks/Example_P.ucas','Voyage/Content/Paks/Example_P.utoc','Voyage/Content/Paks/Example.ini','Voyage/Mods/ExampleCatalogue/ExampleCatalogue.uplugin'); modType = $null },
        @{ name = 'logicmods'; arguments = @{ LogicMods = $true }; entries = @('LogicMods/','LogicMods/Example_P.pak','LogicMods/Example_P.ucas','LogicMods/Example_P.utoc'); modType = $null },
        @{ name = 'logicmods-mixed'; arguments = @{ LogicMods = $true; AdditionalFile = @($ini) }; entries = @('Voyage/Content/Paks/LogicMods/Example_P.pak','Voyage/Content/Paks/LogicMods/Example_P.ucas','Voyage/Content/Paks/LogicMods/Example_P.utoc','Voyage/Content/Paks/Example.ini'); modType = $null },
        @{ name = 'logicmods-plugin'; arguments = @{ LogicMods = $true; ContentPluginDescriptor = $plugin }; entries = @('Voyage/Content/Paks/LogicMods/Example_P.pak','Voyage/Content/Paks/LogicMods/Example_P.ucas','Voyage/Content/Paks/LogicMods/Example_P.utoc','Voyage/Mods/ExampleCatalogue/ExampleCatalogue.uplugin'); modType = $null },
        @{ name = 'paks-root'; arguments = @{ PaksRoot = $true }; entries = @('Example_P.pak','Example_P.ucas','Example_P.utoc','vortex_override_instructions.json'); modType = 'thelastcaretaker-pakalt' },
        @{ name = 'root-extra'; arguments = @{ RootLayout = $true; AdditionalFile = @($ini); AdditionalDestination = @('Voyage/Mods/Example/Example.ini') }; entries = @('Voyage/Content/Paks/Example_P.pak','Voyage/Content/Paks/Example_P.ucas','Voyage/Content/Paks/Example_P.utoc','Voyage/Mods/Example/Example.ini'); modType = $null }
    )
    foreach ($case in $cases) {
        $archive = Join-Path $root ($case.name + '.zip')
        $arguments = $case.arguments
        $result = & $producer -Container $utoc -Archive $archive @arguments
        $expectedLayout = if ($case.name -in @('logicmods-mixed','logicmods-plugin','root-extra')) { 'root' }
            elseif ($case.name -ceq 'paks-root') { 'pakalt' }
            else { $case.name }
        if ($result.status -cne 'verified' -or $result.layout -cne $expectedLayout -or
            $result.overrideModType -cne $case.modType) {
            throw "Producer result mismatch: $($case.name)"
        }
        $zip = [IO.Compression.ZipFile]::OpenRead($archive)
        try {
            $entries = @($zip.Entries | ForEach-Object { $_.FullName })
            if (Compare-Object $case.entries $entries) { throw "ZIP entries mismatch: $($case.name)" }
        } finally { $zip.Dispose() }
    }
    [pscustomobject]@{ status = 'passed'; layouts = $cases.Count }
} finally {
    $expectedPrefix = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/VortexArchiveTests')).TrimEnd('\') + '\'
    if (-not $root.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside Vortex test storage: $root"
    }
    Remove-Item -LiteralPath $root -Recurse -Force
}

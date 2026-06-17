$ErrorActionPreference = "Stop"
Write-Host "[test] foliage_un06" -ForegroundColor Cyan

function Get-ExpectedProtocols([string]$preset) {
    switch ($preset) {
        "core" { return @("core") }
        "bus" { return @("bus") }
        "external" { return @("external") }
        "core-bus" { return @("core", "bus") }
        "core-external" { return @("core", "external") }
        "bus-external" { return @("bus", "external") }
        "all" { return @("core", "bus", "external") }
        default { throw "unknown protocolPreset: $preset" }
    }
}

$root = $PSScriptRoot
$manifestPath = Join-Path $root "plugin.monted.json"
if (-not (Test-Path $manifestPath)) { throw "plugin.monted.json missing" }

$json = Get-Content $manifestPath -Raw | ConvertFrom-Json
foreach ($field in @("id", "name", "version", "type", "protocolPreset", "protocols")) {
    if (-not $json.PSObject.Properties.Name.Contains($field)) { throw "manifest field missing: $field" }
}

$expected = @(Get-ExpectedProtocols $json.protocolPreset)
$actual = @($json.protocols)
$missing = @($expected | Where-Object { $actual -notcontains $_ })
$extra = @($actual | Where-Object { $expected -notcontains $_ })
if ($missing.Count -gt 0) { throw "manifest protocols missing from preset '$($json.protocolPreset)': $($missing -join ', ')" }
if ($extra.Count -gt 0) { throw "manifest protocols not allowed by preset '$($json.protocolPreset)': $($extra -join ', ')" }

$adapterFiles = @{
    core = "src\core_abi\core_entry.cpp"
    bus = "src\bus\bus_handler.cpp"
    external = "src\external\external_tool_schema.json"
}

foreach ($protocol in $expected) {
    $path = Join-Path $root $adapterFiles[$protocol]
    if (-not (Test-Path $path)) { throw "selected protocol '$protocol' file missing: $($adapterFiles[$protocol])" }
}

foreach ($protocol in $adapterFiles.Keys) {
    if ($expected -notcontains $protocol) {
        $path = Join-Path $root $adapterFiles[$protocol]
        if (Test-Path $path) { throw "unselected protocol '$protocol' adapter exists: $($adapterFiles[$protocol])" }
    }
}

# Check new common source files exist
$commonFiles = @(
    "src\common\foliage_type.h",
    "src\common\foliage_prng.h",
    "src\common\foliage_terrain.h",
    "src\common\foliage_scatter.h",
    "src\common\foliage_scatter.cpp",
    "src\common\foliage_paint.h",
    "src\common\foliage_paint.cpp",
    "src\common\foliage_cluster.h",
    "src\common\foliage_cluster.cpp"
)
foreach ($f in $commonFiles) {
    $p = Join-Path $root $f
    if (-not (Test-Path $p)) { Write-Host "  WARN: common file not found: $f" -ForegroundColor Yellow }
    else { Write-Host "  OK: $f" -ForegroundColor Gray }
}

# Check test GUI file
$guiFile = Join-Path $root "src\test_gui\main.cpp"
if (-not (Test-Path $guiFile)) { Write-Host "  WARN: test GUI file not found: src\test_gui\main.cpp" -ForegroundColor Yellow }
else { Write-Host "  OK: src\test_gui\main.cpp" -ForegroundColor Gray }

# Check build output
$dllPath = Join-Path $root "build\foliage_un06.dll"
$guiPath = Join-Path $root "build\foliage_test_gui.exe"
if (Test-Path $dllPath) { Write-Host "  DLL: $dllPath" -ForegroundColor Green }
else { Write-Host "  INFO: DLL not built yet (run build.ps1)" -ForegroundColor Yellow }
if (Test-Path $guiPath) { Write-Host "  GUI: $guiPath" -ForegroundColor Green }
else { Write-Host "  INFO: Test GUI not built yet (run build.ps1)" -ForegroundColor Yellow }

# Validate protocols doc
if (-not (Test-Path (Join-Path $root "PROTOCOLS.md"))) { throw "PROTOCOLS.md missing" }
if (-not (Test-Path (Join-Path $root "PLUGIN_SPEC.md"))) { throw "PLUGIN_SPEC.md missing" }
if (-not (Test-Path (Join-Path $root "AGENTS.md"))) { throw "AGENTS.md missing" }
if (-not (Test-Path (Join-Path $root "TASK.md"))) { throw "TASK.md missing" }

# Validate manifest commands have permissions
if ($json.commands) {
    foreach ($cmd in $json.commands) {
        if (-not $cmd.permission) {
            Write-Host "  WARN: command '$($cmd.id)' has no permission" -ForegroundColor Yellow
        }
        Write-Host "  Command: $($cmd.id) [permission=$($cmd.permission), destructive=$($cmd.destructive)]" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Manifest/protocol consistency OK: $($json.name) v$($json.version) [$($actual -join ', ')]" -ForegroundColor Green
exit 0

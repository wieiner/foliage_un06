$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$OutDir = Join-Path $root "build"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

$manifest = Get-Content (Join-Path $root "plugin.monted.json") -Raw | ConvertFrom-Json
$name = $manifest.name
$version = $manifest.version

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "[build] $name v$version (foliage scatter plugin)" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# Find MSVC
$vcvars = Get-ChildItem -Path 'C:\Program Files\Microsoft Visual Studio' -Filter 'vcvarsall.bat' -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $vcvars) {
    $vcvars = Get-ChildItem -Path 'C:\Program Files (x86)\Microsoft Visual Studio' -Filter 'vcvarsall.bat' -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $vcvars) {
    Write-Host "No MSVC found; skipping native build." -ForegroundColor Yellow
    exit 0
}

$vcEnv = "`"$vcvars`" x64 >nul"

# ----------------------------------------------------------------------
# 1. Plugin DLL (core_abi + bus + common)
# ----------------------------------------------------------------------
Write-Host "" -ForegroundColor White
Write-Host "[1/3] Building foliage_un06.dll ..." -ForegroundColor Green

$cppFiles = @(
    'src\core_abi\core_entry.cpp',
    'src\bus\bus_handler.cpp',
    'src\bus\bus_handler_advanced.cpp',
    'src\common\foliage_json.cpp',
    'src\common\foliage_scatter.cpp',
    'src\common\foliage_paint.cpp',
    'src\common\foliage_cluster.cpp',
    'src\common\foliage_query.cpp',
    'src\common\foliage_simulate.cpp',
    'src\common\foliage_analyze.cpp',
    'src\common\foliage_falloff.cpp',
    'src\common\foliage_noise.cpp',
    'src\common\foliage_mask.cpp',
    'src\common\foliage_pattern.cpp',
    'src\common\foliage_biome.cpp'
)

$srcArgs = ($cppFiles | ForEach-Object { '"' + (Join-Path $root $_) + '"' }) -join ' '
$dllOut = Join-Path $OutDir "$name.dll"

$dllCmd = "cl /nologo /std:c++17 /O2 /MT /EHsc /bigobj /LD $srcArgs /link /OUT:`"$dllOut`""
$fullCmd = "cd /d `"$root`" && $vcEnv && $dllCmd"

cmd /c $fullCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Plugin DLL build failed (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host "  -> $dllOut" -ForegroundColor Green

# ----------------------------------------------------------------------
# 2. Standalone test GUI
# ----------------------------------------------------------------------
Write-Host "" -ForegroundColor White
Write-Host "[2/3] Building foliage_test_gui.exe ..." -ForegroundColor Green

$guiCpp = Join-Path $root 'src\test_gui\main.cpp'
$guiExe = Join-Path $OutDir "foliage_test_gui.exe"

# Compile GUI as a windows subsystem app, linking against the same common sources
$guiCommonCpp = @(
    'src\common\foliage_scatter.cpp',
    'src\common\foliage_paint.cpp',
    'src\common\foliage_cluster.cpp',
    'src\common\foliage_query.cpp',
    'src\common\foliage_simulate.cpp',
    'src\common\foliage_analyze.cpp',
    'src\common\foliage_falloff.cpp',
    'src\common\foliage_noise.cpp',
    'src\common\foliage_mask.cpp',
    'src\common\foliage_pattern.cpp',
    'src\common\foliage_biome.cpp',
    'src\common\foliage_json.cpp'
)
$guiSrcArgs = ($guiCommonCpp | ForEach-Object { '"' + (Join-Path $root $_) + '"' }) -join ' '
$guiSrcArgs = "`"$guiCpp`" $guiSrcArgs"

$guiCmd = "cl /nologo /std:c++17 /O2 /MT /EHsc /bigobj $guiSrcArgs /link /SUBSYSTEM:WINDOWS /OUT:`"$guiExe`" user32.lib gdi32.lib comctl32.lib"
$fullGuiCmd = "cd /d `"$root`" && $vcEnv && $guiCmd"

cmd /c $fullGuiCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: Test GUI build failed (exit $LASTEXITCODE) — DLL built successfully" -ForegroundColor Yellow
} else {
    Write-Host "  -> $guiExe" -ForegroundColor Green
}

# ----------------------------------------------------------------------
# 3. Golden test executable
# ----------------------------------------------------------------------
Write-Host "" -ForegroundColor White
Write-Host "[3/3] Building golden_test.exe ..." -ForegroundColor Green

$goldenCpp = Join-Path $root 'src\test_gui\golden_test.cpp'
$goldenExe = Join-Path $OutDir "golden_test.exe"

$goldenCommonCpp = @(
    'src\common\foliage_scatter.cpp',
    'src\common\foliage_paint.cpp',
    'src\common\foliage_cluster.cpp',
    'src\common\foliage_noise.cpp',
    'src\common\foliage_falloff.cpp'
)
$goldenSrcArgs = ($goldenCommonCpp | ForEach-Object { '"' + (Join-Path $root $_) + '"' }) -join ' '
$goldenSrcArgs = "`"$goldenCpp`" $goldenSrcArgs"

$goldenCmd = "cl /nologo /std:c++17 /O2 /MT /EHsc /bigobj $goldenSrcArgs /link /OUT:`"$goldenExe`""
$fullGoldenCmd = "cd /d `"$root`" && $vcEnv && $goldenCmd"

cmd /c $fullGoldenCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: Golden test build failed (exit $LASTEXITCODE)" -ForegroundColor Yellow
} else {
    Write-Host "  -> $goldenExe" -ForegroundColor Green
    # Run golden tests
    & $goldenExe
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: Golden tests failed (exit $LASTEXITCODE)" -ForegroundColor Yellow
    }
}

# ----------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------
Write-Host "" -ForegroundColor White
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "[build] Complete: $name v$version" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Plugin DLL : $dllOut" -ForegroundColor White
if (Test-Path $guiExe) {
    Write-Host "  Test GUI   : $guiExe" -ForegroundColor White
}
Write-Host ""
Write-Host "  Quick test (headless):"
Write-Host "    EditorApp.exe --bus-call foliage_un06.scatter --payload '{`"areaWidth`":200,`"areaDepth`":200,`"density`":0.03,`"seed`":42}' --allow-all-permissions"
Write-Host "  Standalone GUI:"
Write-Host "    .\build\foliage_test_gui.exe"
Write-Host ""
exit 0

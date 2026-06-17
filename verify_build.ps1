$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$src = Join-Path $root "src\test_gui\dll_verify.cpp"
$out = Join-Path $root "build\dll_verify.exe"

$vcvars = Get-ChildItem -Path 'C:\Program Files\Microsoft Visual Studio' -Filter 'vcvarsall.bat' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $vcvars) {
    Write-Host "No MSVC found"
    exit 1
}

$cmd = "cd /d `"$root`" && `"$vcvars`" x64 >nul && cl /nologo /std:c++17 /O2 /MD /EHsc `"$src`" /link /OUT:`"$out`""
cmd /c $cmd

if (Test-Path $out) {
    Write-Host "Running all 17 command verification..."
    & $out
} else {
    Write-Host "Build failed"
}

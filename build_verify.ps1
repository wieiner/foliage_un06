$root = $PSScriptRoot
$src = Join-Path $root "src\test_gui\quick_verify.cpp"
$out = Join-Path $root "build\quick_verify.exe"
$vcvars = (Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Filter vcvarsall.bat -Recurse -EA 0 | Select-Object -First 1).FullName
if (-not $vcvars) { Write-Host "No MSVC"; exit 1 }
cmd /c "cd /d `"$root`" && `"$vcvars`" x64 >nul && cl /nologo /std:c++17 /O2 /MD /EHsc `"$src`" /link /OUT:`"$out`""
if (Test-Path $out) { & $out } else { Write-Host "Compile failed" }

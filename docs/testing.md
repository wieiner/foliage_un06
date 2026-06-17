# Testing — foliage_un06

## Build + Basic Validation
```powershell
pwsh .\build.ps1          # Builds DLL + test GUI (14 cpp files)
pwsh .\test.ps1           # Manifest/protocol/file consistency
pwsh .\verify_build.ps1   # DLL verification: all 43 handlers + V4 API
```

## Standalone Test GUI
```powershell
.\build\foliage_test_gui.exe
```
- F5: Scatter
- F6: Paint brush at cursor
- F7: Toggle erase mode
- F8: Clear
- F9: Export JSON
- +/-: Zoom
- Arrows: Pan
- Ctrl+Up/Down: Change seed
- Ctrl+R: Reset view

## DLL Handler Verification
```powershell
pwsh .\verify_build.ps1
```
Tests all 43 `handle_*ToBuf` exports + MontEd_GetPluginApiV4 + MontEd_GetBusHandlerDescriptor.

## Headless Engine Tests
```powershell
pwsh .\scripts\engine_smoke.ps1   # if script exists
# or manually:
& "C:\mont-ed\build-codex-new\Debug\EditorApp.exe" --bus-call foliage_un06.inspect --payload "{}" --allow-all-permissions
& "C:\mont-ed\build-codex-new\Debug\EditorApp.exe" --bus-call foliage_un06.health_check --payload "{}" --allow-all-permissions
```

## Expected Outputs
- `build\foliage_un06.dll` — plugin DLL
- `build\foliage_test_gui.exe` — standalone test app
- `build\dll_verify.exe` — DLL verification tool

## Known Test Limitations
- Negative tests (null/empty payload) are partially implemented — some handlers accept empty payload gracefully
- No structured golden tests yet — deterministic PRNG can be verified by comparing instance counts per seed
- Engine smoke test requires running Mont-ED EditorApp.exe — not automatable without the engine built
- No file system fuzzing tests — export/import paths need manual validation

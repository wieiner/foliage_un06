# Engine Integration — foliage_un06

## Prerequisites
- Mont-ED engine built at `C:\mont-ed`
- `MONTED_PLUGIN_ROOT` set to `C:\mont-ed-plg` (or plugin in that tree)

## Installation Layout
```
C:\mont-ed-plg\foliage_un06\
  plugin.monted.json
  build\foliage_un06.dll
  schemas\...
  demo\payloads\...
```

## Build Plugin
```powershell
cd C:\mont-ed-plg\foliage_un06
pwsh .\build.ps1
```

## Engine Audit
```powershell
cd C:\mont-ed
& ".\scripts\build_debug.ps1"                           # if not built
& ".\build-codex-new\Debug\EditorApp.exe" --smoke-test   # verify engine runs
```

## Headless Bus-Call Tests
```powershell
$engine = "C:\mont-ed\build-codex-new\Debug\EditorApp.exe"

# Inspect plugin
& $engine --bus-call foliage_un06.inspect --payload "{}" --allow-all-permissions

# Scatter grass
& $engine --bus-call foliage_un06.scatter --payload '{"storeKey":"demo_grass","areaWidth":200,"areaDepth":200,"density":0.03,"seed":42,"includePreviewMesh":true,"maxInstancesInResponse":500}' --allow-all-permissions

# Analyze
& $engine --bus-call foliage_un06.analyze --payload '{"storeKey":"demo_grass"}' --allow-all-permissions

# Health check
& $engine --bus-call foliage_un06.health_check --payload "{}" --allow-all-permissions

# List commands
& $engine --bus-call foliage_un06.list_commands --payload '{"compact":true}' --allow-all-permissions
```

## Visibility in Editor

### Plugin Browser
- Open Mont-ED editor
- Navigate to Plugin Browser panel
- Find `foliage_un06` — should show `bus-registered` or `native-loaded`
- Check diagnostics tab for warnings/errors

### Plugin Tool Window
- Search for any `foliage_un06.*` command
- Select a command, edit payload, execute
- Results appear in the result panel
- Use dry-run/confirm for destructive commands

### PluginMeshResult / Viewport Preview
Scatter and generate_mesh commands output `mesh` summary:
```json
{
  "mesh": {
    "vertexCount": N,
    "indexCount": N,
    "triangleCount": N,
    "boundsMin": [x,y,z],
    "boundsMax": [x,y,z],
    "boundingBox": [minX,minY,minZ,maxX,maxY,maxZ]
  }
}
```
The engine's PluginMeshResult panel renders the bounding box in the 3D viewport.

## Troubleshooting
1. **Plugin not visible**: Check `MONTED_PLUGIN_ROOT` env var or `--plugin-root` flag
2. **DLL not loading**: Run `dumpbin /dependents build\foliage_un06.dll` to verify VC++ runtime
3. **Commands not found**: Check manifest has all 43 commands (`grep -c '"id"' plugin.monted.json`)
4. **ABI mismatch**: Ensure engine PluginApi v4 is present; plugin provides V3 fallback

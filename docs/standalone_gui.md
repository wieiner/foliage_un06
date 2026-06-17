# Standalone Test GUI — foliage_un06

## How to Run
```powershell
cd C:\mont-ed-plg\foliage_un06
.\build\foliage_test_gui.exe
```

## Window Layout
- **Left panel (320px)**: Parameter editors and action buttons
- **Right panel (canvas)**: 2D top-down view of scatter results
- **Bottom bar**: Status text, instance info on hover

## Controls

| Action | Key/Button |
|--------|------------|
| Scatter | F5 or "🔀 Scatter" button |
| Paint brush | F6 or "🖌 Paint Brush" button |
| Toggle erase | F7 or "🧹 Toggle Erase Mode" button |
| Clear | F8 or "🗑 Clear" button |
| Export JSON | F9 or "📋 Export JSON" button |
| Zoom in | + |
| Zoom out | - |
| Pan | Arrow keys |
| Reset view | Ctrl+R |
| Change seed | Ctrl+↑/↓ |

## Mouse Interaction
- **Click on canvas**: Position brush at world coordinates
- **Hover**: Shows nearest instance info in status bar

## Parameters
- Area Width, Area Depth
- Density, Scale Min/Max
- Slope Max (°), Height Min/Max
- Seed
- Align to Normal checkbox
- Random Rotation checkbox
- Brush Size
- Terrain mode: Hills / Flat
- Mesh type: Cross billboard / Cube

## Visualization
- Green → Red gradient by terrain height
- White line: instance yaw direction indicator
- Grid lines: 10-unit spacing
- Green circle: paint brush position
- Red circle: erase brush position
- Yellow highlight: hovered instance

## Covered Commands
- scatter, paint, erase, clear, export_state (JSON export)

## Not Yet Covered
- fill, simulate, biome, radial, along_path, clump, brush_shapes
- noise_modulate, layer_mask, exclusion_zone
- analyze, sample_terrain, statistics_grouped
- LOD/wind/collision configuration
- import_state, checkpoint, merge
- batch, benchmark, list_commands, health_check

## Building
```powershell
pwsh .\build.ps1    # Builds both DLL and GUI
```
GUI links against shared common/ logic directly (not the DLL). 15 cpp files compiled.

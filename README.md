# foliage_un06 — Foliage Scatter Plugin for Mont-ED

Foliage / Vegetation painting over Instanced Static Mesh. Replicates Unreal Engine's FoliageType → FoliageTool → HISM pipeline for the Mont-ED engine.

**Version:** 0.5.0 | **Status:** production-preview | **Commands:** 43

```
FoliageType (mesh, density, scale range, random rotation, align to normal,
            slope & height filters, seed) → deterministic Poisson-disc scatter
            → instance clusters → ISM output
```

## Quick Start

```powershell
cd C:\mont-ed-plg\foliage_un06

# Build everything
pwsh .\build.ps1

# Validate
pwsh .\test.ps1

# Standalone test GUI
.\build\foliage_test_gui.exe

# Verify DLL exports (all 43 handlers)
pwsh .\verify_build.ps1
```

## Project Structure

```
├── plugin.monted.json              # Engine manifest (43 commands)
├── build.ps1 / test.ps1            # Build + validation scripts
├── schemas/                        # JSON Schema files
├── demo/payloads/                  # Ready-to-use bus-call payloads
├── docs/                           # 12 documentation files
├── src/
│   ├── core_abi/core_entry.cpp     # V4 MontEdPluginApi adapter
│   ├── bus/bus_handler.cpp         # 43 bus command handlers
│   ├── test_gui/main.cpp           # Standalone Win32 GDI test app
│   └── common/                     # Shared logic (15 modules)
│       ├── foliage_type.h          # FoliageType + Instance (v1)
│       ├── foliage_type_v2.h       # FoliageTypeV2 (UE-compatible)
│       ├── foliage_command_registry.h  # Single source of truth
│       ├── foliage_prng.h          # xoshiro256** PRNG
│       ├── foliage_terrain.h       # TerrainSampler interface
│       ├── foliage_scatter.h/cpp   # PoissonDiscScatter
│       ├── foliage_paint.h/cpp     # Brush paint/erase
│       ├── foliage_cluster.h/cpp   # Cluster mesh output
│       ├── foliage_query.h/cpp     # Spatial/GIS index
│       ├── foliage_simulate.h/cpp  # Ecosystem simulation
│       ├── foliage_analyze.h/cpp   # Statistical analysis
│       ├── foliage_falloff.h/cpp   # Edge density falloff
│       ├── foliage_noise.h/cpp     # Perlin noise modulation
│       ├── foliage_mask.h/cpp      # Layer masks + exclusion zones
│       ├── foliage_pattern.h/cpp   # Radial/path/clump patterns
│       ├── foliage_biome.h/cpp     # Biome zone system
│       └── foliage_json.h/cpp      # Structured JSON parser
└── build/                          # Build outputs (DLL, EXE)
```

## Engine Integration

Plugin auto-discovered when `MONTED_PLUGIN_ROOT` points to `C:\mont-ed-plg`.

```powershell
# Build engine
cd C:\mont-ed
pwsh .\scripts\build_debug.ps1

# Plugin audit
pwsh .\scripts\run_plugin_audit.ps1

# Headless bus-call
.\build-codex-new\Debug\EditorApp.exe --bus-call foliage_un06.scatter --payload '{"areaWidth":200,"areaDepth":200,"density":0.03,"seed":42}' --allow-all-permissions --plugin-root "C:\mont-ed-plg"
```

See [docs/engine_integration.md](docs/engine_integration.md) for full instructions.

## Documentation

| Document | Content |
|----------|---------|
| [docs/architecture.md](docs/architecture.md) | Layer diagram, data flow, ABI contract |
| [docs/command_surface.md](docs/command_surface.md) | Full 43-command table with permissions |
| [docs/data_model.md](docs/data_model.md) | UE → Mont-ED FoliageTypeV2 mapping |
| [docs/engine_integration.md](docs/engine_integration.md) | Engine setup, bus-call examples |
| [docs/standalone_gui.md](docs/standalone_gui.md) | GUI controls, hotkeys |
| [docs/testing.md](docs/testing.md) | Test commands, expected outputs |
| [docs/performance.md](docs/performance.md) | Benchmarks, limits, complexity |
| [docs/security.md](docs/security.md) | Thread safety, ABI, known weaknesses |
| [docs/known_limitations.md](docs/known_limitations.md) | 17 documented limitations |
| [docs/hardening_audit.md](docs/hardening_audit.md) | Original baseline audit |
| [docs/source_hygiene.md](docs/source_hygiene.md) | Clean tree policy |
| [docs/phase_report.md](docs/phase_report.md) | Phase-by-phase completion log |

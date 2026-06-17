# Hardening Audit — foliage_un06 (Phase 0 Baseline)

**Date:** 2026-06-17  
**Auditor:** Claude (read-only baseline)  
**Plugin path:** `C:\mont-ed-plg\foliage_un06`  
**Engine path:** `C:\mont-ed` (clone at `/tmp/mont-ed`)

---

## 1. Actual Numbers — Command Surface Mismatch

| Surface | Count | Notes |
|---------|-------|-------|
| Exported `handle_*ToBuf` handlers | **41** | 41 unique implementations in `bus_handler.cpp` |
| `plugin.monted.json` commands | **17** | Only phases 1+2 (7+10). Phases 3+4 missing entirely |
| `executeCommand` routes | **41** | All 41 routed in `core_entry.cpp` |
| `getBusHandlersJson` entries | **~28** | In `core_entry.cpp` — count inferred from commandId strings |
| `getCapabilitiesJson` busHandlers | **~29** | Inline string in `core_entry.cpp` |
| Legacy `MontEd_GetBusHandlerDescriptor` | **1** | Says "29 commands" — returns single-command descriptor |
| `dll_verify.cpp` tested handlers | **~29** | Original 7 + phase 2 (10) + phase 3 (12) |
| `quick_verify.cpp` tested handlers | **12** | Phase 4 only |
| Standalone GUI commands | **~5** | Scatter, paint, erase, clear, export |

### Full Handler List (41)

```
01 handle_scatterToBuf        02 handle_paintToBuf
03 handle_eraseToBuf          04 handle_getTypesToBuf
05 handle_addTypeToBuf        06 handle_inspectToBuf
07 handle_generateMeshToBuf   08 handle_reapplyToBuf
09 handle_fillToBuf           10 handle_simulateToBuf
11 handle_queryToBuf          12 handle_placeSingleToBuf
13 handle_removeInstancesToBuf 14 handle_analyzeToBuf
15 handle_exportStateToBuf    16 handle_importStateToBuf
17 handle_densityFalloffToBuf 18 handle_layerMaskToBuf
19 handle_noiseModulateToBuf  20 handle_zOffsetToBuf
21 handle_cullDistanceToBuf   22 handle_batchToBuf
23 handle_checkpointToBuf     24 handle_mergeToBuf
25 handle_mirrorRegionToBuf   26 handle_exclusionZoneToBuf
27 handle_presetToBuf         28 handle_benchmarkToBuf
29 handle_sampleTerrainToBuf  30 handle_biomeToBuf
31 handle_radialToBuf         32 handle_alongPathToBuf
33 handle_clumpToBuf          34 handle_brushShapesToBuf
35 handle_randomReplaceToBuf   36 handle_filterByTypeToBuf
37 handle_transformInstancesToBuf 38 handle_windParamsToBuf
39 handle_collisionSettingsToBuf 40 handle_lodConfigToBuf
41 handle_statisticsGroupedToBuf
```

---

## 2. Manifest — What Mont-ED Sees

**17 commands in `plugin.monted.json`** (v0.3.0, lifecycle `"2-expansion"`):

```
foliage_un06.inspect
foliage_un06.scatter
foliage_un06.paint
foliage_un06.erase
foliage_un06.get_types
foliage_un06.add_type
foliage_un06.generate_mesh
foliage_un06.reapply
foliage_un06.fill
foliage_un06.simulate
foliage_un06.query
foliage_un06.place_single
foliage_un06.remove_instances
foliage_un06.analyze
foliage_un06.export_state
foliage_un06.import_state
foliage_un06.density_falloff
```

**Missing from manifest (24 commands):**  handles #18–#41 above.

### Manifest Quality Issues

- No `title` field per command
- No `inputSchema`/`outputSchema` references
- No `category`/`tags`
- No `tools` section (empty in file)
- Version stuck at `0.3.0`
- Lifecycle says `"2-expansion"` but code is at phase 4

---

## 3. Descriptor — What NativePluginLoader Sees

`MontEd_GetBusHandlerDescriptor()` returns a single JSON object with `entry_point: "handle_scatterToBuf"`. The text in `description` says "29 commands" — factually incorrect.

**Risk:** Engine may register only `foliage_un06.scatter` via the native descriptor path, and rely on the manifest for other commands. Since the manifest only has 17, commands 18–41 are invisible through either path.

---

## 4. ExecuteCommand — What V4 API Sees

All 41 commands are correctly routed in `foliage_executeCommand()`. If the engine calls `executeCommand` directly (without manifest), all commands work.

---

## 5. JSON Parsing Quality

**Current approach: fragile string parsing.**

- `ej()` — double extractor: searches for `"key"` then `:` then atof. No bracket/brace awareness.
- `ej_str()` — string extractor: same fragile pattern matching.
- `ej_bool()` — boolean extractor.
- `ej_intArray()` — custom array parser.
- `parseInstances()` — custom instance array parser.

**Risks:**
- Malformed JSON not rejected — silent default values
- Nested objects with same key names cause confusion
- No structural validation
- No error reporting for invalid input
- Buffer overflow risk in output writing with `pos` tracking

---

## 6. Schemas

**No `schemas/` directory exists.** No JSON Schema files for any command.

---

## 7. Build / Test Status

| Script | Status | Notes |
|--------|--------|-------|
| `build.ps1` | ✅ Passes | DLL + GUI build clean, 13 cpp files |
| `test.ps1` | ✅ Passes | Only validates manifest (17 commands) + file existence |
| `verify_build.ps1` | ⚠️ Partial | Tests 29 handlers via dll_verify.cpp |
| `build_verify.ps1` | ⚠️ Partial | Tests 12 handlers via quick_verify.cpp |
| Documentation `.md` | ❌ Stale | References 17 and 29 commands inconsistently |

---

## 8. Build Artifacts in Tree

**In workspace root (not ignored):**
- `*.obj` files (13 files: core_entry.obj, bus_handler.obj, foliage_*.obj, etc.)
- `core_entry.lib`, `core_entry.exp`

**In `build/` (ignored but present):**
- `foliage_un06.dll`, `foliage_test_gui.exe`, `dll_verify.exe`, `quick_verify.exe`
- `test_export.json` (temporary export)

**Missing from `.gitignore`:**
- `*.lib`
- `*.exp`
- `*.ilk`
- `*.pdb` (if generated)
- `reports/`
- `test_export.json` equivalent files

---

## 9. Engine Files Read

| File | Path | Key finding |
|------|------|-------------|
| `monted_plugin.h` | `sdk/monted/` | V4 API: structSize, identity, getCapabilitiesJson, getBusHandlersJson, getUiPanelsJson, getSceneToolsJson, getBlueprintNodesJson, executeCommand |
| `monted_abi.h` | `sdk/monted/` | Status codes, MONTED_ABI_VERSION_CURRENT=4 |
| `NativePluginLoader.cpp` | `engine/plugins/` | Loads DLL, probes V4→V3, reads descriptor, registers bus handlers |
| `PluginManager.cpp` | `engine/plugins/` | Manifest→bus registration, external tool fallbacks |
| `MessageBus.h` | `engine/plugins/` | Bus command structure, dispatch, permissions |

**Engine-capable surfaces (no engine changes needed):**
1. Plugin manifest → bus commands (currently 17)
2. Native descriptor → bus handler (single command)
3. `executeCommand` → V4 direct routing (all 41)
4. `getSceneToolsJson` → scene tool contributions
5. `getUiPanelsJson` → panel contributions

---

## 10. Standardalone GUI Coverage

`src/test_gui/main.cpp`: Win32 GDI app, ~1400 lines.

**Covers:**
- Scatter (buttons + auto-run)
- Paint (brush circle)
- Erase (toggle mode)
- Clear
- Export JSON

**Does NOT cover:**
- Biomes, patterns (radial/clump/path), masks, noise, LOD, wind, collision
- Analysis panel, statistics, benchmark
- Import/export state
- Presets
- Command registry
- Dry-run preview
- Undo/redo (checkpoints)

---

## 11. Architecture Issues Summary

| Issue | Severity | Detail |
|-------|----------|--------|
| **Command surface mismatch** | 🔴 Critical | 41 handlers vs 17 manifest vs 28 busHandlersJson |
| **No schemas** | 🔴 Critical | No input/output validation |
| **Fragile JSON parsing** | 🟠 High | String-based, no structural validation |
| **No command registry** | 🟠 High | No single source of truth |
| **FoliageType model v1** | 🟠 High | Missing UE-style fields (procedural, collision, LOD, wind) |
| **No stable instance IDs** | 🟠 High | Cannot undo by ID, cannot diff stores |
| **Instance store ad-hoc** | 🟡 Medium | Bare map+mutex, no spatial index shared |
| **Dry-run incomplete** | 🟡 Medium | Several destructive ops lack dryRun |
| **Legacy descriptor stale** | 🟡 Medium | Single-command, wrong count |
| **Build artifacts in tree** | 🟡 Medium | .obj/.lib/.exp at root |
| **GUI minimal** | 🟡 Medium | Only scatter/paint/erase |
| **No unit/golden tests** | 🟡 Medium | Only DLL verify |
| **No engine smoke script** | 🟡 Medium | No automated engine integration test |
| **No file safety** | 🟠 High | Export/import no path traversal protection |
| **No thread safety audit** | 🟡 Medium | Mutexes exist but held during long compute |

---

## 12. Exact First-Fix Sequence

1. **Create `src/common/foliage_command_registry.h`** — single source of truth for all 41+ commands
2. **Sync `plugin.monted.json`** — add all 41 commands with titles, permissions, destructive flags
3. **Sync `getCapabilitiesJson`** — list all 41 bus handlers
4. **Sync `getBusHandlersJson`** — 41 entries with correct entryPoint names
5. **Sync `MontEd_GetBusHandlerDescriptor`** — return full command list
6. **Sync `dll_verify.cpp`** — test all 41 handlers
7. **Update `.gitignore`** — cover .lib, .exp, .ilk, reports/
8. **Clean build artifacts** from workspace root
9. **Create `schemas/`** directory with input schemas
10. **Create `src/common/foliage_json.h/cpp`** — structured JSON parsing

---

## 13. Next Step

**Phase 1: Source Hygiene + Phase 2: Command Contract Sync.**
Start immediately after audit approval.

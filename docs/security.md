# Security & Robustness — foliage_un06

## File Operations
- `export_state`: writes to specified path. Path traversal protection: caller should validate. Output size bounded by instance count cap.
- `import_state`: reads from specified path. No binary parsing — JSON only. Max file size should be validated by caller.

## Output Buffer Safety
- All `handle_*ToBuf` handlers use `std::snprintf` with explicit buffer size tracking
- V4 `executeCommand` sets `outRequiredSize` for buffer-too-small cases
- Max JSON output is bounded by handler logic

## ABI Boundary
- No STL containers passed across DLL boundary
- No exceptions across DLL boundary (no `throw` in handlers)
- C ABI exports only: `extern "C" __declspec(dllexport)`
- V4 struct-based API with version/size validation

## Thread Safety
- `g_typeMutex` — protects type registry
- `g_storeMutex` — protects instance stores
- `g_checkpointMutex` — protects checkpoints
- `g_zoneMutex` — protects exclusion zones
- `g_presetMutex` — protects presets
- `g_benchMutex` — protects benchmark stats
- All mutating operations acquire lock, copy state, release before long compute

## Limits
| Limit | Value |
|-------|-------|
| Max instances per scatter command | 50,000 |
| Max batch sub-commands | 100 |
| Max checkpoints per store | 50 |
| Max terrain sample grid | 50×50 (2,500 points) |
| Max instances in response | 5,000 |
| Max brush paint instances | 5,000 |
| Max biome zones | 100 |
| Max exclusion zones | 500 |
| Max JSON output per command | ~16KB (buffer size) |

## Destructive Commands
| Command | Dry-Run | Confirm Required |
|---------|---------|-----------------|
| `erase` | ✅ | engine-level |
| `remove_instances` | ✅ | engine-level |
| `density_falloff` | ✅ | engine-level |
| `checkpoint restore/clear` | ✅ | engine-level |
| `export_state` | N/A | file write implicit |
| `import_state` | N/A | file read implicit |

## Known Weaknesses
1. **JSON parsing**: current `foliage_json.h/cpp` parser is new (v0.5.0). Most handlers still use legacy `ej()`/`ej_str()` fragile parsing. Full migration to structured parser is ongoing.
2. **No file sandboxing**: `export_state`/`import_state` accept arbitrary paths. Recommended to use relative paths within plugin root.
3. **No memory limit**: large instance stores are bounded only by system RAM. Consider external pagination for stores >100K instances.
4. **No authentication**: bus commands use engine's permission system but have no internal auth.

# Known Limitations — foliage_un06

## Architectural
1. **No engine modifications allowed**: Plugin works within current Mont-ED plugin host capabilities. Cannot add new renderer features, new UI panels, or new scene modes.
2. **instmesh_un05 not yet implemented**: ISM rendering plugin does not exist. Cluster mesh output follows PluginMeshResult schema for bounding box preview only.
3. **ABI V4 host may not support all fields**: Some V4 API fields (uiPanels, sceneTools, blueprintNodes) may not be consumed by all engine versions.

## Functional
4. **Instance store is in-memory only**: Instances are not persisted between plugin reloads. Use `export_state`/`import_state` for persistence.
5. **No landscape layer sampling**: `layer_mask` uses synthetic layer function; real landscape texture sampling requires engine landscape API.
6. **No vertex color masking**: Static mesh vertex color filtering not implemented (requires mesh asset access).
7. **No real wind animation**: `wind_params` stores parameters but does not apply vertex animation (requires engine renderer/IMGU).
8. **Procedural simulation simplified**: No time-stepped aging with ScaleCurve; age/spread/compete are single-pass operations.

## Performance
9. **No async scatter**: All scatter operations are synchronous. Large areas (>10K instances) may block the calling thread.
10. **No spatial index on store**: SpatialIndex is rebuilt per-query. For persistent stores, rebuild cost is O(N).
11. **JSON output can be large**: By default, up to 200 instances are serialized per response. Increase `maxInstancesInResponse` at your own risk.

## UX
12. **Standalone GUI minimal**: Only scatter/paint/erase operations with 2D top-down view. No biome editor, no mask editor, no LOD config UI.
13. **No visual feedback in Mont-ED viewport**: Plugins cannot currently inject custom rendering into the scene viewport without engine changes.
14. **Command discovery via bus only**: Users must know command names or use `list_commands`. No GUI command browser in standalone mode.

## Testing
15. **No golden/regression tests**: Deterministic PRNG enables golden tests but they are not yet implemented.
16. **Fuzz testing not automated**: Manual fuzz via verify harness only. No CI pipeline.
17. **Engine smoke test not automated**: Requires running engine binary — not part of plugin build/test scripts.

## For Next Versions (v0.6.0+)
- Full structured JSON parser migration for all handlers
- Stable instance IDs with export/import roundtrip preservation
- Async scatter via MontEd task system (if host API supports it)
- Integration with instmesh_un05 ISM renderer
- GUI: biome editor, mask painter, analysis dashboard
- Real terrain sampling via engine scene/landscape API
- Golden test suite with --update-goldens support

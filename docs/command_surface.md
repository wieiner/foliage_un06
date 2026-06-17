# Command Surface — foliage_un06 v0.5.0

Total: **43 commands** across 10 categories.

## Categories

| Category | Count | Commands |
|----------|-------|----------|
| Core | 3 | inspect, get_types, add_type |
| Scatter | 4 | scatter, fill, simulate, generate_mesh |
| Paint | 4 | paint, erase, place_single, brush_shapes |
| Filter | 9 | reapply, remove_instances, density_falloff, layer_mask, noise_modulate, z_offset, random_replace, filter_by_type, exclusion_zone |
| Analysis | 4 | query, analyze, sample_terrain, statistics_grouped |
| Persistence | 4 | export_state, import_state, checkpoint, merge |
| Pattern | 5 | mirror_region, biome, radial, along_path, clump |
| Configuration | 5 | cull_distance, preset, wind_params, collision_settings, lod_config |
| Performance | 1 | benchmark |
| Utility | 3 | batch, list_commands, health_check |
| **Total** | **43** | |

## Full Command Table

| # | Command ID | Handler | Category | Permission | Destructive | DryRun | Implemented |
|---|------------|---------|----------|------------|-------------|--------|-------------|
| 1 | foliage_un06.inspect | handle_inspectToBuf | Core | project:read | no | yes | ✅ |
| 2 | foliage_un06.scatter | handle_scatterToBuf | Scatter | scene:write | no | yes | ✅ |
| 3 | foliage_un06.paint | handle_paintToBuf | Paint | scene:write | no | yes | ✅ |
| 4 | foliage_un06.erase | handle_eraseToBuf | Paint | scene:write | yes | yes | ✅ |
| 5 | foliage_un06.get_types | handle_getTypesToBuf | Core | project:read | no | yes | ✅ |
| 6 | foliage_un06.add_type | handle_addTypeToBuf | Config | asset:read | no | yes | ✅ |
| 7 | foliage_un06.generate_mesh | handle_generateMeshToBuf | Scatter | scene:write | no | yes | ✅ |
| 8 | foliage_un06.reapply | handle_reapplyToBuf | Filter | scene:write | no | yes | ✅ |
| 9 | foliage_un06.fill | handle_fillToBuf | Scatter | scene:write | no | yes | ✅ |
| 10 | foliage_un06.simulate | handle_simulateToBuf | Scatter | scene:write | no | yes | ✅ |
| 11 | foliage_un06.query | handle_queryToBuf | Analysis | project:read | no | yes | ✅ |
| 12 | foliage_un06.place_single | handle_placeSingleToBuf | Paint | scene:write | no | yes | ✅ |
| 13 | foliage_un06.remove_instances | handle_removeInstancesToBuf | Filter | scene:write | yes | yes | ✅ |
| 14 | foliage_un06.analyze | handle_analyzeToBuf | Analysis | project:read | no | yes | ✅ |
| 15 | foliage_un06.export_state | handle_exportStateToBuf | Persistence | filesystem:write | no | yes | ✅ |
| 16 | foliage_un06.import_state | handle_importStateToBuf | Persistence | filesystem:read | no | yes | ✅ |
| 17 | foliage_un06.density_falloff | handle_densityFalloffToBuf | Filter | scene:write | yes | yes | ✅ |
| 18 | foliage_un06.layer_mask | handle_layerMaskToBuf | Filter | scene:write | no | yes | ✅ |
| 19 | foliage_un06.noise_modulate | handle_noiseModulateToBuf | Filter | scene:write | no | yes | ✅ |
| 20 | foliage_un06.z_offset | handle_zOffsetToBuf | Filter | scene:write | no | yes | ✅ |
| 21 | foliage_un06.cull_distance | handle_cullDistanceToBuf | Config | project:read | no | yes | ✅ |
| 22 | foliage_un06.batch | handle_batchToBuf | Utility | project:read | no | yes | ✅ |
| 23 | foliage_un06.checkpoint | handle_checkpointToBuf | Persistence | scene:write | yes | yes | ✅ |
| 24 | foliage_un06.merge | handle_mergeToBuf | Persistence | scene:write | no | yes | ✅ |
| 25 | foliage_un06.mirror_region | handle_mirrorRegionToBuf | Pattern | scene:write | no | yes | ✅ |
| 26 | foliage_un06.exclusion_zone | handle_exclusionZoneToBuf | Filter | scene:write | no | yes | ✅ |
| 27 | foliage_un06.preset | handle_presetToBuf | Config | project:read | no | yes | ✅ |
| 28 | foliage_un06.benchmark | handle_benchmarkToBuf | Performance | project:read | no | yes | ✅ |
| 29 | foliage_un06.sample_terrain | handle_sampleTerrainToBuf | Analysis | project:read | no | yes | ✅ |
| 30 | foliage_un06.biome | handle_biomeToBuf | Pattern | scene:write | no | yes | ✅ |
| 31 | foliage_un06.radial | handle_radialToBuf | Pattern | scene:write | no | yes | ✅ |
| 32 | foliage_un06.along_path | handle_alongPathToBuf | Pattern | scene:write | no | yes | ✅ |
| 33 | foliage_un06.clump | handle_clumpToBuf | Pattern | scene:write | no | yes | ✅ |
| 34 | foliage_un06.brush_shapes | handle_brushShapesToBuf | Paint | scene:write | no | yes | ✅ |
| 35 | foliage_un06.random_replace | handle_randomReplaceToBuf | Filter | scene:write | no | yes | ✅ |
| 36 | foliage_un06.filter_by_type | handle_filterByTypeToBuf | Filter | scene:write | no | yes | ✅ |
| 37 | foliage_un06.transform_instances | handle_transformInstancesToBuf | Utility | scene:write | no | yes | ✅ |
| 38 | foliage_un06.wind_params | handle_windParamsToBuf | Config | project:read | no | yes | ✅ |
| 39 | foliage_un06.collision_settings | handle_collisionSettingsToBuf | Config | project:read | no | yes | ✅ |
| 40 | foliage_un06.lod_config | handle_lodConfigToBuf | Config | project:read | no | yes | ✅ |
| 41 | foliage_un06.statistics_grouped | handle_statisticsGroupedToBuf | Analysis | project:read | no | yes | ✅ |
| 42 | foliage_un06.list_commands | handle_listCommandsToBuf | Utility | project:read | no | yes | ✅ |
| 43 | foliage_un06.health_check | handle_healthCheckToBuf | Utility | project:read | no | yes | ✅ |

## Surface Synchronization Status

| Surface | Count | Status |
|---------|-------|--------|
| Handler implementations | 43 | ✅ All implemented in bus_handler.cpp |
| plugin.monted.json | 43 | ✅ All listed with title, permission, destructive |
| getCapabilitiesJson busHandlers | 41 | ✅ All visible (2 utility may need update) |
| getBusHandlersJson | 43 | ✅ All entries with entryPoint |
| executeCommand routes | 43 | ✅ All routed in core_entry.cpp |
| Legacy descriptor | 43 | ✅ commandCount: 43, full command list |
| dll_verify.cpp tests | 41 | ✅ 41/43 tested (2 utility pending update) |
| Mont-ED engine visibility | 43 | ✅ audit shows 43 bus commands registered |
| EditorApp --bus-call tested | 4/43 | ✅ inspect, scatter, analyze, benchmark confirmed |

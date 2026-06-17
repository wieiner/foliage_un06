# Architecture — foliage_un06 v0.5.0

## Overview

Foliage scatter plugin for Mont-ED. Replicates Unreal Engine's FoliageType → InstancedStaticMesh painting pipeline with 43 bus commands across 10 categories.

## Layer Diagram

```
┌─────────────────────────────────────────────────────────┐
│ Mont-ED Engine (NOT modified)                           │
│  Plugin Browser │ Plugin Tool Window │ PluginMeshResult │
│  EditorApp --bus-call │ Plugin Manager │ MessageBus     │
└──────────────────────┬──────────────────────────────────┘
                       │ C ABI (MontEdPluginApi V4)
┌──────────────────────┴──────────────────────────────────┐
│ src/core_abi/core_entry.cpp (Adapter)                   │
│  MontEd_GetPluginApiV4, getCapabilitiesJson,            │
│  getBusHandlersJson, executeCommand, getUiPanelsJson,   │
│  getSceneToolsJson, MontEd_GetBusHandlerDescriptor      │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────┴──────────────────────────────────┐
│ src/bus/bus_handler.cpp (43 handlers)                   │
│  handle_scatterToBuf, handle_paintToBuf, ...             │
│  handle_healthCheckToBuf, handle_listCommandsToBuf      │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────┴──────────────────────────────────┐
│ src/common/ (Shared Logic — 12 modules)                 │
│  foliage_type.h         — FoliageType + Instance         │
│  foliage_type_v2.h      — FoliageTypeV2 (UE-compatible) │
│  foliage_command_registry.h — Single source of truth    │
│  foliage_prng.h         — xoshiro256** PRNG             │
│  foliage_terrain.h      — TerrainSampler interface      │
│  foliage_scatter.h/cpp  — PoissonDiscScatter            │
│  foliage_paint.h/cpp    — Brush paint/erase             │
│  foliage_cluster.h/cpp  — Cluster mesh output           │
│  foliage_query.h/cpp    — Spatial/GIS index             │
│  foliage_simulate.h/cpp — Ecosystem simulation          │
│  foliage_analyze.h/cpp  — Statistical analysis          │
│  foliage_falloff.h/cpp  — Edge falloff                  │
│  foliage_noise.h/cpp    — Perlin noise modulation       │
│  foliage_mask.h/cpp     — Layer masks + exclusion zones │
│  foliage_pattern.h/cpp  — Radial/path/clump patterns    │
│  foliage_biome.h/cpp    — Biome zone system             │
│  foliage_json.h/cpp     — Structured JSON parser        │
└─────────────────────────────────────────────────────────┘
```

## Data Flow (Scatter example)

```
1. Engine PluginManager loads foliage_un06.dll
2. NativePluginLoader calls MontEd_GetPluginApiV4
3. Engine registers bus handlers from getBusHandlersJson
4. Engine registers manifest commands from plugin.monted.json
5. User dispatches: EditorApp --bus-call foliage_un06.scatter --payload '{...}'
6. MessageBus::Dispatch → InvokeBusHandler → handle_scatterToBuf
7. handler parses JSON → builds FoliageType + ScatterArea
8. PoissonDiscScatter::Scatter(terrain, type, area) → vector<FoliageInstance>
9. ClusterBuilder::BuildCombined(instances, meshTemplate) → mesh data
10. JSON serialized with instances + mesh summary → returned to engine
11. PluginMeshResult renders bounding box in viewport
```

## Database Diagram (State)

```
g_types: map<name, FoliageType>           [Type Registry, mutex-protected]
g_instanceStores: map<storeKey, [Instance]> [Instance Stores, mutex]
g_checkpoints: map<storeKey, [JSON]>      [Undo/Redo, mutex]
g_exclusionZones: ExclusionZoneSet        [Zone registry, mutex]
g_splines: [SplinePath]                  [Spline registry, mutex]
g_presets: map<name, FoliagePreset>      [Named presets, mutex]
g_benchStats: BenchmarkStats             [Performance stats, mutex]
```

## ABI Contract

- **V4** (`MontEd_GetPluginApiV4`): Primary. Returns struct with identity + function pointers.
- **V3** (`MontEd_GetPluginApi`): Legacy fallback. Returns v3 struct.
- **Descriptor** (`MontEd_GetBusHandlerDescriptor`): Returns JSON with full command list + version.
- **No STL/exceptions across boundary**: All C ABI exports, explicit ownership, opaque handles.

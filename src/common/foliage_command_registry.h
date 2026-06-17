// foliage_un06 — command registry: single source of truth.
// Every command MUST have an entry here. This header is consumed by:
//   - core_entry.cpp  (getCapabilitiesJson, getBusHandlersJson, executeCommand)
//   - bus_handler.cpp (MontEd_GetBusHandlerDescriptor)
//   - test/verify tools
//   - plugin.monted.json generator
//   - standalone GUI
//   - docs generator
//
// To add a command: append a REGISTRY_ENTRY line, then sync all consumers.
#pragma once

#include <cstdint>

namespace foliage {

// ---------------------------------------------------------------------------
// Permission enum — matches Mont-ED MessageBus permission strings
// ---------------------------------------------------------------------------
namespace Permission {
    inline constexpr const char* kProjectRead      = "project:read";
    inline constexpr const char* kProjectWrite     = "project:write";
    inline constexpr const char* kAssetRead        = "asset:read";
    inline constexpr const char* kAssetWrite       = "asset:write";
    inline constexpr const char* kSceneRead        = "scene:read";
    inline constexpr const char* kSceneWrite       = "scene:write";
    inline constexpr const char* kFilesystemRead   = "filesystem:read";
    inline constexpr const char* kFilesystemWrite  = "filesystem:write";
}

// ---------------------------------------------------------------------------
// Category
// ---------------------------------------------------------------------------
namespace Category {
    inline constexpr const char* kCore          = "core";
    inline constexpr const char* kScatter       = "scatter";
    inline constexpr const char* kPaint         = "paint";
    inline constexpr const char* kFilter        = "filter";
    inline constexpr const char* kAnalysis      = "analysis";
    inline constexpr const char* kPersistence   = "persistence";
    inline constexpr const char* kPattern       = "pattern";
    inline constexpr const char* kConfiguration = "configuration";
    inline constexpr const char* kPerformance   = "performance";
    inline constexpr const char* kUtility       = "utility";
}

// ---------------------------------------------------------------------------
// One command entry
// ---------------------------------------------------------------------------
struct CommandEntry {
    const char* commandId;          // e.g. "foliage_un06.scatter"
    const char* shortName;          // e.g. "scatter"
    const char* handlerExport;      // e.g. "handle_scatterToBuf"
    const char* title;
    const char* description;
    const char* category;
    const char* permission;
    bool        destructive;
    bool        defaultDryRun;
    bool        requiresTransaction;
    bool        implemented;       // true if handler exists and works
    bool        visibleInManifest; // should appear in plugin.monted.json
    bool        visibleInGui;      // should appear in standalone GUI
    bool        visibleAsSceneTool;
    const char* inputSchema;       // relative path or nullptr
    const char* outputSchema;      // relative path or nullptr
};

// ---------------------------------------------------------------------------
// Registry — all commands in declaration order
// ---------------------------------------------------------------------------
// clang-format off
inline constexpr CommandEntry kCommandRegistry[] = {
    // ---- Phase 1: Core (7) ----
    {"foliage_un06.inspect",          "inspect",           "handle_inspectToBuf",           "Inspect",            "Plugin identity, capabilities and feature list.",                    Category::kCore,          Permission::kProjectRead,    false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.scatter",          "scatter",           "handle_scatterToBuf",           "Scatter",            "Deterministic Poisson-disc scatter with slope/height filters.",      Category::kScatter,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  true,  nullptr, nullptr},
    {"foliage_un06.paint",            "paint",             "handle_paintToBuf",             "Paint Brush",        "Brush paint: Poisson-disc inside brush circle.",                     Category::kPaint,         Permission::kSceneWrite,     false, true,  false, true,  true,  true,  true,  nullptr, nullptr},
    {"foliage_un06.erase",            "erase",             "handle_eraseToBuf",             "Erase Brush",        "Brush erase: remove instances within radius.",                       Category::kPaint,         Permission::kSceneWrite,     true,  true,  false, true,  true,  true,  true,  nullptr, nullptr},
    {"foliage_un06.get_types",        "get_types",         "handle_getTypesToBuf",          "List Types",         "List all registered FoliageType definitions.",                       Category::kCore,          Permission::kProjectRead,    false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.add_type",         "add_type",          "handle_addTypeToBuf",           "Add Type",           "Create or update a FoliageType from JSON parameters.",               Category::kConfiguration, Permission::kAssetRead,      false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.generate_mesh",    "generate_mesh",     "handle_generateMeshToBuf",      "Generate Mesh",      "Scatter and generate per-cluster vertex/index data.",                Category::kScatter,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},

    // ---- Phase 2: Extended (10) ----
    {"foliage_un06.reapply",          "reapply",           "handle_reapplyToBuf",           "Reapply Filters",    "Re-check height/slope/align filters on existing instances.",          Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.fill",             "fill",              "handle_fillToBuf",              "Fill Area",          "Multi-pass auto-fill area with convergence.",                        Category::kScatter,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.simulate",         "simulate",          "handle_simulateToBuf",          "Ecosystem Sim",      "Age instances, spread seeds, run competition.",                      Category::kScatter,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.query",            "query",             "handle_queryToBuf",             "Spatial Query",      "Grid-hash nearest/radius/count lookup.",                             Category::kAnalysis,      Permission::kProjectRead,    false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.place_single",     "place_single",      "handle_placeSingleToBuf",       "Place Single",       "Place one instance at exact world position.",                        Category::kPaint,         Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.remove_instances", "remove_instances",  "handle_removeInstancesToBuf",   "Remove Instances",   "Remove by index/radius/height/slope filter.",                        Category::kFilter,        Permission::kSceneWrite,     true,  true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.analyze",          "analyze",           "handle_analyzeToBuf",           "Analyze Store",      "Statistical analysis: NN, Clark-Evans, height hist, density grid.",   Category::kAnalysis,      Permission::kProjectRead,    false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.export_state",     "export_state",      "handle_exportStateToBuf",       "Export State",       "Export FoliageTypes + instances to JSON file.",                      Category::kPersistence,   Permission::kFilesystemWrite,false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.import_state",     "import_state",      "handle_importStateToBuf",       "Import State",       "Import plugin state from JSON file.",                                Category::kPersistence,   Permission::kFilesystemRead, false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.density_falloff",  "density_falloff",   "handle_densityFalloffToBuf",    "Density Falloff",    "Edge feathering: linear, smoothstep, sqrt, inverse curves.",         Category::kFilter,        Permission::kSceneWrite,     true,  true,  false, true,  true,  true,  false, nullptr, nullptr},

    // ---- Phase 3: Masking / Noise / Utility (12) ----
    {"foliage_un06.layer_mask",       "layer_mask",        "handle_layerMaskToBuf",         "Layer Mask",         "Filter instances by inclusion/exclusion layers.",                    Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.noise_modulate",   "noise_modulate",    "handle_noiseModulateToBuf",     "Noise Modulate",     "Perlin/FBM/ridged noise for natural density variation.",             Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.z_offset",         "z_offset",          "handle_zOffsetToBuf",           "Z Offset",           "Random vertical offset range per FoliageType.",                      Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.cull_distance",    "cull_distance",     "handle_cullDistanceToBuf",      "Cull Distance",      "Set/get start/end cull distance per FoliageType.",                   Category::kConfiguration,  Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.batch",            "batch",             "handle_batchToBuf",             "Batch Execute",      "Execute multiple sub-commands in one call.",                         Category::kUtility,       Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.checkpoint",       "checkpoint",        "handle_checkpointToBuf",        "Checkpoint",         "Save/restore/clear snapshot checkpoints for undo.",                  Category::kPersistence,   Permission::kSceneWrite,     true,  true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.merge",            "merge",             "handle_mergeToBuf",             "Merge Stores",       "Combine multiple instance stores into one.",                         Category::kPersistence,   Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.mirror_region",    "mirror_region",     "handle_mirrorRegionToBuf",      "Mirror Region",      "Copy/move instances between regions with mirror/rotate.",            Category::kPattern,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.exclusion_zone",   "exclusion_zone",    "handle_exclusionZoneToBuf",     "Exclusion Zone",     "Add/remove/query circle/rect blocking volumes.",                     Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.preset",           "preset",            "handle_presetToBuf",            "Presets",            "Save/load/delete named scatter presets.",                            Category::kConfiguration,  Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.benchmark",        "benchmark",         "handle_benchmarkToBuf",         "Benchmark",          "Measure scatter performance: runs, ms, instances/s.",                Category::kPerformance,    Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.sample_terrain",   "sample_terrain",    "handle_sampleTerrainToBuf",     "Sample Terrain",     "Bulk sample terrain height/slope/normal at grid points.",            Category::kAnalysis,      Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},

    // ---- Phase 4: Patterns / Biomes / Config (12) ----
    {"foliage_un06.biome",            "biome",             "handle_biomeToBuf",             "Biome Scatter",      "Multi-zone height/slope-based type assignment with blending.",       Category::kPattern,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.radial",           "radial",            "handle_radialToBuf",            "Radial Scatter",     "Concentric rings/full disc placement.",                              Category::kPattern,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.along_path",       "along_path",        "handle_alongPathToBuf",         "Along Path",         "Scatter instances along polyline/spline with width.",                Category::kPattern,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.clump",            "clump",             "handle_clumpToBuf",             "Natural Clumps",     "Thomas cluster process: parents + Gaussian children.",               Category::kPattern,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.brush_shapes",     "brush_shapes",      "handle_brushShapesToBuf",       "Brush Shapes",       "Paint with circle/square/donut/line/gaussian brushes.",             Category::kPaint,         Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.random_replace",   "random_replace",    "handle_randomReplaceToBuf",     "Random Replace",     "Replace random % of instances with alternative types.",              Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.filter_by_type",   "filter_by_type",    "handle_filterByTypeToBuf",      "Filter By Type",     "Keep/remove instances by scale range (proxy for type).",             Category::kFilter,        Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.transform_instances","transform_instances","handle_transformInstancesToBuf","Transform Instances","Bulk translate/rotate/scale on instances.",                           Category::kUtility,       Permission::kSceneWrite,     false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.wind_params",      "wind_params",       "handle_windParamsToBuf",        "Wind Parameters",    "Set/get wind strength/sway/flutter/gust per FoliageType.",          Category::kConfiguration, Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.collision_settings","collision_settings","handle_collisionSettingsToBuf", "Collision Settings", "Set/get collision preset per FoliageType.",                          Category::kConfiguration, Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.lod_config",       "lod_config",        "handle_lodConfigToBuf",         "LOD Config",         "Set/get LOD distances and impostor settings per FoliageType.",      Category::kConfiguration, Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.statistics_grouped","statistics_grouped","handle_statisticsGroupedToBuf", "Grouped Statistics", "Stats grouped by height bands and slope bands with avg scale.",      Category::kAnalysis,      Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},

    // ---- Phase 5 (hardening): Utility commands (2) ----
    {"foliage_un06.list_commands",     "list_commands",      "handle_listCommandsToBuf",      "List Commands",      "Return canonical command registry with all plugin commands.",          Category::kUtility,       Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
    {"foliage_un06.health_check",      "health_check",       "handle_healthCheckToBuf",       "Health Check",       "Deep self-test: schemas, registry, handlers, stores, limits.",        Category::kUtility,       Permission::kProjectRead,   false, true,  false, true,  true,  true,  false, nullptr, nullptr},
};

inline constexpr int kCommandCount = sizeof(kCommandRegistry) / sizeof(kCommandRegistry[0]);
static_assert(kCommandCount == 43, "Command registry must have exactly 43 entries");

// ---------------------------------------------------------------------------
// Convenience helpers
// ---------------------------------------------------------------------------
inline int CommandCount() { return kCommandCount; }
inline const CommandEntry* FindCommand(const char* commandId) {
    for (int i = 0; i < kCommandCount; ++i) {
        if (std::strcmp(kCommandRegistry[i].commandId, commandId) == 0)
            return &kCommandRegistry[i];
    }
    return nullptr;
}

} // namespace foliage

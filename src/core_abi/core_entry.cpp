// foliage_un06 — Core ABI adapter (Mont-ED plugin API V4).
// Exposes plugin identity, capabilities, bus handlers, UI panels, scene tools.
//
// The shared foliage logic lives in src/common/; this file is the thin
// C-ABI wrapper that the engine's NativePluginLoader calls.
//
// Upgrade from Phase 0 skeleton (V3) to full V4 with all surface exports.

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#define MONTED_API __declspec(dllexport)
#define MONTED_CALL __cdecl
#else
#define MONTED_API
#define MONTED_CALL
#endif

// Minimal SDK types (self-contained; we don't include the full SDK to keep
// the build hermetic — the engine's NativePluginLoader CPI is the authority).
using MontEdStatus = std::uint32_t;
#define MONTED_STATUS_OK               0u
#define MONTED_STATUS_INVALID_ARGUMENT 1u
#define MONTED_STATUS_NOT_IMPLEMENTED  2u
#define MONTED_STATUS_INTERNAL_ERROR   8u
#define MONTED_ABI_VERSION_CURRENT     4u

// ---- externs from bus_handler.cpp (same DLL) --------------------------
extern "C" {
    // Legacy descriptor (called by NativePluginLoader)
    MONTED_API const char* MONTED_CALL MontEd_GetBusHandlerDescriptor();

    // Bus handler entry points (17 total)
    int handle_scatterToBuf(const char* in, char* out, size_t n);
    int handle_paintToBuf(const char* in, char* out, size_t n);
    int handle_eraseToBuf(const char* in, char* out, size_t n);
    int handle_getTypesToBuf(const char* in, char* out, size_t n);
    int handle_addTypeToBuf(const char* in, char* out, size_t n);
    int handle_inspectToBuf(const char* in, char* out, size_t n);
    int handle_generateMeshToBuf(const char* in, char* out, size_t n);
    int handle_reapplyToBuf(const char* in, char* out, size_t n);
    int handle_fillToBuf(const char* in, char* out, size_t n);
    int handle_simulateToBuf(const char* in, char* out, size_t n);
    int handle_queryToBuf(const char* in, char* out, size_t n);
    int handle_placeSingleToBuf(const char* in, char* out, size_t n);
    int handle_removeInstancesToBuf(const char* in, char* out, size_t n);
    int handle_analyzeToBuf(const char* in, char* out, size_t n);
    int handle_exportStateToBuf(const char* in, char* out, size_t n);
    int handle_importStateToBuf(const char* in, char* out, size_t n);
    int handle_densityFalloffToBuf(const char* in, char* out, size_t n);
    int handle_layerMaskToBuf(const char* in, char* out, size_t n);
    int handle_noiseModulateToBuf(const char* in, char* out, size_t n);
    int handle_zOffsetToBuf(const char* in, char* out, size_t n);
    int handle_cullDistanceToBuf(const char* in, char* out, size_t n);
    int handle_batchToBuf(const char* in, char* out, size_t n);
    int handle_checkpointToBuf(const char* in, char* out, size_t n);
    int handle_mergeToBuf(const char* in, char* out, size_t n);
    int handle_mirrorRegionToBuf(const char* in, char* out, size_t n);
    int handle_exclusionZoneToBuf(const char* in, char* out, size_t n);
    int handle_presetToBuf(const char* in, char* out, size_t n);
    int handle_benchmarkToBuf(const char* in, char* out, size_t n);
    int handle_sampleTerrainToBuf(const char* in, char* out, size_t n);
    int handle_biomeToBuf(const char* in, char* out, size_t n);
    int handle_radialToBuf(const char* in, char* out, size_t n);
    int handle_alongPathToBuf(const char* in, char* out, size_t n);
    int handle_clumpToBuf(const char* in, char* out, size_t n);
    int handle_brushShapesToBuf(const char* in, char* out, size_t n);
    int handle_randomReplaceToBuf(const char* in, char* out, size_t n);
    int handle_filterByTypeToBuf(const char* in, char* out, size_t n);
    int handle_transformInstancesToBuf(const char* in, char* out, size_t n);
    int handle_windParamsToBuf(const char* in, char* out, size_t n);
    int handle_collisionSettingsToBuf(const char* in, char* out, size_t n);
    int handle_lodConfigToBuf(const char* in, char* out, size_t n);
    int handle_statisticsGroupedToBuf(const char* in, char* out, size_t n);
    int handle_listCommandsToBuf(const char* in, char* out, size_t n);
    int handle_healthCheckToBuf(const char* in, char* out, size_t n);
    int handle_scatterMultiTypeToBuf(const char* in, char* out, size_t n);
    int handle_selectInstancesToBuf(const char* in, char* out, size_t n);
    int handle_clearStoreToBuf(const char* in, char* out, size_t n);
    int handle_duplicateStoreToBuf(const char* in, char* out, size_t n);
    int handle_compactStoreToBuf(const char* in, char* out, size_t n);
    int handle_reseedToBuf(const char* in, char* out, size_t n);
    int handle_validatePayloadToBuf(const char* in, char* out, size_t n);
    int handle_diffStoresToBuf(const char* in, char* out, size_t n);
    int handle_exportEnginePayloadToBuf(const char* in, char* out, size_t n);
    int handle_densityHeatmapToBuf(const char* in, char* out, size_t n);
    int handle_computeBoundsToBuf(const char* in, char* out, size_t n);
    int handle_autoLODToBuf(const char* in, char* out, size_t n);
    int handle_groupByClusterToBuf(const char* in, char* out, size_t n);
    int handle_jitterPositionsToBuf(const char* in, char* out, size_t n);
    int handle_exportCSVToBuf(const char* in, char* out, size_t n);
    int handle_snapToTerrainToBuf(const char* in, char* out, size_t n);
    int handle_removeOutliersToBuf(const char* in, char* out, size_t n);
    int handle_coverageAnalysisToBuf(const char* in, char* out, size_t n);
    int handle_pipelineToBuf(const char* in, char* out, size_t n);
    int handle_storeMemoryToBuf(const char* in, char* out, size_t n);
}

// ---- V4 Plugin API ----------------------------------------------------

struct MontEdPluginIdentity {
    std::uint32_t structSize;
    std::uint32_t abiVersion;
    const char* pluginId;
    const char* pluginName;
    const char* pluginVersion;
    const char* pluginType;
    const char* protocolPreset;
    std::uint64_t reserved0;
    std::uint64_t reserved1;
    std::uint64_t reserved2;
    std::uint64_t reserved3;
};

using MontEdPluginQueryJsonFn = std::uint32_t (MONTED_CALL*)(char*, std::uint32_t, std::uint32_t*);
using MontEdPluginExecuteCommandFn = std::uint32_t (MONTED_CALL*)(const char*, const char*, char*, std::uint32_t, std::uint32_t*);

struct MontEdPluginApi {
    std::uint32_t structSize;
    std::uint32_t abiVersion;
    MontEdPluginIdentity identity;
    void* initialize;        // not used
    void* shutdown;          // not used
    void* getIdentity;       // not used (identity field is inline)
    MontEdPluginQueryJsonFn getCapabilitiesJson;
    MontEdPluginQueryJsonFn getBusHandlersJson;
    MontEdPluginQueryJsonFn getUiPanelsJson;
    MontEdPluginQueryJsonFn getSceneToolsJson;
    MontEdPluginQueryJsonFn getBlueprintNodesJson;
    MontEdPluginExecuteCommandFn executeCommand;
    std::uint64_t reserved2;
    std::uint64_t reserved3;
};

// ---- JSON helper -------------------------------------------------------

namespace {

std::uint32_t copyJson(const char* json, char* out, std::uint32_t outSize, std::uint32_t* required) {
    std::uint32_t len = json ? static_cast<std::uint32_t>(std::strlen(json) + 1) : 0u;
    if (required) *required = len;
    if (!out || outSize == 0) return MONTED_STATUS_INVALID_ARGUMENT;
    if (outSize < len) return MONTED_STATUS_INTERNAL_ERROR;
    std::memcpy(out, json, len);
    return MONTED_STATUS_OK;
}

} // namespace

// ---- QueryJson callbacks ----------------------------------------------

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_getCapabilitiesJson(char* out, std::uint32_t outSize, std::uint32_t* required) {
    return copyJson(
        "{\"pluginId\":\"com.monted.plugins.foliage_un06\","
        "\"version\":\"0.5.0\","
        "\"abiVersion\":4,"
        "\"protocolPreset\":\"core-bus\","
        "\"capabilities\":["
        "\"identity\",\"bus\",\"ui-panel\",\"scene-tool\",\"execute-command\","
        "\"deterministic-scatter\",\"brush-paint\",\"brush-erase\","
        "\"procedural-simulation\",\"spatial-query\",\"state-persistence\","
        "\"masks\",\"biomes\",\"patterns\",\"analysis\","
        "\"engine-preview\",\"standalone-gui\",\"command-registry\","
        "\"dry-run\",\"undo-redo\""
        "],"
        "\"busHandlers\":["
        "\"foliage_un06.inspect\",\"foliage_un06.scatter\",\"foliage_un06.paint\","
        "\"foliage_un06.erase\",\"foliage_un06.get_types\",\"foliage_un06.add_type\","
        "\"foliage_un06.generate_mesh\",\"foliage_un06.reapply\",\"foliage_un06.fill\","
        "\"foliage_un06.simulate\",\"foliage_un06.query\",\"foliage_un06.place_single\","
        "\"foliage_un06.remove_instances\",\"foliage_un06.analyze\","
        "\"foliage_un06.export_state\",\"foliage_un06.import_state\","
        "\"foliage_un06.density_falloff\",\"foliage_un06.layer_mask\","
        "\"foliage_un06.noise_modulate\",\"foliage_un06.z_offset\","
        "\"foliage_un06.cull_distance\",\"foliage_un06.batch\","
        "\"foliage_un06.checkpoint\",\"foliage_un06.merge\","
        "\"foliage_un06.mirror_region\",\"foliage_un06.exclusion_zone\","
        "\"foliage_un06.preset\",\"foliage_un06.benchmark\","
        "\"foliage_un06.sample_terrain\",\"foliage_un06.biome\","
        "\"foliage_un06.radial\",\"foliage_un06.along_path\","
        "\"foliage_un06.clump\",\"foliage_un06.brush_shapes\","
        "\"foliage_un06.random_replace\",\"foliage_un06.filter_by_type\","
        "\"foliage_un06.transform_instances\",\"foliage_un06.wind_params\","
        "\"foliage_un06.collision_settings\",\"foliage_un06.lod_config\","
        "\"foliage_un06.statistics_grouped\""
        "],"
        "\"uiPanels\":[\"foliage.scatterPanel\",\"foliage.analyzePanel\",\"foliage.biomePanel\"],"
        "\"sceneTools\":["
        "\"foliage.scatterTool\",\"foliage.paintTool\",\"foliage.fillTool\","
        "\"foliage.analyzeTool\",\"foliage.falloffTool\",\"foliage.biomeTool\""
        "]}",
        out, outSize, required);
}

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_getBusHandlersJson(char* out, std::uint32_t outSize, std::uint32_t* required) {
    return copyJson(
        "{\"handlers\":["
        "{\"commandId\":\"foliage_un06.scatter\",\"entryPoint\":\"handle_scatterToBuf\"},"
        "{\"commandId\":\"foliage_un06.paint\",\"entryPoint\":\"handle_paintToBuf\"},"
        "{\"commandId\":\"foliage_un06.erase\",\"entryPoint\":\"handle_eraseToBuf\"},"
        "{\"commandId\":\"foliage_un06.get_types\",\"entryPoint\":\"handle_getTypesToBuf\"},"
        "{\"commandId\":\"foliage_un06.add_type\",\"entryPoint\":\"handle_addTypeToBuf\"},"
        "{\"commandId\":\"foliage_un06.generate_mesh\",\"entryPoint\":\"handle_generateMeshToBuf\"},"
        "{\"commandId\":\"foliage_un06.reapply\",\"entryPoint\":\"handle_reapplyToBuf\"},"
        "{\"commandId\":\"foliage_un06.fill\",\"entryPoint\":\"handle_fillToBuf\"},"
        "{\"commandId\":\"foliage_un06.simulate\",\"entryPoint\":\"handle_simulateToBuf\"},"
        "{\"commandId\":\"foliage_un06.query\",\"entryPoint\":\"handle_queryToBuf\"},"
        "{\"commandId\":\"foliage_un06.place_single\",\"entryPoint\":\"handle_placeSingleToBuf\"},"
        "{\"commandId\":\"foliage_un06.remove_instances\",\"entryPoint\":\"handle_removeInstancesToBuf\"},"
        "{\"commandId\":\"foliage_un06.analyze\",\"entryPoint\":\"handle_analyzeToBuf\"},"
        "{\"commandId\":\"foliage_un06.export_state\",\"entryPoint\":\"handle_exportStateToBuf\"},"
        "{\"commandId\":\"foliage_un06.import_state\",\"entryPoint\":\"handle_importStateToBuf\"},"
        "{\"commandId\":\"foliage_un06.density_falloff\",\"entryPoint\":\"handle_densityFalloffToBuf\"},"
        "{\"commandId\":\"foliage_un06.layer_mask\",\"entryPoint\":\"handle_layerMaskToBuf\"},"
        "{\"commandId\":\"foliage_un06.noise_modulate\",\"entryPoint\":\"handle_noiseModulateToBuf\"},"
        "{\"commandId\":\"foliage_un06.z_offset\",\"entryPoint\":\"handle_zOffsetToBuf\"},"
        "{\"commandId\":\"foliage_un06.cull_distance\",\"entryPoint\":\"handle_cullDistanceToBuf\"},"
        "{\"commandId\":\"foliage_un06.batch\",\"entryPoint\":\"handle_batchToBuf\"},"
        "{\"commandId\":\"foliage_un06.checkpoint\",\"entryPoint\":\"handle_checkpointToBuf\"},"
        "{\"commandId\":\"foliage_un06.merge\",\"entryPoint\":\"handle_mergeToBuf\"},"
        "{\"commandId\":\"foliage_un06.mirror_region\",\"entryPoint\":\"handle_mirrorRegionToBuf\"},"
        "{\"commandId\":\"foliage_un06.exclusion_zone\",\"entryPoint\":\"handle_exclusionZoneToBuf\"},"
        "{\"commandId\":\"foliage_un06.preset\",\"entryPoint\":\"handle_presetToBuf\"},"
        "{\"commandId\":\"foliage_un06.benchmark\",\"entryPoint\":\"handle_benchmarkToBuf\"},"
        "{\"commandId\":\"foliage_un06.sample_terrain\",\"entryPoint\":\"handle_sampleTerrainToBuf\"},"
        "{\"commandId\":\"foliage_un06.biome\",\"entryPoint\":\"handle_biomeToBuf\"},"
        "{\"commandId\":\"foliage_un06.radial\",\"entryPoint\":\"handle_radialToBuf\"},"
        "{\"commandId\":\"foliage_un06.along_path\",\"entryPoint\":\"handle_alongPathToBuf\"},"
        "{\"commandId\":\"foliage_un06.clump\",\"entryPoint\":\"handle_clumpToBuf\"},"
        "{\"commandId\":\"foliage_un06.brush_shapes\",\"entryPoint\":\"handle_brushShapesToBuf\"},"
        "{\"commandId\":\"foliage_un06.random_replace\",\"entryPoint\":\"handle_randomReplaceToBuf\"},"
        "{\"commandId\":\"foliage_un06.filter_by_type\",\"entryPoint\":\"handle_filterByTypeToBuf\"},"
        "{\"commandId\":\"foliage_un06.transform_instances\",\"entryPoint\":\"handle_transformInstancesToBuf\"},"
        "{\"commandId\":\"foliage_un06.wind_params\",\"entryPoint\":\"handle_windParamsToBuf\"},"
        "{\"commandId\":\"foliage_un06.collision_settings\",\"entryPoint\":\"handle_collisionSettingsToBuf\"},"
        "{\"commandId\":\"foliage_un06.lod_config\",\"entryPoint\":\"handle_lodConfigToBuf\"},"
        "{\"commandId\":\"foliage_un06.statistics_grouped\",\"entryPoint\":\"handle_statisticsGroupedToBuf\"},"
        "{\"commandId\":\"foliage_un06.list_commands\",\"entryPoint\":\"handle_listCommandsToBuf\"},"
        "{\"commandId\":\"foliage_un06.health_check\",\"entryPoint\":\"handle_healthCheckToBuf\"},"
        "{\"commandId\":\"foliage_un06.scatter_multi_type\",\"entryPoint\":\"handle_scatterMultiTypeToBuf\"},"
        "{\"commandId\":\"foliage_un06.select_instances\",\"entryPoint\":\"handle_selectInstancesToBuf\"},"
        "{\"commandId\":\"foliage_un06.clear_store\",\"entryPoint\":\"handle_clearStoreToBuf\"},"
        "{\"commandId\":\"foliage_un06.duplicate_store\",\"entryPoint\":\"handle_duplicateStoreToBuf\"},"
        "{\"commandId\":\"foliage_un06.compact_store\",\"entryPoint\":\"handle_compactStoreToBuf\"},"
        "{\"commandId\":\"foliage_un06.reseed\",\"entryPoint\":\"handle_reseedToBuf\"},"
        "{\"commandId\":\"foliage_un06.validate_payload\",\"entryPoint\":\"handle_validatePayloadToBuf\"},"
        "{\"commandId\":\"foliage_un06.diff_stores\",\"entryPoint\":\"handle_diffStoresToBuf\"},"
        "{\"commandId\":\"foliage_un06.export_engine_payload\",\"entryPoint\":\"handle_exportEnginePayloadToBuf\"},"
        "{\"commandId\":\"foliage_un06.density_heatmap\",\"entryPoint\":\"handle_densityHeatmapToBuf\"},"
        "{\"commandId\":\"foliage_un06.compute_bounds\",\"entryPoint\":\"handle_computeBoundsToBuf\"},"
        "{\"commandId\":\"foliage_un06.auto_lod\",\"entryPoint\":\"handle_autoLODToBuf\"},"
        "{\"commandId\":\"foliage_un06.group_by_cluster\",\"entryPoint\":\"handle_groupByClusterToBuf\"},"
        "{\"commandId\":\"foliage_un06.jitter_positions\",\"entryPoint\":\"handle_jitterPositionsToBuf\"},"
        "{\"commandId\":\"foliage_un06.export_instances_csv\",\"entryPoint\":\"handle_exportCSVToBuf\"},"
        "{\"commandId\":\"foliage_un06.snap_to_terrain\",\"entryPoint\":\"handle_snapToTerrainToBuf\"},"
        "{\"commandId\":\"foliage_un06.remove_outliers\",\"entryPoint\":\"handle_removeOutliersToBuf\"},"
        "{\"commandId\":\"foliage_un06.coverage_analysis\",\"entryPoint\":\"handle_coverageAnalysisToBuf\"},"
        "{\"commandId\":\"foliage_un06.pipeline\",\"entryPoint\":\"handle_pipelineToBuf\"},"
        "{\"commandId\":\"foliage_un06.store_memory\",\"entryPoint\":\"handle_storeMemoryToBuf\"}"
        "]}",
        out, outSize, required);
}

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_getUiPanelsJson(char* out, std::uint32_t outSize, std::uint32_t* required) {
    return copyJson(
        "{\"panels\":["
        "{\"id\":\"foliage.scatterPanel\",\"title\":\"Foliage Scatter\","
        "\"category\":\"Foliage\",\"commandId\":\"foliage_un06.scatter\"}"
        "]}",
        out, outSize, required);
}

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_getSceneToolsJson(char* out, std::uint32_t outSize, std::uint32_t* required) {
    return copyJson(
        "{\"sceneTools\":["
        "{\"id\":\"foliage.scatterTool\",\"title\":\"Foliage Scatter\","
        "\"category\":\"Foliage\",\"commandId\":\"foliage_un06.scatter\","
        "\"defaultPayloadJson\":\"{\\\"areaWidth\\\":200,\\\"areaDepth\\\":200,\\\"density\\\":0.03,\\\"seed\\\":42}\"},"
        "{\"id\":\"foliage.paintTool\",\"title\":\"Foliage Paint\","
        "\"category\":\"Foliage\",\"commandId\":\"foliage_un06.paint\","
        "\"defaultPayloadJson\":\"{\\\"centerX\\\":0,\\\"centerZ\\\":0,\\\"radius\\\":20,\\\"densityMul\\\":1.5}\"}"
        "]}",
        out, outSize, required);
}

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_getBlueprintNodesJson(char* out, std::uint32_t outSize, std::uint32_t* required) {
    return copyJson("{\"nodes\":[]}", out, outSize, required);
}

// ---- executeCommand ---------------------------------------------------

extern "C" MONTED_API MontEdStatus MONTED_CALL
foliage_executeCommand(const char* commandId,
                       const char* payloadJson,
                       char* outJson,
                       std::uint32_t outJsonSize,
                       std::uint32_t* outRequiredSize)
{
    if (!commandId || !outJson || outJsonSize == 0) {
        return MONTED_STATUS_INVALID_ARGUMENT;
    }

    int (*handler)(const char*, char*, size_t) = nullptr;

    if (std::strcmp(commandId, "foliage_un06.scatter") == 0)
        handler = handle_scatterToBuf;
    else if (std::strcmp(commandId, "foliage_un06.paint") == 0)
        handler = handle_paintToBuf;
    else if (std::strcmp(commandId, "foliage_un06.erase") == 0)
        handler = handle_eraseToBuf;
    else if (std::strcmp(commandId, "foliage_un06.get_types") == 0)
        handler = handle_getTypesToBuf;
    else if (std::strcmp(commandId, "foliage_un06.add_type") == 0)
        handler = handle_addTypeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.generate_mesh") == 0)
        handler = handle_generateMeshToBuf;
    else if (std::strcmp(commandId, "foliage_un06.inspect") == 0)
        handler = handle_inspectToBuf;
    else if (std::strcmp(commandId, "foliage_un06.reapply") == 0)
        handler = handle_reapplyToBuf;
    else if (std::strcmp(commandId, "foliage_un06.fill") == 0)
        handler = handle_fillToBuf;
    else if (std::strcmp(commandId, "foliage_un06.simulate") == 0)
        handler = handle_simulateToBuf;
    else if (std::strcmp(commandId, "foliage_un06.query") == 0)
        handler = handle_queryToBuf;
    else if (std::strcmp(commandId, "foliage_un06.place_single") == 0)
        handler = handle_placeSingleToBuf;
    else if (std::strcmp(commandId, "foliage_un06.remove_instances") == 0)
        handler = handle_removeInstancesToBuf;
    else if (std::strcmp(commandId, "foliage_un06.analyze") == 0)
        handler = handle_analyzeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.export_state") == 0)
        handler = handle_exportStateToBuf;
    else if (std::strcmp(commandId, "foliage_un06.import_state") == 0)
        handler = handle_importStateToBuf;
    else if (std::strcmp(commandId, "foliage_un06.density_falloff") == 0)
        handler = handle_densityFalloffToBuf;
    else if (std::strcmp(commandId, "foliage_un06.layer_mask") == 0)
        handler = handle_layerMaskToBuf;
    else if (std::strcmp(commandId, "foliage_un06.noise_modulate") == 0)
        handler = handle_noiseModulateToBuf;
    else if (std::strcmp(commandId, "foliage_un06.z_offset") == 0)
        handler = handle_zOffsetToBuf;
    else if (std::strcmp(commandId, "foliage_un06.cull_distance") == 0)
        handler = handle_cullDistanceToBuf;
    else if (std::strcmp(commandId, "foliage_un06.batch") == 0)
        handler = handle_batchToBuf;
    else if (std::strcmp(commandId, "foliage_un06.checkpoint") == 0)
        handler = handle_checkpointToBuf;
    else if (std::strcmp(commandId, "foliage_un06.merge") == 0)
        handler = handle_mergeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.mirror_region") == 0)
        handler = handle_mirrorRegionToBuf;
    else if (std::strcmp(commandId, "foliage_un06.exclusion_zone") == 0)
        handler = handle_exclusionZoneToBuf;
    else if (std::strcmp(commandId, "foliage_un06.preset") == 0)
        handler = handle_presetToBuf;
    else if (std::strcmp(commandId, "foliage_un06.benchmark") == 0)
        handler = handle_benchmarkToBuf;
    else if (std::strcmp(commandId, "foliage_un06.sample_terrain") == 0)
        handler = handle_sampleTerrainToBuf;
    else if (std::strcmp(commandId, "foliage_un06.biome") == 0)
        handler = handle_biomeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.radial") == 0)
        handler = handle_radialToBuf;
    else if (std::strcmp(commandId, "foliage_un06.along_path") == 0)
        handler = handle_alongPathToBuf;
    else if (std::strcmp(commandId, "foliage_un06.clump") == 0)
        handler = handle_clumpToBuf;
    else if (std::strcmp(commandId, "foliage_un06.brush_shapes") == 0)
        handler = handle_brushShapesToBuf;
    else if (std::strcmp(commandId, "foliage_un06.random_replace") == 0)
        handler = handle_randomReplaceToBuf;
    else if (std::strcmp(commandId, "foliage_un06.filter_by_type") == 0)
        handler = handle_filterByTypeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.transform_instances") == 0)
        handler = handle_transformInstancesToBuf;
    else if (std::strcmp(commandId, "foliage_un06.wind_params") == 0)
        handler = handle_windParamsToBuf;
    else if (std::strcmp(commandId, "foliage_un06.collision_settings") == 0)
        handler = handle_collisionSettingsToBuf;
    else if (std::strcmp(commandId, "foliage_un06.lod_config") == 0)
        handler = handle_lodConfigToBuf;
    else if (std::strcmp(commandId, "foliage_un06.statistics_grouped") == 0)
        handler = handle_statisticsGroupedToBuf;
    else if (std::strcmp(commandId, "foliage_un06.list_commands") == 0)
        handler = handle_listCommandsToBuf;
    else if (std::strcmp(commandId, "foliage_un06.health_check") == 0)
        handler = handle_healthCheckToBuf;
    else if (std::strcmp(commandId, "foliage_un06.scatter_multi_type") == 0)
        handler = handle_scatterMultiTypeToBuf;
    else if (std::strcmp(commandId, "foliage_un06.select_instances") == 0)
        handler = handle_selectInstancesToBuf;
    else if (std::strcmp(commandId, "foliage_un06.clear_store") == 0)
        handler = handle_clearStoreToBuf;
    else if (std::strcmp(commandId, "foliage_un06.duplicate_store") == 0)
        handler = handle_duplicateStoreToBuf;
    else if (std::strcmp(commandId, "foliage_un06.compact_store") == 0)
        handler = handle_compactStoreToBuf;
    else if (std::strcmp(commandId, "foliage_un06.reseed") == 0)
        handler = handle_reseedToBuf;
    else if (std::strcmp(commandId, "foliage_un06.validate_payload") == 0)
        handler = handle_validatePayloadToBuf;
    else if (std::strcmp(commandId, "foliage_un06.diff_stores") == 0)
        handler = handle_diffStoresToBuf;
    else if (std::strcmp(commandId, "foliage_un06.export_engine_payload") == 0)
        handler = handle_exportEnginePayloadToBuf;
    else if (std::strcmp(commandId, "foliage_un06.density_heatmap") == 0)
        handler = handle_densityHeatmapToBuf;
    else if (std::strcmp(commandId, "foliage_un06.compute_bounds") == 0)
        handler = handle_computeBoundsToBuf;
    else if (std::strcmp(commandId, "foliage_un06.auto_lod") == 0)
        handler = handle_autoLODToBuf;
    else if (std::strcmp(commandId, "foliage_un06.group_by_cluster") == 0)
        handler = handle_groupByClusterToBuf;
    else if (std::strcmp(commandId, "foliage_un06.jitter_positions") == 0)
        handler = handle_jitterPositionsToBuf;
    else if (std::strcmp(commandId, "foliage_un06.export_instances_csv") == 0)
        handler = handle_exportCSVToBuf;
    else if (std::strcmp(commandId, "foliage_un06.snap_to_terrain") == 0)
        handler = handle_snapToTerrainToBuf;
    else if (std::strcmp(commandId, "foliage_un06.remove_outliers") == 0)
        handler = handle_removeOutliersToBuf;
    else if (std::strcmp(commandId, "foliage_un06.coverage_analysis") == 0)
        handler = handle_coverageAnalysisToBuf;
    else if (std::strcmp(commandId, "foliage_un06.pipeline") == 0)
        handler = handle_pipelineToBuf;
    else if (std::strcmp(commandId, "foliage_un06.store_memory") == 0)
        handler = handle_storeMemoryToBuf;
    else
        return MONTED_STATUS_NOT_IMPLEMENTED;

    int code = handler(payloadJson, outJson, outJsonSize);
    if (outRequiredSize) {
        *outRequiredSize = static_cast<std::uint32_t>(std::strlen(outJson) + 1);
    }
    return code == 0 ? MONTED_STATUS_OK : static_cast<MontEdStatus>(code);
}

// ---- V4 entry point ---------------------------------------------------

extern "C" MONTED_API MontEdStatus MONTED_CALL
MontEd_GetPluginApiV4(std::uint32_t requestedVersion, void* outApi, std::uint32_t outApiSize)
{
    if (!outApi || outApiSize < sizeof(MontEdPluginApi)) {
        return MONTED_STATUS_INVALID_ARGUMENT;
    }
    if (requestedVersion > MONTED_ABI_VERSION_CURRENT) {
        return MONTED_STATUS_INVALID_ARGUMENT; // plugin too old for host
    }

    auto* api = static_cast<MontEdPluginApi*>(outApi);
    std::memset(api, 0, sizeof(*api));

    api->structSize = sizeof(MontEdPluginApi);
    api->abiVersion = MONTED_ABI_VERSION_CURRENT;

    api->identity.structSize = sizeof(MontEdPluginIdentity);
    api->identity.abiVersion = MONTED_ABI_VERSION_CURRENT;
    api->identity.pluginId       = "com.monted.plugins.foliage_un06";
    api->identity.pluginName     = "foliage_un06";
    api->identity.pluginVersion  = "0.5.0";
    api->identity.pluginType     = "foliage_tool";
    api->identity.protocolPreset = "core-bus";

    api->getCapabilitiesJson  = foliage_getCapabilitiesJson;
    api->getBusHandlersJson   = foliage_getBusHandlersJson;
    api->getUiPanelsJson      = foliage_getUiPanelsJson;
    api->getSceneToolsJson    = foliage_getSceneToolsJson;
    api->getBlueprintNodesJson = foliage_getBlueprintNodesJson;
    api->executeCommand       = foliage_executeCommand;

    return MONTED_STATUS_OK;
}

// ---- V3 legacy entry point --------------------------------------------

struct MontEdPluginApiV3 {
    std::uint32_t abiVersion;
    const char* pluginId;
    const char* pluginName;
    const char* pluginType;
    const char* protocolPreset;
};

extern "C" MONTED_API std::uint32_t MONTED_CALL
MontEd_GetPluginApi(std::uint32_t requestedVersion, void* outApi)
{
    if (outApi == nullptr) return MONTED_STATUS_INVALID_ARGUMENT;
    if (requestedVersion != 3u) return 7u; // MONTED_STATUS_ABI_MISMATCH

    auto* api = static_cast<MontEdPluginApiV3*>(outApi);
    api->abiVersion     = 3u;
    api->pluginId       = "com.monted.plugins.foliage_un06";
    api->pluginName     = "foliage_un06";
    api->pluginType     = "foliage_tool";
    api->protocolPreset = "core-bus";
    return 0u; // MONTED_STATUS_OK
}

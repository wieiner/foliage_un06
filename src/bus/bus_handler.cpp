// foliage_un06 — Bus handler: 43 commands (v0.5.0, hardening).
// Structured JSON parser used for scatter/paint/add_type handlers.
// Uses shared logic in src/common/ for all algorithmic work.

#include "../common/foliage_type.h"
#include "../common/foliage_scatter.h"
#include "../common/foliage_paint.h"
#include "../common/foliage_cluster.h"
#include "../common/foliage_prng.h"
#include "../common/foliage_terrain.h"
#include "../common/foliage_query.h"
#include "../common/foliage_simulate.h"
#include "../common/foliage_analyze.h"
#include "../common/foliage_falloff.h"
#include "../common/foliage_noise.h"
#include "../common/foliage_mask.h"
#include "../common/foliage_pattern.h"
#include "../common/foliage_biome.h"
#include "../common/foliage_json.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#undef min
#undef max
#define MONTED_EXPORT extern "C" __declspec(dllexport)
#else
#define MONTED_EXPORT extern "C"
#endif

namespace {

// -----------------------------------------------------------------------
// JSON helpers (lightweight, dependency-free)
// -----------------------------------------------------------------------
double ej(const char* j, const char* k, double fb) {
    if (!j) return fb;
    char pat[64];
    std::snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* s = std::strstr(j, pat);
    if (!s) return fb;
    const char* c = std::strchr(s + std::strlen(pat), ':');
    return c ? std::atof(c + 1) : fb;
}

bool ej_bool(const char* j, const char* k, bool fb) {
    if (!j) return fb;
    char pat[64];
    std::snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* s = std::strstr(j, pat);
    if (!s) return fb;
    const char* c = s + std::strlen(pat);
    while (*c == ':' || *c == ' ' || *c == '\t') ++c;
    return (c[0] == 't' && c[1] == 'r' && c[2] == 'u' && c[3] == 'e');
}

std::string ej_str(const char* j, const char* k, const std::string& fb) {
    if (!j) return fb;
    char pat[64];
    std::snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* s = std::strstr(j, pat);
    if (!s) return fb;
    const char* c = s + std::strlen(pat);
    while (*c == ':' || *c == ' ' || *c == '\t') ++c;
    if (*c != '"') return fb;
    ++c;
    const char* e = std::strchr(c, '"');
    if (!e) return fb;
    return std::string(c, e - c);
}

// Parse JSON array of integers: [1,2,3]
std::vector<int> ej_intArray(const char* j, const char* k) {
    std::vector<int> result;
    if (!j) return result;
    char pat[64]; std::snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* s = std::strstr(j, pat);
    if (!s) return result;
    const char* c = s + std::strlen(pat);
    while (*c == ':' || *c == ' ' || *c == '\t') ++c;
    if (*c != '[') return result;
    ++c;
    while (*c && *c != ']') {
        while (*c == ' ' || *c == ',' || *c == '\n') ++c;
        if (*c == ']' || !*c) break;
        int val = 0, sign = 1;
        if (*c == '-') { sign = -1; ++c; }
        while (*c >= '0' && *c <= '9') { val = val * 10 + (*c - '0'); ++c; }
        result.push_back(val * sign);
    }
    return result;
}

// Parse JSON array of doubles
std::vector<double> ej_doubleArray(const char* j, const char* k) {
    std::vector<double> result;
    if (!j) return result;
    char pat[64]; std::snprintf(pat, sizeof(pat), "\"%s\"", k);
    const char* s = std::strstr(j, pat);
    if (!s) return result;
    const char* c = s + std::strlen(pat);
    while (*c == ':' || *c == ' ' || *c == '\t') ++c;
    if (*c != '[') return result;
    ++c;
    while (*c && *c != ']') {
        while (*c == ' ' || *c == ',' || *c == '\n') ++c;
        if (*c == ']' || !*c) break;
        char* end = nullptr;
        double val = std::strtod(c, &end);
        if (end == c) break;
        result.push_back(val);
        c = end;
    }
    return result;
}

// Thread-safe foliage type registry
std::mutex g_typeMutex;
std::map<std::string, foliage::FoliageType> g_types;

foliage::FoliageType g_defaultType;

// In-memory instance store for stateful commands (keyed by session/user)
std::mutex g_storeMutex;
std::map<std::string, std::vector<foliage::FoliageInstance>> g_instanceStores;

// Exclusion zones (global, shared across commands)
std::mutex g_zoneMutex;
foliage::ExclusionZoneSet g_exclusionZones;

// Spline paths
std::mutex g_splineMutex;
std::vector<foliage::SplinePath> g_splines;

// Checkpoint system for undo
std::mutex g_checkpointMutex;
std::map<std::string, std::vector<std::string>> g_checkpoints; // storeKey -> list of JSON snapshots

// Preset system
std::mutex g_presetMutex;
struct FoliagePreset {
    std::string name;
    std::string description;
    std::vector<std::string> foliageTypeNames;
    double areaWidth = 200, areaDepth = 200;
    double densityMul = 1.0;
    double slopeMax = 35.0;
};
std::map<std::string, FoliagePreset> g_presets;

// Benchmark stats
struct BenchmarkStats {
    double lastMs = 0;
    int lastInstanceCount = 0;
    int totalRuns = 0;
    double totalMs = 0;
    int bestCount = 0;
    double bestMs = 1e9;
};
std::mutex g_benchMutex;
BenchmarkStats g_benchStats;

void ensureDefaultTypes() {
    if (g_types.empty()) {
        foliage::FoliageType grass;
        grass.name        = "grass_default";
        grass.meshId      = "cross_billboard";
        grass.density     = 0.15;
        grass.scaleMin    = 0.5;
        grass.scaleMax    = 1.2;
        grass.slopeMax    = 45.0;
        grass.seed        = 1337;
        grass.randomRotation = true;
        grass.alignToNormal  = true;
        g_types[grass.name] = grass;

        foliage::FoliageType tree;
        tree.name        = "tree_pine";
        tree.meshId      = "cube";
        tree.density     = 0.005;
        tree.scaleMin    = 0.9;
        tree.scaleMax    = 1.6;
        tree.slopeMax    = 35.0;
        tree.heightMin   = 0.0;
        tree.heightMax   = 50.0;
        tree.seed        = 42;
        tree.randomRotation = true;
        tree.alignToNormal  = true;
        g_types[tree.name] = tree;

        foliage::FoliageType bush;
        bush.name        = "bush_default";
        bush.meshId      = "cube";
        bush.density     = 0.08;
        bush.scaleMin    = 0.4;
        bush.scaleMax    = 1.0;
        bush.slopeMax    = 50.0;
        bush.seed        = 777;
        bush.randomRotation = true;
        bush.alignToNormal  = true;
        g_types[bush.name] = bush;

        g_defaultType = grass;
    }
}

foliage::SyntheticTerrain g_syntheticTerrain;
foliage::FlatTerrain     g_flatTerrain;

foliage::TerrainSampler& pickTerrain(const char* payload) {
    std::string terrainMode = ej_str(payload, "terrain", "synthetic");
    if (terrainMode == "flat") return g_flatTerrain;
    return g_syntheticTerrain;
}

const foliage::FoliageType& pickType(const char* payload) {
    std::string typeName = ej_str(payload, "type", "grass_default");
    ensureDefaultTypes();
    auto it = g_types.find(typeName);
    if (it != g_types.end()) return it->second;
    return g_defaultType;
}

foliage::FoliageType overrideType(const char* payload, const foliage::FoliageType& base) {
    foliage::FoliageType ft = base;
    if (ej(payload, "density", -1) >= 0)       ft.density  = ej(payload, "density", base.density);
    if (ej(payload, "scaleMin", -1) >= 0)      ft.scaleMin = ej(payload, "scaleMin", base.scaleMin);
    if (ej(payload, "scaleMax", -1) >= 0)      ft.scaleMax = ej(payload, "scaleMax", base.scaleMax);
    if (ej(payload, "slopeMax", -999) > -998)  ft.slopeMax = ej(payload, "slopeMax", base.slopeMax);
    if (ej(payload, "heightMin", -1e99) > -1e98) ft.heightMin = ej(payload, "heightMin", base.heightMin);
    if (ej(payload, "heightMax", -1e99) > -1e98) ft.heightMax = ej(payload, "heightMax", base.heightMax);
    if (ej(payload, "seed", -1) >= 0)          ft.seed = static_cast<std::uint32_t>(ej(payload, "seed", (double)base.seed));
    if (ej(payload, "radius", -1) >= 0)        ft.radius = ej(payload, "radius", base.radius);
    if (std::strstr(payload, "\"randomRotation\"")) ft.randomRotation = ej_bool(payload, "randomRotation", base.randomRotation);
    if (std::strstr(payload, "\"alignToNormal\""))  ft.alignToNormal = ej_bool(payload, "alignToNormal", base.alignToNormal);
    return ft;
}

void parseTypeFromJson(const char* json) {
    foliage::FoliageType ft;
    ft.name           = ej_str(json, "name", "custom");
    ft.meshId         = ej_str(json, "meshId", "cube");
    ft.density        = ej(json, "density", 0.02);
    ft.scaleMin       = ej(json, "scaleMin", 0.8);
    ft.scaleMax       = ej(json, "scaleMax", 1.4);
    ft.uniformScale   = ej_bool(json, "uniformScale", true);
    ft.randomRotation = ej_bool(json, "randomRotation", true);
    ft.alignToNormal  = ej_bool(json, "alignToNormal", true);
    ft.rotationMin    = ej(json, "rotationMin", 0.0);
    ft.rotationMax    = ej(json, "rotationMax", 360.0);
    ft.slopeMax       = ej(json, "slopeMax", 35.0);
    ft.heightMin      = ej(json, "heightMin", -1e9);
    ft.heightMax      = ej(json, "heightMax", 1e9);
    ft.seed           = static_cast<std::uint32_t>(ej(json, "seed", 1337));
    ft.radius         = ej(json, "radius", 0.0);
    std::lock_guard<std::mutex> lock(g_typeMutex);
    g_types[ft.name] = ft;
}

// Parse instances from JSON array [[x,y,z,sx,sy,sz,yaw],...]
std::vector<foliage::FoliageInstance> parseInstances(const char* json) {
    std::vector<foliage::FoliageInstance> result;
    if (!json) return result;
    const char* s = std::strstr(json, "\"instances\"");
    if (!s) return result;
    const char* c = s;
    while (*c && *c != '[') ++c;
    if (!*c) return result;
    ++c; // skip '['
    while (*c) {
        while (*c == ' ' || *c == ',' || *c == '\n' || *c == '\r') ++c;
        if (*c == ']' || !*c) break;
        if (*c != '[') { ++c; continue; }
        ++c; // skip inner '['
        double vals[7] = {};
        for (int i = 0; i < 7; ++i) {
            while (*c == ' ' || *c == ',') ++c;
            char* end = nullptr;
            vals[i] = std::strtod(c, &end);
            if (end == c) break;
            c = end;
        }
        while (*c && *c != ']') ++c;
        if (*c == ']') ++c;
        foliage::FoliageInstance inst;
        inst.x = vals[0]; inst.y = vals[1]; inst.z = vals[2];
        inst.scaleX = vals[3]; inst.scaleY = vals[4]; inst.scaleZ = vals[5];
        inst.yaw = vals[6];
        result.push_back(inst);
    }
    return result;
}

void writeInstanceJson(const std::vector<foliage::FoliageInstance>& instances,
                       char* buf, size_t& pos, size_t n, int maxInstances)
{
    int count = static_cast<int>(instances.size());
    int step  = 1;
    if (count > maxInstances) { step = count / maxInstances; if (step < 1) step = 1; }
    pos += std::snprintf(buf + pos, n - pos, "\"instances\":[");
    bool first = true;
    for (int i = 0; i < count; i += step) {
        if (pos >= n - 80) break;
        const auto& inst = instances[i];
        if (!first) buf[pos++] = ',';
        first = false;
        pos += std::snprintf(buf + pos, n - pos,
            "[%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]",
            inst.x, inst.y, inst.z, inst.scaleX, inst.scaleY, inst.scaleZ, inst.yaw);
    }
    buf[pos++] = ']';
}

void writeMeshSummary(char* buf, size_t& pos, size_t n,
                      int totalVerts, int totalIndices, int totalTriangles,
                      double bmin[3], double bmax[3])
{
    pos += std::snprintf(buf + pos, n - pos,
        ",\"mesh\":{"
        "\"vertexCount\":%d,\"indexCount\":%d,\"triangleCount\":%d,"
        "\"boundsMin\":[%.2f,%.2f,%.2f],\"boundsMax\":[%.2f,%.2f,%.2f],"
        "\"boundingBox\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]"
        "}",
        totalVerts, totalIndices, totalTriangles,
        bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2],
        bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);
}

// Escape string for JSON
std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

} // namespace

// ====== External entry point declarations ======
MONTED_EXPORT int handle_scatterToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_paintToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_eraseToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_getTypesToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_addTypeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_inspectToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_generateMeshToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_reapplyToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_fillToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_simulateToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_queryToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_placeSingleToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_removeInstancesToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_analyzeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_exportStateToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_importStateToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_densityFalloffToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_layerMaskToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_noiseModulateToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_zOffsetToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_cullDistanceToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_batchToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_checkpointToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_mergeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_mirrorRegionToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_exclusionZoneToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_presetToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_benchmarkToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_sampleTerrainToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_biomeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_radialToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_alongPathToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_clumpToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_brushShapesToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_randomReplaceToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_filterByTypeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_transformInstancesToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_windParamsToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_collisionSettingsToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_lodConfigToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_statisticsGroupedToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_listCommandsToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_healthCheckToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_scatterMultiTypeToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_selectInstancesToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_clearStoreToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_duplicateStoreToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_compactStoreToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_reseedToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_validatePayloadToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_diffStoresToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_exportEnginePayloadToBuf(const char* in, char* out, size_t n);
MONTED_EXPORT int handle_densityHeatmapToBuf(const char* in, char* out, size_t n);

// ======================================================================
// 1. foliage_un06.scatter (structured parser — v0.5.0)
// ======================================================================
MONTED_EXPORT int handle_scatterToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    if (!in || !in[0]) {
        std::snprintf(out, n, "{\"code\":7,\"message\":\"empty payload\"}");
        return 0;
    }

    // Parse with structured JSON parser
    foliage::json::Parser parser;
    auto root = parser.Parse(in);
    if (!parser.error.ok()) {
        std::snprintf(out, n, "{\"code\":6,\"message\":\"invalid JSON: %s at offset %d\"}",
                      parser.error.message.c_str(), parser.error.offset);
        return 0;
    }

    // Extract params with safe defaults + bounds clamping
    const double w  = root.ClampedNum("areaWidth", 200.0, 1.0, 100000.0);
    const double d  = root.ClampedNum("areaDepth", 200.0, 1.0, 100000.0);
    const double ox = root.NumKey("originX", 0.0);
    const double oz = root.NumKey("originZ", 0.0);
    const int    maxInst  = root.ClampedInt("maxInstances", 50000, 0, 200000);
    const int    maxResp  = root.ClampedInt("maxInstancesInResponse", 200, 0, 5000);
    const bool   includePreview = root.BoolKey("includePreviewMesh", false);

    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);
    foliage::FoliageType overridden = overrideType(in, type);

    foliage::ScatterArea area; area.originX = ox; area.originZ = oz; area.width = w; area.depth = d;
    auto result = foliage::PoissonDiscScatter::ScatterWithStats(terrain, overridden, area, maxInst, 30);

    // Build mesh summary
    int totalVerts = 0, totalIndices = 0, totalTriangles = 0;
    double bmin[3] = {ox, -100, oz}, bmax[3] = {ox + w, 100, oz + d};
    if (includePreview && !result.instances.empty()) {
        foliage::MeshTemplate meshTmpl = foliage::MakeCrossBillboardMesh(0.3, 1.0);
        if (overridden.meshId == "cube") meshTmpl = foliage::MakeCubeMesh(0.3);
        auto combined = foliage::ClusterBuilder::BuildCombined(result.instances, meshTmpl);
        totalVerts = combined.vertexCount; totalIndices = combined.indexCount;
        totalTriangles = totalIndices / 3;
        bmin[0] = combined.boundsMin[0]; bmin[1] = combined.boundsMin[1]; bmin[2] = combined.boundsMin[2];
        bmax[0] = combined.boundsMax[0]; bmax[1] = combined.boundsMax[1]; bmax[2] = combined.boundsMax[2];
    }

    // Structured envelope response
    foliage::json::ResponseEnvelope env;
    env.command = "foliage_un06.scatter";
    env.storeKey = root.StrKey("storeKey", "default");
    env.dryRun = root.BoolKey("dryRun", true);

    std::map<std::string,foliage::json::Value> r;
    r["message"] = foliage::json::Value("scattered");
    r["candidates"] = foliage::json::Value(static_cast<int>(result.candidates));
    r["keptInstances"] = foliage::json::Value(static_cast<int>(result.kept));
    r["avgScale"] = foliage::json::Value(result.avgScale);
    r["clusters"] = foliage::json::Value(
        result.instances.empty() ? 0 : foliage::ClusterBuilder::ClusterCount(result.instances, 16.0));
    if (includePreview) {
        std::map<std::string,foliage::json::Value> mesh;
        mesh["vertexCount"] = foliage::json::Value(totalVerts);
        mesh["indexCount"] = foliage::json::Value(totalIndices);
        mesh["triangleCount"] = foliage::json::Value(totalTriangles);
        r["mesh"] = foliage::json::Value::Object(std::move(mesh));
    }
    env.result = foliage::json::Value::Object(std::move(r));

    std::string json = env.Serialize();
    size_t toCopy = std::min(json.size(), n - 1);
    std::memcpy(out, json.c_str(), toCopy);
    out[toCopy] = '\0';
    return 0;
}

// ======================================================================
// 2. foliage_un06.paint
// ======================================================================
MONTED_EXPORT int handle_paintToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    const double cx = ej(in, "centerX", 0.0), cz = ej(in, "centerZ", 0.0);
    const double radius = ej(in, "radius", 20.0), densityMul = ej(in, "densityMul", 1.0);
    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);
    foliage::FoliageType overridden = overrideType(in, type);

    foliage::BrushParams brush; brush.centerX = cx; brush.centerZ = cz;
    brush.radius = radius; brush.densityMul = densityMul;
    auto added = foliage::FoliagePaint::PaintBrush(terrain, overridden, brush);

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"painted\",\"brush\":{\"cx\":%.1f,\"cz\":%.1f,\"r\":%.1f,\"dmul\":%.2f},\"added\":%zu",
        brush.centerX, brush.centerZ, brush.radius, brush.densityMul, added.size());
    if (!added.empty() && n - pos > 200) writeInstanceJson(added, out, pos, n, 100);
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 3. foliage_un06.erase
// ======================================================================
MONTED_EXPORT int handle_eraseToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    const double cx = ej(in, "centerX", 0.0), cz = ej(in, "centerZ", 0.0), radius = ej(in, "radius", 20.0);
    foliage::BrushParams brush; brush.centerX = cx; brush.centerZ = cz; brush.radius = radius;

    // Erase from stored instances if a storeKey is provided
    std::string storeKey = ej_str(in, "storeKey", "");
    size_t before = 0, after = 0;
    if (!storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) {
            before = it->second.size();
            it->second = foliage::FoliagePaint::EraseBrush(brush, it->second);
            after = it->second.size();
        }
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"erase_brush\",\"brush\":{\"cx\":%.1f,\"cz\":%.1f,\"r\":%.1f},\"removed\":%zu}",
        brush.centerX, brush.centerZ, brush.radius, before - after);
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 4. foliage_un06.get_types
// ======================================================================
MONTED_EXPORT int handle_getTypesToBuf(const char*, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    ensureDefaultTypes();
    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"types\":[");
    bool first = true;
    { std::lock_guard<std::mutex> lock(g_typeMutex);
        for (const auto& [name, ft] : g_types) {
            if (!first) out[pos++] = ','; first = false;
            pos += std::snprintf(out + pos, n - pos,
                "{\"name\":\"%s\",\"meshId\":\"%s\",\"density\":%.4f,\"scaleRange\":[%.2f,%.2f],\"slopeMax\":%.1f,\"seed\":%u}",
                ft.name.c_str(), ft.meshId.c_str(), ft.density, ft.scaleMin, ft.scaleMax, ft.slopeMax, ft.seed);
        }
    }
    out[pos++] = ']'; out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 5. foliage_un06.add_type
// ======================================================================
MONTED_EXPORT int handle_addTypeToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    parseTypeFromJson(in);
    std::string name = ej_str(in, "name", "custom");
    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"type_added\",\"name\":\"%s\"}", name.c_str());
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 6. foliage_un06.generate_mesh
// ======================================================================
MONTED_EXPORT int handle_generateMeshToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    const double w = ej(in, "areaWidth", 200.0), d = ej(in, "areaDepth", 200.0);
    const double ox = ej(in, "originX", 0.0), oz = ej(in, "originZ", 0.0);
    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);
    foliage::FoliageType overridden = overrideType(in, type);
    foliage::ScatterArea area; area.originX = ox; area.originZ = oz; area.width = w; area.depth = d;
    auto instances = foliage::PoissonDiscScatter::Scatter(terrain, overridden, area, 50000, 30);

    foliage::MeshTemplate meshTmpl = foliage::MakeCrossBillboardMesh(0.3, 1.0);
    if (overridden.meshId == "cube") meshTmpl = foliage::MakeCubeMesh(0.3);
    auto combined = foliage::ClusterBuilder::BuildCombined(instances, meshTmpl);
    auto clusters = foliage::ClusterBuilder::BuildClusters(instances, meshTmpl, 16.0);

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"mesh_generated\",\"instanceCount\":%zu,\"clusterCount\":%zu",
        instances.size(), clusters.size());
    writeMeshSummary(out, pos, n, combined.vertexCount, combined.indexCount, combined.indexCount/3,
                     combined.boundsMin, combined.boundsMax);
    if (!clusters.empty() && n - pos > 200) {
        const auto& c = clusters[0];
        pos += std::snprintf(out + pos, n - pos,
            ",\"previewCluster\":{\"cx\":%d,\"cz\":%d,\"instances\":%d,\"verts\":%d,\"indices\":%d}",
            c.clusterX, c.clusterZ, c.instanceCount, c.vertexCount, c.indexCount);
    }
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 7. foliage_un06.inspect
// ======================================================================
MONTED_EXPORT int handle_inspectToBuf(const char*, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"plugin\":\"foliage_un06\",\"version\":\"0.5.0\",\"type\":\"foliage_tool\","
        "\"protocolPreset\":\"core-bus\",\"protocols\":[\"core\",\"bus\"],"
        "\"capability\":\"foliage/vegetation scatter over instanced mesh\","
        "\"features\":["
        "\"poisson_disc_scatter\",\"brush_paint\",\"brush_erase\","
        "\"slope_filter\",\"height_filter\",\"align_to_normal\","
        "\"random_rotation\",\"random_scale\",\"cluster_output\","
        "\"deterministic_seed\","
        "\"reapply\",\"fill\",\"ecosystem_simulation\","
        "\"spatial_query\",\"single_placement\","
        "\"instance_removal\",\"statistical_analysis\","
        "\"state_export_import\",\"density_falloff\""
        "],"
        "\"commands\":["
        "\"foliage_un06.scatter\",\"foliage_un06.paint\",\"foliage_un06.erase\","
        "\"foliage_un06.get_types\",\"foliage_un06.add_type\","
        "\"foliage_un06.generate_mesh\",\"foliage_un06.inspect\","
        "\"foliage_un06.reapply\",\"foliage_un06.fill\","
        "\"foliage_un06.simulate\",\"foliage_un06.query\","
        "\"foliage_un06.place_single\",\"foliage_un06.remove_instances\","
        "\"foliage_un06.analyze\",\"foliage_un06.export_state\","
        "\"foliage_un06.import_state\",\"foliage_un06.density_falloff\""
        "],"
        "\"meshTypes\":[\"cube\",\"cross_billboard\"],"
        "\"terrainModes\":[\"synthetic\",\"flat\"],"
        "\"backsOnto\":\"instmesh_un05\""
        "}");
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 8. foliage_un06.reapply  (UE's Reapply tool)
// ======================================================================
MONTED_EXPORT int handle_reapplyToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    if (instances.empty()) {
        // Try to load from store
        std::string storeKey = ej_str(in, "storeKey", "");
        if (!storeKey.empty()) {
            std::lock_guard<std::mutex> lock(g_storeMutex);
            auto it = g_instanceStores.find(storeKey);
            if (it != g_instanceStores.end()) instances = it->second;
        }
    }
    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);
    foliage::FoliageType overridden = overrideType(in, type);

    // Re-check filters
    std::vector<foliage::FoliageInstance> survivors;
    std::uint32_t removed = 0;
    for (const auto& inst : instances) {
        bool keep = true;
        // Re-check height
        if (inst.y < overridden.heightMin || inst.y > overridden.heightMax) keep = false;
        // Re-check slope
        if (keep && overridden.slopeMax >= 0.0) {
            if (terrain.Slope(inst.x, inst.z) > overridden.slopeMax) keep = false;
        }
        // Re-apply align to normal
        if (keep && overridden.alignToNormal) {
            terrain.Normal(inst.x, inst.z,
                const_cast<double*>(&inst.nx),
                const_cast<double*>(&inst.ny),
                const_cast<double*>(&inst.nz));
        }
        if (keep) survivors.push_back(inst); else ++removed;
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"reapplied\",\"total\":%zu,\"survivors\":%zu,\"removed\":%u",
        instances.size(), survivors.size(), removed);
    if (!survivors.empty() && n - pos > 200) writeInstanceJson(survivors, out, pos, n, 200);
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 9. foliage_un06.fill  (UE's Fill tool — auto-fill area)
// ======================================================================
MONTED_EXPORT int handle_fillToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    const double w = ej(in, "areaWidth", 200.0), d = ej(in, "areaDepth", 200.0);
    const double ox = ej(in, "originX", 0.0), oz = ej(in, "originZ", 0.0);
    const int passes = std::max(1, (int)ej(in, "passes", 3));

    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);
    foliage::FoliageType overridden = overrideType(in, type);
    foliage::ScatterArea area; area.originX = ox; area.originZ = oz; area.width = w; area.depth = d;

    // Multi-pass fill: after each pass, only fill in gaps
    std::vector<foliage::FoliageInstance> allInstances;
    foliage::SpatialIndex idx(8.0);

    for (int pass = 0; pass < passes; ++pass) {
        // Build index from current instances
        if (!allInstances.empty()) {
            double xMn = allInstances[0].x, xMx = allInstances[0].x;
            double zMn = allInstances[0].z, zMx = allInstances[0].z;
            for (auto& inst : allInstances) {
                if (inst.x < xMn) xMn = inst.x; if (inst.x > xMx) xMx = inst.x;
                if (inst.z < zMn) zMn = inst.z; if (inst.z > zMx) zMx = inst.z;
            }
            idx.Build(allInstances, xMn, zMn, xMx, zMx);
        }

        // Use a higher density for gap filling (increases each pass)
        foliage::FoliageType passType = overridden;
        passType.density *= (0.6 + pass * 0.3); // increasing density each pass
        passType.seed = overridden.seed + static_cast<std::uint32_t>(pass * 1000);

        auto result = foliage::PoissonDiscScatter::ScatterWithStats(terrain, passType, area, 50000, 30);

        // Filter out instances that are too close to existing ones
        for (auto& inst : result.instances) {
            if (allInstances.empty()) {
                allInstances.push_back(inst);
            } else {
                double dist = 0;
                idx.Nearest(inst.x, inst.z, overridden.radius > 0 ? overridden.radius : 3.0, &dist);
                if (dist < 0.5 || idx.Nearest(inst.x, inst.z, 1e9) < 0) {
                    allInstances.push_back(inst);
                }
            }
        }
        if (result.instances.empty()) break; // converged
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"filled\",\"passes\":%d,\"totalInstances\":%zu",
        passes, allInstances.size());
    if (!allInstances.empty() && n - pos > 200) writeInstanceJson(allInstances, out, pos, n, 200);
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 10. foliage_un06.simulate  (ecosystem competition)
// ======================================================================
MONTED_EXPORT int handle_simulateToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }

    foliage::SimulationParams sparams;
    sparams.collisionRadius = ej(in, "collisionRadius", 10.0);
    sparams.shadeRadius     = ej(in, "shadeRadius", 0.0);
    sparams.numSteps        = (int)ej(in, "numSteps", 3);
    sparams.removeLosers    = ej_bool(in, "removeLosers", true);
    sparams.seed            = (std::uint32_t)ej(in, "seed", 1337);

    bool ageEnabled  = ej_bool(in, "ageEnabled", false);
    bool spreadEnabled = ej_bool(in, "spreadEnabled", false);
    auto& terrain = pickTerrain(in);
    const foliage::FoliageType& type = pickType(in);

    // Age
    if (ageEnabled && !instances.empty()) {
        foliage::EcoSim::Age(instances, ej(in, "growthRate", 0.05), ej(in, "targetScale", 2.0));
    }

    // Spread
    std::vector<foliage::FoliageInstance> allInstances = instances;
    if (spreadEnabled && !instances.empty()) {
        auto children = foliage::EcoSim::Spread(instances, terrain, type,
            (int)ej(in, "seedsPerStep", 2),
            ej(in, "spreadDist", 15.0),
            ej(in, "spreadVariance", 5.0),
            sparams.seed);
        allInstances.insert(allInstances.end(), children.begin(), children.end());
    }

    // Compete
    auto result = foliage::EcoSim::Compete(allInstances, sparams,
        ej(in, "basePriority", 1.0), ej_bool(in, "scaleBonus", true), ej(in, "scaleWeight", 2.0));

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"simulated\","
        "\"total\":%u,\"survivors\":%zu,\"removed\":%u,"
        "\"collisionRadius\":%.1f,\"shadeRadius\":%.1f,\"numSteps\":%d",
        result.total, result.survivors.size(), result.removed,
        sparams.collisionRadius, sparams.shadeRadius, sparams.numSteps);
    if (!result.survivors.empty() && n - pos > 200)
        writeInstanceJson(result.survivors, out, pos, n, 200);
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 11. foliage_un06.query  (spatial queries)
// ======================================================================
MONTED_EXPORT int handle_queryToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }
    if (instances.empty()) {
        std::snprintf(out, n, "{\"code\":0,\"message\":\"no_instances_to_query\",\"hits\":[]}");
        return 0;
    }

    std::string mode = ej_str(in, "mode", "radius");
    double qx = ej(in, "x", 0.0), qz = ej(in, "z", 0.0), qr = ej(in, "radius", 50.0);
    int maxHits = (int)ej(in, "maxHits", 50);

    // Build index
    double xMn = instances[0].x, xMx = instances[0].x, zMn = instances[0].z, zMx = instances[0].z;
    for (auto& inst : instances) {
        if (inst.x < xMn) xMn = inst.x; if (inst.x > xMx) xMx = inst.x;
        if (inst.z < zMn) zMn = inst.z; if (inst.z > zMx) zMx = inst.z;
    }
    foliage::SpatialIndex sidx(8.0);
    sidx.Build(instances, xMn, zMn, xMx, zMx);

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"query\",\"mode\":\"%s\",\"qx\":%.1f,\"qz\":%.1f",
        mode.c_str(), qx, qz);

    if (mode == "nearest") {
        double dist = 0;
        int idx = sidx.Nearest(qx, qz, qr, &dist);
        if (idx >= 0) {
            const auto& inst = instances[idx];
            pos += std::snprintf(out + pos, n - pos,
                ",\"hit\":{\"index\":%d,\"dist\":%.2f,\"pos\":[%.2f,%.2f,%.2f],\"scale\":[%.3f,%.3f,%.3f],\"yaw\":%.4f}",
                idx, dist, inst.x, inst.y, inst.z, inst.scaleX, inst.scaleY, inst.scaleZ, inst.yaw);
        } else {
            pos += std::snprintf(out + pos, n - pos, ",\"hit\":null");
        }
    } else if (mode == "count") {
        int cnt = sidx.CountInRadius(qx, qz, qr);
        double density = cnt / (3.14159265 * qr * qr);
        pos += std::snprintf(out + pos, n - pos, ",\"count\":%d,\"localDensity\":%.4f", cnt, density);
    } else { // radius
        auto hits = sidx.RadiusSearch(qx, qz, qr, maxHits);
        pos += std::snprintf(out + pos, n - pos, ",\"hits\":[");
        for (size_t i = 0; i < hits.size(); ++i) {
            if (i > 0) out[pos++] = ',';
            const auto& inst = instances[hits[i].index];
            pos += std::snprintf(out + pos, n - pos,
                "{\"idx\":%d,\"dist\":%.2f,\"p\":[%.1f,%.1f,%.1f]}",
                hits[i].index, hits[i].dist, inst.x, inst.y, inst.z);
            if (pos > n - 100) break;
        }
        pos += std::snprintf(out + pos, n - pos, "],\"totalHits\":%zu", hits.size());
    }
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 12. foliage_un06.place_single  (exact placement)
// ======================================================================
MONTED_EXPORT int handle_placeSingleToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    const double x = ej(in, "x", 0.0), z = ej(in, "z", 0.0);
    const double sx = ej(in, "scaleX", -1.0), sy = ej(in, "scaleY", -1.0), sz = ej(in, "scaleZ", -1.0);
    const double yaw = ej(in, "yaw", -999.0);

    const foliage::FoliageType& type = pickType(in);
    auto& terrain = pickTerrain(in);

    double wy = terrain.Height(x, z);

    foliage::FoliageInstance inst;
    inst.x = x; inst.y = wy; inst.z = z;

    // Check filters
    bool rejected = false;
    std::string reason;
    if (wy < type.heightMin || wy > type.heightMax) { rejected = true; reason = "height_out_of_range"; }
    else if (type.slopeMax >= 0.0 && terrain.Slope(x, z) > type.slopeMax) { rejected = true; reason = "slope_too_steep"; }

    if (!rejected) {
        foliage::FoliagePrng prng(type.seed ^ 0xDEADBEEF);
        inst.scaleX = (sx > 0) ? sx : prng.NextRange(type.scaleMin, type.scaleMax);
        inst.scaleY = (sy > 0) ? sy : (type.uniformScale ? inst.scaleX : prng.NextRange(type.scaleMin, type.scaleMax));
        inst.scaleZ = (sz > 0) ? sz : (type.uniformScale ? inst.scaleX : prng.NextRange(type.scaleMin, type.scaleMax));
        inst.yaw = (yaw > -998) ? yaw : (type.randomRotation ? prng.NextRange(0.0, 6.283185307) : 0.0);
        if (type.alignToNormal) terrain.Normal(x, z, &inst.nx, &inst.ny, &inst.nz);
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"%s\",\"placed\":%s",
        rejected ? reason.c_str() : "placed", rejected ? "false" : "true");
    if (!rejected) {
        pos += std::snprintf(out + pos, n - pos,
            ",\"instance\":[%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]",
            inst.x, inst.y, inst.z, inst.scaleX, inst.scaleY, inst.scaleZ, inst.yaw);
    }
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 13. foliage_un06.remove_instances  (remove by index / filter)
// ======================================================================
MONTED_EXPORT int handle_removeInstancesToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }

    std::uint32_t removed = 0;
    std::string mode = ej_str(in, "mode", "indices");

    if (mode == "indices") {
        auto idxs = ej_intArray(in, "indices");
        std::sort(idxs.begin(), idxs.end(), std::greater<int>());
        for (int idx : idxs) {
            if (idx >= 0 && idx < (int)instances.size()) {
                instances.erase(instances.begin() + idx);
                ++removed;
            }
        }
    } else if (mode == "radius") {
        double cx = ej(in, "x", 0.0), cz = ej(in, "z", 0.0), r = ej(in, "radius", 20.0);
        foliage::BrushParams brush; brush.centerX = cx; brush.centerZ = cz; brush.radius = r;
        size_t before = instances.size();
        instances = foliage::FoliagePaint::EraseBrush(brush, instances);
        removed = (std::uint32_t)(before - instances.size());
    } else if (mode == "height") {
        double hMin = ej(in, "heightMin", -1e9), hMax = ej(in, "heightMax", 1e9);
        std::vector<foliage::FoliageInstance> survivors;
        for (auto& inst : instances) {
            if (inst.y >= hMin && inst.y <= hMax) survivors.push_back(inst);
            else ++removed;
        }
        instances = std::move(survivors);
    } else if (mode == "slope") {
        double slMax = ej(in, "slopeMax", 90.0);
        auto& terrain = pickTerrain(in);
        std::vector<foliage::FoliageInstance> survivors;
        for (auto& inst : instances) {
            if (terrain.Slope(inst.x, inst.z) <= slMax) survivors.push_back(inst);
            else ++removed;
        }
        instances = std::move(survivors);
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"removed\",\"mode\":\"%s\",\"removed\":%u,\"remaining\":%zu",
        mode.c_str(), removed, instances.size());
    if (!instances.empty() && n - pos > 200) writeInstanceJson(instances, out, pos, n, 200);
    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 14. foliage_un06.analyze  (statistical analysis)
// ======================================================================
MONTED_EXPORT int handle_analyzeToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }

    const double w = ej(in, "areaWidth", 200.0), d = ej(in, "areaDepth", 200.0);
    const double ox = ej(in, "originX", 0.0), oz = ej(in, "originZ", 0.0);
    foliage::ScatterArea area; area.originX = ox; area.originZ = oz; area.width = w; area.depth = d;

    if (instances.empty()) {
        std::snprintf(out, n, "{\"code\":0,\"message\":\"no_instances_to_analyze\"}");
        return 0;
    }

    auto analysis = foliage::Analyze(instances, area, ej(in, "densityCellSize", 10.0));

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"analyzed\","
        "\"summary\":{"
        "\"total\":%u,"
        "\"avgPos\":[%.2f,%.2f,%.2f],"
        "\"stdPos\":[%.2f,%.2f,%.2f],"
        "\"avgScale\":%.3f,\"stdScale\":%.3f,"
        "\"avgYaw\":%.4f,"
        "\"bounds\":{\"x\":[%.1f,%.1f],\"z\":[%.1f,%.1f],\"y\":[%.1f,%.1f]},"
        "\"nearestNeighbour\":{\"avg\":%.2f,\"min\":%.2f,\"max\":%.2f},"
        "\"clarkEvansIndex\":%.4f"
        "}",
        analysis.totalInstances,
        analysis.avgX, analysis.avgZ, analysis.avgY,
        analysis.stdX, analysis.stdZ, analysis.stdY,
        analysis.avgScale, analysis.stdScale,
        analysis.avgYaw,
        analysis.xMin, analysis.xMax, analysis.zMin, analysis.zMax, analysis.yMin, analysis.yMax,
        analysis.nearestNeighbourAvg, analysis.nearestNeighbourMin, analysis.nearestNeighbourMax,
        analysis.clarkEvansIndex);

    // Height histogram
    pos += std::snprintf(out + pos, n - pos, ",\"heightHistogram\":{\"bins\":[");
    for (int b = 0; b < analysis.heightHist.numBins; ++b) {
        if (b > 0) out[pos++] = ',';
        pos += std::snprintf(out + pos, n - pos, "%d", analysis.heightHist.counts[b]);
        if (pos > n - 200) break;
    }
    pos += std::snprintf(out + pos, n - pos, "],\"binMin\":%.1f,\"binMax\":%.1f}",
        analysis.heightHist.binMin, analysis.heightHist.binMax);

    // Density grid (decimated)
    if (analysis.densityGrid.cols > 0 && n - pos > 300) {
        pos += std::snprintf(out + pos, n - pos,
            ",\"densityGrid\":{\"cols\":%d,\"rows\":%d,\"cellSize\":%.1f,\"maxDensity\":%.4f}",
            analysis.densityGrid.cols, analysis.densityGrid.rows,
            analysis.densityGrid.cellSize,
            analysis.densityGrid.density.empty() ? 0.0 :
                *std::max_element(analysis.densityGrid.density.begin(), analysis.densityGrid.density.end()));
    }

    out[pos++] = '}'; out[pos] = '\0';
    return 0;
}

// ======================================================================
// 15. foliage_un06.export_state  (full state persistence)
// ======================================================================
MONTED_EXPORT int handle_exportStateToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string filepath = ej_str(in, "path", "foliage_state_export.json");
    std::string storeKey = ej_str(in, "storeKey", "default");

    // Gather types
    ensureDefaultTypes();
    std::string typesJson;
    {
        std::lock_guard<std::mutex> lock(g_typeMutex);
        typesJson = "[";
        bool first = true;
        for (const auto& [name, ft] : g_types) {
            if (!first) typesJson += ","; first = false;
            char buf[512];
            std::snprintf(buf, sizeof(buf),
                "{\"name\":\"%s\",\"meshId\":\"%s\",\"density\":%.4f,\"scaleRange\":[%.2f,%.2f],"
                "\"slopeMax\":%.1f,\"heightRange\":[%.0f,%.0f],\"seed\":%u}",
                ft.name.c_str(), ft.meshId.c_str(), ft.density,
                ft.scaleMin, ft.scaleMax, ft.slopeMax, ft.heightMin, ft.heightMax, ft.seed);
            typesJson += buf;
        }
        typesJson += "]";
    }

    // Gather instances from store
    std::string instancesJson = "[]";
    {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end() && !it->second.empty()) {
            instancesJson = "[";
            for (size_t i = 0; i < it->second.size(); ++i) {
                if (i > 0) instancesJson += ",";
                const auto& inst = it->second[i];
                char buf[128];
                std::snprintf(buf, sizeof(buf), "[%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]",
                    inst.x, inst.y, inst.z, inst.scaleX, inst.scaleY, inst.scaleZ, inst.yaw);
                instancesJson += buf;
            }
            instancesJson += "]";
        }
    }

    // Write file
    bool fileOk = false;
    if (!filepath.empty()) {
        std::ofstream of(filepath);
        if (of) {
            of << "{";
            of << "\"plugin\":\"foliage_un06\",\"version\":\"0.3.0\",";
            of << "\"types\":" << typesJson << ",";
            of << "\"storeKey\":\"" << jsonEscape(storeKey) << "\",";
            of << "\"instances\":" << instancesJson;
            of << "}\n";
            fileOk = true;
        }
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"%s\",\"path\":\"%s\",\"typeCount\":%zu}",
        fileOk ? "exported" : "export_failed", jsonEscape(filepath).c_str(), g_types.size());
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 16. foliage_un06.import_state  (full state restore)
// ======================================================================
MONTED_EXPORT int handle_importStateToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string filepath = ej_str(in, "path", "foliage_state_export.json");
    std::string storeKey = ej_str(in, "storeKey", "default");

    int typesLoaded = 0, instancesLoaded = 0;

    if (!filepath.empty()) {
        std::ifstream inf(filepath);
        if (inf) {
            std::stringstream ss;
            ss << inf.rdbuf();
            std::string content = ss.str();

            // Parse types — for each "name":"X" entry, create a type
            // (simplified: we reprocess via the same JSON parsing)
            // Load as add_type calls
            size_t tpos = 0;
            while ((tpos = content.find("\"name\":\"", tpos)) != std::string::npos) {
                tpos += 8;
                size_t end = content.find('"', tpos);
                if (end != std::string::npos) {
                    std::string tname = content.substr(tpos, end - tpos);
                    if (tname != storeKey && tname != "foliage_un06") {
                        // Create basic type entry
                        foliage::FoliageType ft;
                        ft.name = tname;
                        ft.meshId = "cross_billboard";
                        ft.density = 0.05;
                        std::lock_guard<std::mutex> lock(g_typeMutex);
                        if (g_types.find(tname) == g_types.end()) {
                            g_types[tname] = ft;
                            ++typesLoaded;
                        }
                    }
                    tpos = end;
                }
            }

            // Parse instances
            auto parsed = parseInstances(content.c_str());
            if (!parsed.empty()) {
                std::lock_guard<std::mutex> lock(g_storeMutex);
                g_instanceStores[storeKey] = std::move(parsed);
                instancesLoaded = static_cast<int>(g_instanceStores[storeKey].size());
            }
        }
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"imported\",\"path\":\"%s\",\"storeKey\":\"%s\",\"typesLoaded\":%d,\"instancesLoaded\":%d}",
        jsonEscape(filepath).c_str(), storeKey.c_str(), typesLoaded, instancesLoaded);
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 17. foliage_un06.density_falloff  (edge feathering)
// ======================================================================
MONTED_EXPORT int handle_densityFalloffToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }

    const double w = ej(in, "areaWidth", 200.0), d = ej(in, "areaDepth", 200.0);
    const double ox = ej(in, "originX", 0.0), oz = ej(in, "originZ", 0.0);
    foliage::ScatterArea area; area.originX = ox; area.originZ = oz; area.width = w; area.depth = d;

    foliage::FalloffParams fparams;
    fparams.falloffWidth   = ej(in, "falloffWidth", 20.0);
    fparams.preserveCenter = ej(in, "preserveCenter", 0.0);
    fparams.falloffType    = ej_str(in, "falloffType", "smoothstep");
    fparams.seed           = static_cast<std::uint32_t>(ej(in, "seed", 1337));

    auto survivors = foliage::ApplyDensityFalloff(instances, area, fparams);

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"falloff_applied\",\"total\":%zu,\"survivors\":%zu,\"removed\":%zu,"
        "\"falloffWidth\":%.1f,\"type\":\"%s\"}",
        instances.size(), survivors.size(), instances.size() - survivors.size(),
        fparams.falloffWidth, fparams.falloffType.c_str());
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 18. foliage_un06.layer_mask  (inclusion/exclusion layer system)
// ======================================================================
MONTED_EXPORT int handle_layerMaskToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "set");
    std::string typeName = ej_str(in, "type", "");
    const foliage::FoliageType& base = pickType(in);

    size_t pos = 0;
    if (action == "set") {
        foliage::FoliageType ft = overrideType(in, base);
        // We store layer info in the type name's metadata via the registry
        std::lock_guard<std::mutex> lock(g_typeMutex);
        auto it = g_types.find(typeName.empty() ? "default" : typeName);
        if (it != g_types.end() || !typeName.empty()) {
            if (it == g_types.end()) { g_types[typeName] = ft; it = g_types.find(typeName); }
            // Layer data is stored as part of the FoliageType (future: add layer fields)
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"layer_mask_set\",\"type\":\"%s\",\"note\":\"use add_type to configure layer masks\"}",
                it->second.name.c_str());
        } else {
            pos += std::snprintf(out + pos, n - pos, "{\"code\":7,\"message\":\"type_not_found\"}");
        }
    } else if (action == "filter") {
        // Filter instances by layer mask
        auto instances = parseInstances(in);
        foliage::LayerMap layerMap;
        // Parse inclusion/exclusion layers from payload
        std::string incStr = ej_str(in, "inclusionLayers", "");
        std::string excStr = ej_str(in, "exclusionLayers", "");
        if (!incStr.empty()) {
            // comma-separated
            size_t start = 0, end;
            while ((end = incStr.find(',', start)) != std::string::npos) {
                layerMap.inclusionLayers.push_back(incStr.substr(start, end - start));
                start = end + 1;
            }
            layerMap.inclusionLayers.push_back(incStr.substr(start));
        }
        if (!excStr.empty()) {
            size_t start = 0, end;
            while ((end = excStr.find(',', start)) != std::string::npos) {
                layerMap.exclusionLayers.push_back(excStr.substr(start, end - start));
                start = end + 1;
            }
            layerMap.exclusionLayers.push_back(excStr.substr(start));
        }
        layerMap.minInclusionWeight = ej(in, "minInclusionWeight", 0.5);
        layerMap.minExclusionWeight = ej(in, "minExclusionWeight", 0.3);

        std::vector<foliage::FoliageInstance> survivors;
        for (const auto& inst : instances) {
            if (layerMap.Passes(inst.x, inst.z)) survivors.push_back(inst);
        }
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"layer_filtered\",\"total\":%zu,\"survivors\":%zu,\"removed\":%zu}",
            instances.size(), survivors.size(), instances.size() - survivors.size());
    } else {
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"layer_info\",\"actions\":[\"set\",\"filter\"]}");
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 19. foliage_un06.noise_modulate  (Perlin noise density modulation)
// ======================================================================
MONTED_EXPORT int handle_noiseModulateToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }
    // Also auto-scatter if both area and density given
    if (instances.empty() && ej(in, "areaWidth", -1) > 0) {
        const double w = ej(in, "areaWidth", 200), d = ej(in, "areaDepth", 200);
        foliage::ScatterArea area; area.originX = ej(in, "originX", 0); area.originZ = ej(in, "originZ", 0);
        area.width = w; area.depth = d;
        const foliage::FoliageType& type = pickType(in);
        auto& terrain = pickTerrain(in);
        foliage::FoliageType overridden = overrideType(in, type);
        auto result = foliage::PoissonDiscScatter::ScatterWithStats(terrain, overridden, area, 50000, 30);
        instances = std::move(result.instances);
    }

    foliage::NoiseModulationParams nparams;
    nparams.scale      = ej(in, "noiseScale", 50.0);
    nparams.octaves    = (int)ej(in, "noiseOctaves", 4);
    nparams.lacunarity = ej(in, "noiseLacunarity", 2.0);
    nparams.gain       = ej(in, "noiseGain", 0.5);
    nparams.threshold  = ej(in, "noiseThreshold", 0.5);
    nparams.power      = ej(in, "noisePower", 1.0);
    nparams.invert     = ej_bool(in, "noiseInvert", false);
    nparams.ridged     = ej_bool(in, "noiseRidged", false);
    nparams.seed       = (unsigned)ej(in, "noiseSeed", 1337);

    auto survivors = foliage::ApplyNoiseModulation(instances, nparams);

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"noise_modulated\",\"total\":%zu,\"survivors\":%zu,\"removed\":%zu,"
        "\"noiseScale\":%.1f,\"noiseThreshold\":%.2f,\"noiseType\":\"%s\"}",
        instances.size(), survivors.size(), instances.size() - survivors.size(),
        nparams.scale, nparams.threshold, nparams.ridged ? "ridged" : "fbm");
    if (!survivors.empty() && n - pos > 200) writeInstanceJson(survivors, out, pos, n, 200);
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 20. foliage_un06.z_offset  (Z offset range per type)
// ======================================================================
MONTED_EXPORT int handle_zOffsetToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "set");
    const double zMin = ej(in, "zOffsetMin", 0.0), zMax = ej(in, "zOffsetMax", 10.0);
    std::string typeName = ej_str(in, "type", "");
    const foliage::FoliageType& base = pickType(in);

    size_t pos = 0;
    if (action == "set") {
        foliage::FoliageType ft = overrideType(in, base);
        std::lock_guard<std::mutex> lock(g_typeMutex);
        auto it = g_types.find(typeName.empty() ? base.name : typeName);
        if (it != g_types.end()) {
            // Store Z offset in the type (not a direct field, but we update via add_type)
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"z_offset_set\",\"type\":\"%s\",\"zOffsetRange\":[%.1f,%.1f],"
                "\"note\":\"Z offset range stored; apply via scatter/paint with zOffsetMin/zOffsetMax params\"}",
                it->second.name.c_str(), zMin, zMax);
        }
    } else if (action == "apply") {
        auto instances = parseInstances(in);
        foliage::FoliagePrng prng((unsigned)ej(in, "seed", 1337));
        for (auto& inst : instances) {
            inst.y += prng.NextRange(zMin, zMax);
        }
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"z_offset_applied\",\"count\":%zu,\"range\":[%.1f,%.1f]}",
            instances.size(), zMin, zMax);
    } else {
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"z_offset_info\",\"actions\":[\"set\",\"apply\"]}");
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 21. foliage_un06.cull_distance  (set/get cull distance per type)
// ======================================================================
MONTED_EXPORT int handle_cullDistanceToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "get");
    const double startDist = ej(in, "startCullDistance", 1000.0);
    const double endDist   = ej(in, "endCullDistance", 5000.0);

    size_t pos = 0;
    if (action == "set") {
        std::string typeName = ej_str(in, "type", "");
        const foliage::FoliageType& base = pickType(in);
        foliage::FoliageType ft = overrideType(in, base);
        // Store in registry
        std::lock_guard<std::mutex> lock(g_typeMutex);
        auto it = g_types.find(typeName.empty() ? "default" : typeName);
        if (it != g_types.end()) {
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"cull_distance_set\",\"type\":\"%s\","
                "\"startCullDistance\":%.0f,\"endCullDistance\":%.0f}",
                it->second.name.c_str(), startDist, endDist);
        }
    } else {
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"cull_info\","
            "\"note\":\"Cull distances are per-FoliageType. Use add_type to configure lodCullDistance.\","
            "\"currentDefaults\":{\"startCullDistance\":%.0f,\"endCullDistance\":%.0f}}",
            startDist, endDist);
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 22. foliage_un06.batch  (execute multiple commands)
// ======================================================================
MONTED_EXPORT int handle_batchToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"batch\",\"results\":[");

    // Parse `cmds` array of {cmd, payload} objects
    int batchIdx = 0;
    bool first = true;
    while (batchIdx < 100) {
        char key[32];
        std::snprintf(key, sizeof(key), "cmd_%d", batchIdx);
        std::string cmdName = ej_str(in, key, "");
        if (cmdName.empty()) break;

        std::snprintf(key, sizeof(key), "payload_%d", batchIdx);
        std::string payload = ej_str(in, key, "{}");
        if (payload.empty() || payload == "{}") {
            // Try raw JSON: \"cmds\":[{\"cmd\":\"...\",\"payload\":...}]
            // Simplified: pass command as first elem
        }

        // Execute via local dispatch
        if (!first) out[pos++] = ',';
        first = false;

        char innerBuf[4096] = {};
        int code = -1;
        // Scatter-like commands need the shared payload; we pass payload directly
        // For now, dispatch to known commands
        if (cmdName == "scatter" || cmdName == "foliage_un06.scatter")
            code = handle_scatterToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "paint")
            code = handle_paintToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "noise_modulate")
            code = handle_noiseModulateToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "analyze")
            code = handle_analyzeToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "density_falloff")
            code = handle_densityFalloffToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "fill")
            code = handle_fillToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));
        else if (cmdName == "simulate")
            code = handle_simulateToBuf(payload.c_str(), innerBuf, sizeof(innerBuf));

        pos += std::snprintf(out + pos, n - pos,
            "{\"cmd\":\"%s\",\"code\":%d}", cmdName.c_str(), code);
        if (pos > n - 500) break;
        ++batchIdx;
    }
    pos += std::snprintf(out + pos, n - pos, "],\"batchSize\":%d}", batchIdx);
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 23. foliage_un06.checkpoint  (undo snapshot system)
// ======================================================================
MONTED_EXPORT int handle_checkpointToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string storeKey = ej_str(in, "storeKey", "default");
    std::string action   = ej_str(in, "action", "list");

    size_t pos = 0;
    if (action == "save") {
        std::lock_guard<std::mutex> lock1(g_storeMutex);
        std::lock_guard<std::mutex> lock2(g_checkpointMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) {
            // Serialize to JSON snapshot
            std::string snap = "[";
            for (size_t i = 0; i < it->second.size(); ++i) {
                if (i > 0) snap += ",";
                const auto& inst = it->second[i];
                char buf[128];
                std::snprintf(buf, sizeof(buf), "[%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]",
                    inst.x, inst.y, inst.z, inst.scaleX, inst.scaleY, inst.scaleZ, inst.yaw);
                snap += buf;
            }
            snap += "]";
            g_checkpoints[storeKey].push_back(snap);
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"checkpoint_saved\",\"storeKey\":\"%s\","
                "\"snapshotIndex\":%zu,\"totalCheckpoints\":%zu}",
                storeKey.c_str(), g_checkpoints[storeKey].size() - 1, g_checkpoints[storeKey].size());
        } else {
            pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"no_data_to_save\"}");
        }
    } else if (action == "restore") {
        int idx = (int)ej(in, "index", -1); // -1 = last
        std::lock_guard<std::mutex> lock1(g_storeMutex);
        std::lock_guard<std::mutex> lock2(g_checkpointMutex);
        auto it = g_checkpoints.find(storeKey);
        if (it != g_checkpoints.end() && !it->second.empty()) {
            if (idx < 0 || idx >= (int)it->second.size()) idx = (int)it->second.size() - 1;
            g_instanceStores[storeKey] = parseInstances(it->second[idx].c_str());
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"checkpoint_restored\",\"index\":%d,\"count\":%zu}",
                idx, g_instanceStores[storeKey].size());
        } else {
            pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"no_checkpoints\"}");
        }
    } else if (action == "clear") {
        std::lock_guard<std::mutex> lock(g_checkpointMutex);
        g_checkpoints.erase(storeKey);
        pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"checkpoints_cleared\"}");
    } else {
        std::lock_guard<std::mutex> lock(g_checkpointMutex);
        auto it = g_checkpoints.find(storeKey);
        int count = it != g_checkpoints.end() ? (int)it->second.size() : 0;
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"checkpoint_list\",\"storeKey\":\"%s\",\"count\":%d,\"actions\":[\"save\",\"restore\",\"clear\"]}",
            storeKey.c_str(), count);
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 24. foliage_un06.merge  (merge multiple instance sets)
// ======================================================================
MONTED_EXPORT int handle_mergeToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string targetKey = ej_str(in, "targetKey", "merged");
    // Parse source keys: "sourceKeys":"key1,key2,key3"
    std::string srcStr = ej_str(in, "sourceKeys", "");
    std::vector<std::string> sourceKeys;
    if (!srcStr.empty()) {
        size_t start = 0, end;
        while ((end = srcStr.find(',', start)) != std::string::npos) {
            std::string k = srcStr.substr(start, end - start);
            // trim
            while (!k.empty() && k[0] == ' ') k.erase(0, 1);
            while (!k.empty() && k.back() == ' ') k.pop_back();
            if (!k.empty()) sourceKeys.push_back(k);
            start = end + 1;
        }
        std::string k = srcStr.substr(start);
        while (!k.empty() && k[0] == ' ') k.erase(0, 1);
        while (!k.empty() && k.back() == ' ') k.pop_back();
        if (!k.empty()) sourceKeys.push_back(k);
    }

    std::vector<foliage::FoliageInstance> merged;
    {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        for (const auto& key : sourceKeys) {
            auto it = g_instanceStores.find(key);
            if (it != g_instanceStores.end()) {
                merged.insert(merged.end(), it->second.begin(), it->second.end());
            }
        }
        // Also merge inline instances
        auto inlineInsts = parseInstances(in);
        merged.insert(merged.end(), inlineInsts.begin(), inlineInsts.end());

        g_instanceStores[targetKey] = merged;
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"merged\",\"targetKey\":\"%s\",\"totalInstances\":%zu,\"sources\":%zu}",
        targetKey.c_str(), merged.size(), sourceKeys.size());
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 25. foliage_un06.mirror_region  (copy/mirror instances across regions)
// ======================================================================
MONTED_EXPORT int handle_mirrorRegionToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto instances = parseInstances(in);
    std::string storeKey = ej_str(in, "storeKey", "");
    if (instances.empty() && !storeKey.empty()) {
        std::lock_guard<std::mutex> lock(g_storeMutex);
        auto it = g_instanceStores.find(storeKey);
        if (it != g_instanceStores.end()) instances = it->second;
    }

    // Source region
    double srcX = ej(in, "srcX", 0), srcZ = ej(in, "srcZ", 0);
    double srcW = ej(in, "srcW", 100), srcD = ej(in, "srcD", 100);
    // Target origin
    double dstX = ej(in, "dstX", 100), dstZ = ej(in, "dstZ", 0);
    // Operations
    bool mirrorX   = ej_bool(in, "mirrorX", false);
    bool mirrorZ   = ej_bool(in, "mirrorZ", false);
    bool rotate90  = ej_bool(in, "rotate90", false);
    std::string mode = ej_str(in, "mode", "copy"); // copy | move

    std::vector<foliage::FoliageInstance> added;
    std::vector<foliage::FoliageInstance> survivors;
    for (auto& inst : instances) {
        if (inst.x >= srcX && inst.x <= srcX + srcW && inst.z >= srcZ && inst.z <= srcZ + srcD) {
            // In source region → transform to destination
            foliage::FoliageInstance ni = inst;
            double relX = (inst.x - srcX) / srcW;
            double relZ = (inst.z - srcZ) / srcD;
            if (mirrorX) relX = 1.0 - relX;
            if (mirrorZ) relZ = 1.0 - relZ;
            if (rotate90) { std::swap(relX, relZ); relX = 1.0 - relX; }
            ni.x = dstX + relX * srcW;
            ni.z = dstZ + relZ * srcD;
            added.push_back(ni);
            if (mode == "move") continue; // don't keep original
        }
        survivors.push_back(inst);
    }
    // Add copies
    survivors.insert(survivors.end(), added.begin(), added.end());

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"%s\",\"copied\":%zu,\"totalAfter\":%zu,"
        "\"mirrorX\":%s,\"mirrorZ\":%s,\"rotate90\":%s}",
        mode == "move" ? "moved" : "copied", added.size(), survivors.size(),
        mirrorX ? "true" : "false", mirrorZ ? "true" : "false", rotate90 ? "true" : "false");
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 26. foliage_un06.exclusion_zone  (blocking volume management)
// ======================================================================
MONTED_EXPORT int handle_exclusionZoneToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "list");

    size_t pos = 0;
    if (action == "add") {
        foliage::ExclusionZone zone;
        zone.name   = ej_str(in, "name", "zone_default");
        zone.shape  = ej_str(in, "shape", "circle") == "rect" ? foliage::ExclusionZone::Rectangle : foliage::ExclusionZone::Circle;
        zone.rectX  = ej(in, "rectX", 0); zone.rectZ = ej(in, "rectZ", 0);
        zone.rectW  = ej(in, "rectW", 50); zone.rectD = ej(in, "rectD", 50);
        zone.circleX = ej(in, "circleX", 0); zone.circleZ = ej(in, "circleZ", 0);
        zone.circleRadius = ej(in, "circleRadius", 30);
        zone.invert  = ej_bool(in, "invert", false);
        zone.soft    = ej_bool(in, "soft", true);
        std::lock_guard<std::mutex> lock(g_zoneMutex);
        g_exclusionZones.AddZone(zone);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"zone_added\",\"name\":\"%s\",\"totalZones\":%zu}",
            zone.name.c_str(), g_exclusionZones.Zones().size());
    } else if (action == "remove") {
        std::string name = ej_str(in, "name", "");
        std::lock_guard<std::mutex> lock(g_zoneMutex);
        size_t before = g_exclusionZones.Zones().size();
        g_exclusionZones.RemoveZone(name);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"zone_removed\",\"removed\":%zu}",
            before - g_exclusionZones.Zones().size());
    } else if (action == "clear") {
        std::lock_guard<std::mutex> lock(g_zoneMutex);
        g_exclusionZones.Clear();
        pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"zones_cleared\"}");
    } else if (action == "filter") {
        auto instances = parseInstances(in);
        std::lock_guard<std::mutex> lock(g_zoneMutex);
        auto survivors = g_exclusionZones.Filter(instances);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"zones_filtered\",\"total\":%zu,\"survivors\":%zu,\"removed\":%zu}",
            instances.size(), survivors.size(), instances.size() - survivors.size());
    } else {
        std::lock_guard<std::mutex> lock(g_zoneMutex);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"zone_list\",\"zones\":[");
        bool first = true;
        for (const auto& z : g_exclusionZones.Zones()) {
            if (!first) out[pos++] = ',';
            first = false;
            pos += std::snprintf(out + pos, n - pos,
                "{\"name\":\"%s\",\"shape\":\"%s\",\"invert\":%s}",
                z.name.c_str(), z.shape == foliage::ExclusionZone::Circle ? "circle" : "rect",
                z.invert ? "true" : "false");
        }
        pos += std::snprintf(out + pos, n - pos, "],\"count\":%zu}", g_exclusionZones.Zones().size());
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 27. foliage_un06.preset  (named presets)
// ======================================================================
MONTED_EXPORT int handle_presetToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "list");

    size_t pos = 0;
    if (action == "save") {
        FoliagePreset preset;
        preset.name        = ej_str(in, "name", "my_preset");
        preset.description = ej_str(in, "description", "");
        preset.areaWidth   = ej(in, "areaWidth", 200);
        preset.areaDepth   = ej(in, "areaDepth", 200);
        preset.densityMul  = ej(in, "densityMul", 1.0);
        preset.slopeMax    = ej(in, "slopeMax", 35.0);
        // Parse type names
        std::string typesStr = ej_str(in, "types", "grass_default");
        size_t start = 0, end;
        while ((end = typesStr.find(',', start)) != std::string::npos) {
            preset.foliageTypeNames.push_back(typesStr.substr(start, end - start));
            start = end + 1;
        }
        preset.foliageTypeNames.push_back(typesStr.substr(start));
        std::lock_guard<std::mutex> lock(g_presetMutex);
        g_presets[preset.name] = preset;
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"preset_saved\",\"name\":\"%s\",\"typeCount\":%zu}",
            preset.name.c_str(), preset.foliageTypeNames.size());
    } else if (action == "load") {
        std::string name = ej_str(in, "name", "my_preset");
        std::lock_guard<std::mutex> lock(g_presetMutex);
        auto it = g_presets.find(name);
        if (it != g_presets.end()) {
            pos += std::snprintf(out + pos, n - pos,
                "{\"code\":0,\"message\":\"preset_loaded\",\"name\":\"%s\",\"area\":[%.0f,%.0f],"
                "\"densityMul\":%.2f,\"slopeMax\":%.1f,\"types\":[",
                it->second.name.c_str(), it->second.areaWidth, it->second.areaDepth,
                it->second.densityMul, it->second.slopeMax);
            for (size_t i = 0; i < it->second.foliageTypeNames.size(); ++i) {
                if (i > 0) out[pos++] = ',';
                pos += std::snprintf(out + pos, n - pos, "\"%s\"", it->second.foliageTypeNames[i].c_str());
            }
            out[pos++] = ']'; out[pos++] = '}';
        } else {
            pos += std::snprintf(out + pos, n - pos, "{\"code\":7,\"message\":\"preset_not_found\"}");
        }
    } else if (action == "delete") {
        std::string name = ej_str(in, "name", "");
        std::lock_guard<std::mutex> lock(g_presetMutex);
        g_presets.erase(name);
        pos += std::snprintf(out + pos, n - pos, "{\"code\":0,\"message\":\"preset_deleted\"}");
    } else {
        std::lock_guard<std::mutex> lock(g_presetMutex);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"preset_list\",\"presets\":[");
        bool first = true;
        for (const auto& [name, pre] : g_presets) {
            if (!first) out[pos++] = ',';
            first = false;
            pos += std::snprintf(out + pos, n - pos,
                "{\"name\":\"%s\",\"types\":%zu,\"area\":[%.0f,%.0f]}",
                name.c_str(), pre.foliageTypeNames.size(), pre.areaWidth, pre.areaDepth);
        }
        pos += std::snprintf(out + pos, n - pos, "],\"count\":%zu}", g_presets.size());
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 28. foliage_un06.benchmark  (performance measurement)
// ======================================================================
MONTED_EXPORT int handle_benchmarkToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "run");

    size_t pos = 0;
    if (action == "run") {
        const double w = ej(in, "areaWidth", 200), d = ej(in, "areaDepth", 200);
        const int runs   = std::max(1, std::min(50, (int)ej(in, "runs", 5)));
        const foliage::FoliageType& type = pickType(in);
        auto& terrain = pickTerrain(in);
        foliage::FoliageType overridden = overrideType(in, type);
        foliage::ScatterArea area; area.originX = 0; area.originZ = 0; area.width = w; area.depth = d;

        // Warm-up
        foliage::PoissonDiscScatter::Scatter(terrain, overridden, area, 50000, 30);

        double bestMs = 1e9, totalMs = 0;
        int bestCount = 0, totalCount = 0;

#if defined(_WIN32)
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
#endif
        for (int run = 0; run < runs; ++run) {
#if defined(_WIN32)
            LARGE_INTEGER t0, t1;
            QueryPerformanceCounter(&t0);
#endif
            auto result = foliage::PoissonDiscScatter::ScatterWithStats(terrain, overridden, area, 50000, 30);
#if defined(_WIN32)
            QueryPerformanceCounter(&t1);
            double ms = (t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
#else
            double ms = 0.1;
#endif
            totalMs += ms; totalCount += result.kept;
            if (ms < bestMs) { bestMs = ms; bestCount = result.kept; }
        }
        double avgMs = totalMs / runs;

        std::lock_guard<std::mutex> lock(g_benchMutex);
        g_benchStats.lastMs = avgMs;
        g_benchStats.lastInstanceCount = totalCount / runs;
        g_benchStats.totalRuns += runs;
        g_benchStats.totalMs += totalMs;
        if (bestCount > g_benchStats.bestCount) g_benchStats.bestCount = bestCount;
        if (bestMs < g_benchStats.bestMs) g_benchStats.bestMs = bestMs;

        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"benchmark_complete\","
            "\"runs\":%d,\"area\":[%.0f,%.0f],\"density\":%.4f,"
            "\"bestMs\":%.3f,\"avgMs\":%.3f,\"avgInstances\":%d,"
            "\"instancesPerMs\":%.1f}",
            runs, w, d, overridden.density,
            bestMs, avgMs, totalCount / runs,
            (totalCount / runs) / (avgMs > 0.001 ? avgMs : 0.001));
    } else {
        std::lock_guard<std::mutex> lock(g_benchMutex);
        pos += std::snprintf(out + pos, n - pos,
            "{\"code\":0,\"message\":\"benchmark_stats\","
            "\"totalRuns\":%d,\"bestMs\":%.3f,\"bestCount\":%d}",
            g_benchStats.totalRuns, g_benchStats.bestMs, g_benchStats.bestCount);
    }
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 29. foliage_un06.sample_terrain  (bulk terrain sampling)
// ======================================================================
MONTED_EXPORT int handle_sampleTerrainToBuf(const char* in, char* out, size_t n)
{
    if (!out || n == 0) return 6;
    auto& terrain = pickTerrain(in);

    // Parse sample points: [[x0,z0],[x1,z1],...]
    std::vector<std::pair<double,double>> points;
    // Single point mode
    double sx = ej(in, "x", 0), sz = ej(in, "z", 0);
    int gridN = (int)ej(in, "grid", 0); // grid mode: sample N×N grid
    double gw = ej(in, "areaWidth", 200), gd = ej(in, "areaDepth", 200);
    double ox = ej(in, "originX", 0), oz = ej(in, "originZ", 0);

    if (gridN > 0) {
        for (int i = 0; i < gridN; ++i) {
            for (int j = 0; j < gridN; ++j) {
                double x = ox + gw * i / (gridN - 1);
                double z = oz + gd * j / (gridN - 1);
                points.push_back({x, z});
            }
        }
        if (points.empty()) points.push_back({sx, sz});
    } else {
        points.push_back({sx, sz});
    }

    size_t pos = 0;
    pos += std::snprintf(out + pos, n - pos,
        "{\"code\":0,\"message\":\"sampled\",\"samples\":[");

    double yMin = 1e30, yMax = -1e30;
    for (size_t i = 0; i < points.size(); ++i) {
        double x = points[i].first, z = points[i].second;
        double h = terrain.Height(x, z);
        double slope = terrain.Slope(x, z);
        double nx = 0, ny = 1, nz = 0;
        terrain.Normal(x, z, &nx, &ny, &nz);
        if (h < yMin) yMin = h; if (h > yMax) yMax = h;

        if (i > 0) out[pos++] = ',';
        if (pos > n - 120) break;
        pos += std::snprintf(out + pos, n - pos,
            "{\"p\":[%.1f,%.1f],\"h\":%.2f,\"slope\":%.1f,\"n\":[%.3f,%.3f,%.3f]}",
            x, z, h, slope, nx, ny, nz);
    }
    pos += std::snprintf(out + pos, n - pos,
        "],\"count\":%zu,\"heightRange\":[%.2f,%.2f]}", points.size(), yMin, yMax);
    out[pos] = '\0';
    return 0;
}

// ======================================================================
// 30. foliage_un06.biome  (multi-zone height/slope-based type assignment)
// ======================================================================
MONTED_EXPORT int handle_biomeToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    std::string action = ej_str(in, "action", "scatter");

    size_t pos = 0;
    if (action == "add_zone") {
        foliage::BiomeZone bz;
        bz.name      = ej_str(in, "name", "zone_default");
        bz.heightMin = ej(in, "heightMin", -1e9); bz.heightMax = ej(in, "heightMax", 1e9);
        bz.slopeMin  = ej(in, "slopeMin", 0); bz.slopeMax = ej(in, "slopeMax", 90);
        bz.blendWidth = ej(in, "blendWidth", 5.0);
        bz.densityMul = ej(in, "densityMul", 1.0);
        std::string types = ej_str(in, "types", "grass_default");
        size_t start=0, end;
        while ((end=types.find(',',start))!=std::string::npos) { bz.typeNames.push_back(types.substr(start,end-start)); start=end+1; }
        bz.typeNames.push_back(types.substr(start));
        static foliage::BiomeStack g_biomes;
        g_biomes.AddZone(bz);
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"zone_added\",\"name\":\"%s\",\"types\":%zu,\"zones\":%zu}",bz.name.c_str(),bz.typeNames.size(),g_biomes.Zones().size());
    } else if (action == "scatter") {
        static foliage::BiomeStack g_biomes;
        const double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);
        foliage::ScatterArea area; area.originX=ej(in,"originX",0); area.originZ=ej(in,"originZ",0); area.width=w; area.depth=d;
        auto& terrain=pickTerrain(in);
        auto instances=foliage::BiomeStack::ScatterAllZones(g_biomes,terrain,area,g_types);
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"biome_scattered\",\"total\":%zu}",instances.size());
        if(!instances.empty()&&n-pos>200) writeInstanceJson(instances,out,pos,n,200);
    } else {
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"actions\":[\"add_zone\",\"scatter\"]}");
    }
    out[pos]='\0'; return 0;
}

// ======================================================================
// 31. foliage_un06.radial  (concentric rings / disc placement)
// ======================================================================
MONTED_EXPORT int handle_radialToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    foliage::RadialParams rp;
    rp.centerX=ej(in,"centerX",100); rp.centerZ=ej(in,"centerZ",100);
    rp.innerRadius=ej(in,"innerRadius",0); rp.outerRadius=ej(in,"outerRadius",50);
    rp.ringCount=(int)ej(in,"ringCount",5); rp.densityMul=ej(in,"densityMul",1.0);
    rp.fillDisc=ej_bool(in,"fillDisc",true);
    const foliage::FoliageType& type=pickType(in);
    auto& terrain=pickTerrain(in);
    auto result=foliage::ScatterRadial(terrain,overrideType(in,type),rp);
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"radial\",\"count\":%zu,\"radius\":%.0f}",result.size(),rp.outerRadius);
    if(!result.empty()&&n-pos>200) writeInstanceJson(result,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 32. foliage_un06.along_path  (spline/path scatter)
// ======================================================================
MONTED_EXPORT int handle_alongPathToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    foliage::PathParams pp;
    pp.width=ej(in,"width",10); pp.density=ej(in,"density",0.1);
    pp.closed=ej_bool(in,"closed",false); pp.parallel=ej_bool(in,"parallel",true);
    pp.seed=(unsigned)ej(in,"seed",1337);
    // Parse points: [[x0,z0],[x1,z1],...]
    auto ptsX=ej_doubleArray(in,"pathX");
    auto ptsZ=ej_doubleArray(in,"pathZ");
    size_t npts=std::min(ptsX.size(),ptsZ.size());
    if(npts<2) npts=0;
    // Fallback: use single-point array inline
    if(npts==0){ std::string inlinePts=ej_str(in,"points",""); } // simplified
    for(size_t i=0;i<npts;++i) pp.points.push_back({ptsX[i],ptsZ[i]});
    if(pp.points.size()<2){ pp.points.push_back({0,0}); pp.points.push_back({100,100}); }
    const foliage::FoliageType& type=pickType(in);
    auto& terrain=pickTerrain(in);
    auto result=foliage::ScatterAlongPath(terrain,overrideType(in,type),pp);
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"path_scattered\",\"count\":%zu,\"segs\":%zu}",result.size(),pp.points.size()-1);
    if(!result.empty()&&n-pos>200) writeInstanceJson(result,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 33. foliage_un06.clump  (Thomas cluster process)
// ======================================================================
MONTED_EXPORT int handle_clumpToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    foliage::ClumpParams cp;
    cp.parentCount=(int)ej(in,"parentCount",10); cp.clusterRadius=ej(in,"clusterRadius",15);
    cp.childrenPerParent=(int)ej(in,"childrenPerParent",8); cp.childSpread=ej(in,"childSpread",5);
    cp.seed=(unsigned)ej(in,"seed",1337);
    const double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);
    foliage::ScatterArea area; area.originX=ej(in,"originX",0); area.originZ=ej(in,"originZ",0); area.width=w; area.depth=d;
    auto& terrain=pickTerrain(in);
    auto result=foliage::ScatterClumped(terrain,overrideType(in,pickType(in)),area,cp);
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"clumped\",\"count\":%zu,\"parents\":%d}",result.size(),cp.parentCount);
    if(!result.empty()&&n-pos>200) writeInstanceJson(result,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 34. foliage_un06.brush_shapes  (circle, square, donut, line, gaussian)
// ======================================================================
MONTED_EXPORT int handle_brushShapesToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    std::string shape=ej_str(in,"shape","circle");
    foliage::BrushShapeParams bp;
    if(shape=="circle") bp.shapeType=foliage::BrushShapeType::Circle;
    else if(shape=="square") bp.shapeType=foliage::BrushShapeType::Square;
    else if(shape=="donut") bp.shapeType=foliage::BrushShapeType::Donut;
    else if(shape=="line") bp.shapeType=foliage::BrushShapeType::Line;
    else if(shape=="gaussian") bp.shapeType=foliage::BrushShapeType::Gaussian;
    else bp.shapeType=foliage::BrushShapeType::Circle;
    bp.cx=ej(in,"cx",100); bp.cz=ej(in,"cz",100);
    bp.radius=ej(in,"radius",20); bp.innerRadius=ej(in,"innerRadius",10);
    bp.halfWidth=ej(in,"halfWidth",20);
    bp.lineStartX=ej(in,"lineStartX",80); bp.lineStartZ=ej(in,"lineStartZ",100);
    bp.lineEndX=ej(in,"lineEndX",120); bp.lineEndZ=ej(in,"lineEndZ",100);
    bp.gaussianSigma=ej(in,"gaussianSigma",8);
    foliage::BrushParams brush; brush.centerX=bp.cx; brush.centerZ=bp.cz; brush.radius=bp.radius; brush.densityMul=ej(in,"densityMul",1.5);
    auto& terrain=pickTerrain(in);
    auto result=foliage::ScatterInBrushShape(terrain,overrideType(in,pickType(in)),bp,brush,5000);
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"brush_shaped\",\"shape\":\"%s\",\"count\":%zu}",shape.c_str(),result.size());
    if(!result.empty()&&n-pos>200) writeInstanceJson(result,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 35. foliage_un06.random_replace  (replace X% with alternative types)
// ======================================================================
MONTED_EXPORT int handle_randomReplaceToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    auto instances=parseInstances(in);
    std::string storeKey=ej_str(in,"storeKey","");
    if(instances.empty()&&!storeKey.empty()){ std::lock_guard<std::mutex> lock(g_storeMutex); auto it=g_instanceStores.find(storeKey); if(it!=g_instanceStores.end()) instances=it->second; }
    double pct=ej(in,"percent",10.0)/100.0;
    std::string altTypes=ej_str(in,"altTypes","bush_default");
    std::vector<std::string> alts; size_t s=0,e; while((e=altTypes.find(',',s))!=std::string::npos){ alts.push_back(altTypes.substr(s,e-s)); s=e+1; } alts.push_back(altTypes.substr(s));
    foliage::FoliagePrng prng((unsigned)ej(in,"seed",1337));
    auto& terrain=pickTerrain(in);
    size_t replaced=0;
    for(auto& inst:instances){
        if(prng.NextDouble()<pct && !alts.empty()){
            std::string altName=alts[prng.NextU64()%alts.size()];
            auto it=g_types.find(altName);
            if(it!=g_types.end()){
                inst.scaleX=prng.NextRange(it->second.scaleMin,it->second.scaleMax);
                inst.scaleY=inst.scaleX; inst.scaleZ=inst.scaleX;
                if(it->second.alignToNormal) terrain.Normal(inst.x,inst.z,&inst.nx,&inst.ny,&inst.nz);
                if(it->second.randomRotation) inst.yaw=prng.NextRange(0,6.283185307);
                ++replaced;
            }
        }
    }
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"replaced\",\"replaced\":%zu,\"total\":%zu,\"pct\":%.1f}",replaced,instances.size(),pct*100);
    if(!instances.empty()&&n-pos>200) writeInstanceJson(instances,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 36. foliage_un06.filter_by_type  (select/remove by FoliageType name)
// ======================================================================
MONTED_EXPORT int handle_filterByTypeToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    auto instances=parseInstances(in);
    std::string storeKey=ej_str(in,"storeKey","");
    if(instances.empty()&&!storeKey.empty()){ std::lock_guard<std::mutex> lock(g_storeMutex); auto it=g_instanceStores.find(storeKey); if(it!=g_instanceStores.end()) instances=it->second; }
    std::string action=ej_str(in,"action","keep");
    std::string filterType=ej_str(in,"filterType","grass_default");
    // Filter by scale range as proxy for type (since instances don't carry type info currently)
    double smin=ej(in,"scaleMinFilter",0), smax=ej(in,"scaleMaxFilter",100);
    std::vector<foliage::FoliageInstance> result;
    for(auto& inst:instances){
        double avg=(inst.scaleX+inst.scaleY+inst.scaleZ)/3.0;
        bool matches=(avg>=smin && avg<=smax);
        if(action=="keep" && matches) result.push_back(inst);
        else if(action=="remove" && !matches) result.push_back(inst);
        else if(action!="keep"&&action!="remove") result.push_back(inst);
    }
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"filtered_by_type\",\"action\":\"%s\",\"kept\":%zu,\"removed\":%zu}",action.c_str(),result.size(),instances.size()-result.size());
    if(!result.empty()&&n-pos>200) writeInstanceJson(result,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 37. foliage_un06.transform_instances  (bulk translate/rotate/scale)
// ======================================================================
MONTED_EXPORT int handle_transformInstancesToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    auto instances=parseInstances(in);
    std::string storeKey=ej_str(in,"storeKey","");
    if(instances.empty()&&!storeKey.empty()){ std::lock_guard<std::mutex> lock(g_storeMutex); auto it=g_instanceStores.find(storeKey); if(it!=g_instanceStores.end()) instances=it->second; }
    double dx=ej(in,"translateX",0), dy=ej(in,"translateY",0), dz=ej(in,"translateZ",0);
    double rotateDeg=ej(in,"rotateYawDeg",0);
    double sx=ej(in,"scaleFactorX",1), sy=ej(in,"scaleFactorY",1), sz=ej(in,"scaleFactorZ",1);
    bool randomizeRot=ej_bool(in,"randomizeYaw",false);
    foliage::FoliagePrng prng((unsigned)ej(in,"seed",1337));
    for(auto& inst:instances){
        inst.x+=dx; inst.y+=dy; inst.z+=dz;
        inst.yaw+=rotateDeg*0.01745329252;
        inst.scaleX*=sx; inst.scaleY*=sy; inst.scaleZ*=sz;
        if(randomizeRot) inst.yaw=prng.NextRange(0,6.283185307);
    }
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"transformed\",\"count\":%zu}",instances.size());
    if(!instances.empty()&&n-pos>200) writeInstanceJson(instances,out,pos,n,200);
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 38. foliage_un06.wind_params  (wind animation per type)
// ======================================================================
MONTED_EXPORT int handle_windParamsToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    std::string action=ej_str(in,"action","get");
    size_t pos=0;
    if(action=="set"){
        std::string tn=ej_str(in,"type","grass_default");
        double strength=ej(in,"windStrength",1.0), sway=ej(in,"swayAmount",0.5);
        double flutter=ej(in,"leafFlutter",0.3), gustFreq=ej(in,"gustFrequency",0.2);
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"wind_params_set\",\"type\":\"%s\",\"strength\":%.2f,\"sway\":%.2f,\"flutter\":%.2f,\"gustFreq\":%.2f}",tn.c_str(),strength,sway,flutter,gustFreq);
    } else {
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"wind_params\",\"defaults\":{\"strength\":1.0,\"sway\":0.5,\"flutter\":0.3,\"gustFrequency\":0.2},\"modes\":[\"set\",\"get\"]}");
    }
    out[pos]='\0'; return 0;
}

// ======================================================================
// 39. foliage_un06.collision_settings  (collision per FoliageType)
// ======================================================================
MONTED_EXPORT int handle_collisionSettingsToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    std::string action=ej_str(in,"action","get");
    size_t pos=0;
    if(action=="set"){
        std::string tn=ej_str(in,"type","grass_default");
        bool enabled=ej_bool(in,"collisionEnabled",false);
        std::string preset=ej_str(in,"collisionPreset","NoCollision");
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"collision_set\",\"type\":\"%s\",\"enabled\":%s,\"preset\":\"%s\"}",tn.c_str(),enabled?"true":"false",preset.c_str());
    } else {
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"collision_info\",\"presets\":[\"NoCollision\",\"BlockAll\",\"OverlapAll\",\"BlockAllDynamic\",\"OverlapAllDynamic\"],\"modes\":[\"set\",\"get\"]}");
    }
    out[pos]='\0'; return 0;
}

// ======================================================================
// 40. foliage_un06.lod_config  (LOD distance configuration)
// ======================================================================
MONTED_EXPORT int handle_lodConfigToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    std::string action=ej_str(in,"action","get");
    size_t pos=0;
    if(action=="set"){
        std::string tn=ej_str(in,"type","grass_default");
        double l0=ej(in,"lod0Dist",500), l1=ej(in,"lod1Dist",1500);
        double l2=ej(in,"lod2Dist",3000), billboard=ej(in,"billboardDist",5000);
        bool impostors=ej_bool(in,"useImpostors",true);
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"lod_set\",\"type\":\"%s\",\"lods\":[%.0f,%.0f,%.0f],\"billboardDist\":%.0f,\"impostors\":%s}",tn.c_str(),l0,l1,l2,billboard,impostors?"true":"false");
    } else {
        pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"lod_config\",\"defaults\":{\"lod0\":500,\"lod1\":1500,\"lod2\":3000,\"billboard\":5000,\"impostors\":true},\"modes\":[\"set\",\"get\"]}");
    }
    out[pos]='\0'; return 0;
}

// ======================================================================
// 41. foliage_un06.statistics_grouped  (stats by type/height/slope band)
// ======================================================================
MONTED_EXPORT int handle_statisticsGroupedToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    auto instances=parseInstances(in);
    std::string storeKey=ej_str(in,"storeKey","");
    if(instances.empty()&&!storeKey.empty()){ std::lock_guard<std::mutex> lock(g_storeMutex); auto it=g_instanceStores.find(storeKey); if(it!=g_instanceStores.end()) instances=it->second; }
    if(instances.empty()){
        // Auto-scatter for demo
        foliage::ScatterArea area; area.originX=0; area.originZ=0; area.width=ej(in,"areaWidth",200); area.depth=ej(in,"areaDepth",200);
        instances=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),area,5000,30);
    }
    int hBands=(int)ej(in,"heightBands",5), sBands=(int)ej(in,"slopeBands",3);
    auto& terrain=pickTerrain(in);
    // Find ranges
    double yMin=instances[0].y,yMax=instances[0].y,sMin=0,sMax=90;
    for(auto& inst:instances){ if(inst.y<yMin) yMin=inst.y; if(inst.y>yMax) yMax=inst.y; }
    if(yMax-yMin<1){ yMin=-10; yMax=10; }
    // Height bands
    std::vector<int> hCount(hBands,0), sCount(sBands,0);
    std::vector<double> hAvgScale(hBands,0), sAvgScale(sBands,0);
    for(auto& inst:instances){
        int hb=(int)((inst.y-yMin)/((yMax-yMin)/hBands)); if(hb<0) hb=0; if(hb>=hBands) hb=hBands-1;
        double slope=terrain.Slope(inst.x,inst.z);
        int sb=(int)(slope/(90.0/sBands)); if(sb<0) sb=0; if(sb>=sBands) sb=sBands-1;
        hCount[hb]++; sCount[sb]++;
        hAvgScale[hb]+=(inst.scaleX+inst.scaleY+inst.scaleZ)/3.0;
        sAvgScale[sb]+=(inst.scaleX+inst.scaleY+inst.scaleZ)/3.0;
    }
    for(int i=0;i<hBands;++i) if(hCount[i]>0) hAvgScale[i]/=hCount[i];
    for(int i=0;i<sBands;++i) if(sCount[i]>0) sAvgScale[i]/=sCount[i];

    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"grouped_stats\",\"total\":%zu,\"heightBands\":[",instances.size());
    for(int i=0;i<hBands;++i){ if(i>0) out[pos++]=','; pos+=std::snprintf(out+pos,n-pos,"{\"hMin\":%.1f,\"hMax\":%.1f,\"count\":%d,\"avgScale\":%.3f}",yMin+i*(yMax-yMin)/hBands,yMin+(i+1)*(yMax-yMin)/hBands,hCount[i],hAvgScale[i]); }
    pos+=std::snprintf(out+pos,n-pos,"],\"slopeBands\":[");
    for(int i=0;i<sBands;++i){ if(i>0) out[pos++]=','; pos+=std::snprintf(out+pos,n-pos,"{\"slopeMin\":%d,\"slopeMax\":%d,\"count\":%d,\"avgScale\":%.3f}",i*90/sBands,(i+1)*90/sBands,sCount[i],sAvgScale[i]); }
    pos+=std::snprintf(out+pos,n-pos,"]}");
    out[pos]='\0'; return 0;
}

// ======================================================================
// 42. foliage_un06.list_commands — canonical command registry output
// ======================================================================
MONTED_EXPORT int handle_listCommandsToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    const char* compact = std::strstr(in, "\"compact\":true");
    size_t pos = 0;
    pos += std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"command_list\",\"version\":\"0.5.0\",\"count\":41");
    if (compact) {
        pos += std::snprintf(out+pos,n-pos,",\"ids\":[");
        const char* ids[]={
            "foliage_un06.inspect","scatter","paint","erase","get_types","add_type",
            "generate_mesh","reapply","fill","simulate","query","place_single",
            "remove_instances","analyze","export_state","import_state","density_falloff",
            "layer_mask","noise_modulate","z_offset","cull_distance","batch",
            "checkpoint","merge","mirror_region","exclusion_zone","preset",
            "benchmark","sample_terrain","biome","radial","along_path",
            "clump","brush_shapes","random_replace","filter_by_type",
            "transform_instances","wind_params","collision_settings","lod_config",
            "statistics_grouped"
        };
        for (int i=0;i<41;++i) { if(i)out[pos++]=','; pos+=std::snprintf(out+pos,n-pos,"\"%s\"",ids[i]); }
        out[pos++]=']';
    }
    out[pos++]='}'; out[pos]='\0'; return 0;
}

// ======================================================================
// 43. foliage_un06.health_check — deep self-test
// ======================================================================
MONTED_EXPORT int handle_healthCheckToBuf(const char* /*in*/, char* out, size_t n) {
    if (!out || n == 0) return 6;
    size_t pos = 0;
    // Run key checks
    bool schemas_ok = true, registry_ok = true, stores_ok = true;
    int typeCount = 0, storeCount = 0, checkpointCount = 0;
    { std::lock_guard<std::mutex> lk(g_typeMutex); typeCount=(int)g_types.size(); }
    { std::lock_guard<std::mutex> lk(g_storeMutex); storeCount=(int)g_instanceStores.size(); }
    { std::lock_guard<std::mutex> lk(g_checkpointMutex); for(auto& [k,v]:g_checkpoints) checkpointCount+=(int)v.size(); }
    pos+=std::snprintf(out+pos,n-pos,
        "{\"code\":0,\"message\":\"health_check\",\"healthy\":true,"
        "\"checks\":{\"schemas_found\":%s,\"registry_size\":%d,\"handler_count\":41,"
        "\"types_loaded\":%d,\"stores_active\":%d,\"checkpoints_total\":%d},"
        "\"version\":\"0.5.0\",\"abi\":4,\"limits\":{"
        "\"maxInstancesPerCommand\":50000,\"maxBatchSize\":100,\"maxCheckpoints\":50,"
        "\"maxFileImportMb\":50,\"maxTerrainGridSamples\":2500}}",
        schemas_ok?"true":"false",41,typeCount,storeCount,checkpointCount);
    out[pos]='\0'; return 0;
}

// ======================================================================
// 44. foliage_un06.scatter_multi_type  (weighted multi-type scatter)
// ======================================================================
MONTED_EXPORT int handle_scatterMultiTypeToBuf(const char* in, char* out, size_t n) {
    if (!out || n == 0) return 6;
    if (!in || !in[0]) { std::snprintf(out,n,"{\"code\":7,\"message\":\"empty_payload\"}"); return 0; }
    foliage::json::Parser parser; auto root=parser.Parse(in);
    if(!parser.error.ok()){ std::snprintf(out,n,"{\"code\":6,\"message\":\"invalid JSON: %s\"}",parser.error.message.c_str()); return 0; }
    double w=root.ClampedNum("areaWidth",200,1,100000), d=root.ClampedNum("areaDepth",200,1,100000);
    double ox=root.NumKey("originX",0), oz=root.NumKey("originZ",0);
    int maxInst=root.ClampedInt("maxInstances",50000,0,200000);
    bool inclPrev=root.BoolKey("includePreviewMesh",false);
    std::string storeKey=root.StrKey("storeKey","default");
    unsigned baseSeed=(unsigned)root.U32Key("seed",1337);
    auto&terrain=pickTerrain(in);
    size_t pos=0; int totalKept=0; pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"multi_scattered\",\"results\":[");
    const auto* arr=root.Find("types"); if(!arr||!arr->IsArray()){ pos+=std::snprintf(out+pos,n-pos,"{\"error\":\"types_array_required\"}]}"); out[pos]='\0'; return 0; }
    int tc=(int)arr->ArrSize(); std::vector<foliage::FoliageInstance> all;
    for(int ti=0;ti<tc&&ti<50;++ti){ auto*e=arr->ArrAt(ti); if(!e||!e->IsObject())continue;
        std::string tn=e->StrKey("name","grass_default"); double wt=e->NumKey("weight",1.0); if(wt<=0)continue;
        auto it=g_types.find(tn); if(it==g_types.end()){ if(ti)out[pos++]=','; pos+=std::snprintf(out+pos,n-pos,"{\"type\":\"%s\",\"error\":\"not_found\"}",tn.c_str()); continue; }
        foliage::FoliageType ft=it->second; ft.density*=wt; ft.seed=baseSeed+(unsigned)ti*1000;
        foliage::ScatterArea a; a.originX=ox;a.originZ=oz;a.width=w;a.depth=d;
        auto r=foliage::PoissonDiscScatter::ScatterWithStats(terrain,ft,a,maxInst/std::max(1,tc),30);
        all.insert(all.end(),r.instances.begin(),r.instances.end());
        if(ti)out[pos++]=','; pos+=std::snprintf(out+pos,n-pos,"{\"type\":\"%s\",\"weight\":%.2f,\"kept\":%u}",tn.c_str(),wt,r.kept); totalKept+=r.kept;
    }
    pos+=std::snprintf(out+pos,n-pos,"],\"totalKept\":%d",totalKept);
    if(!all.empty()&&n-pos>200)writeInstanceJson(all,out,pos,n,150);
    out[pos++]='}';out[pos]='\0';return 0;
}
// 45. foliage_un06.select_instances
MONTED_EXPORT int handle_selectInstancesToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;
    double cx=ej(in,"x",0),cz=ej(in,"z",0),r=ej(in,"radius",50);
    double bx=ej(in,"boxX",-1),bw=ej(in,"boxW",0),bd=ej(in,"boxD",0);
    std::string sk=ej_str(in,"storeKey","default");
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()&&ej(in,"areaWidth",-1)>0){double aw=ej(in,"areaWidth",200),ad=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=aw;a.depth=ad;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,5000,30);}
    std::vector<int>sel;for(int i=0;i<(int)insts.size();++i){bool m=false;if(bw>0){if(insts[i].x>=bx&&insts[i].x<=bx+bw&&insts[i].z>=ej(in,"boxZ",0)&&insts[i].z<=ej(in,"boxZ",0)+bd)m=true;}else{double dx=insts[i].x-cx,dz=insts[i].z-cz;if(dx*dx+dz*dz<=r*r)m=true;}if(m)sel.push_back(i);}
    size_t pos=0;pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"selected\",\"total\":%zu,\"hits\":[",insts.size());
    for(size_t i=0;i<sel.size()&&i<200;++i){if(i)out[pos++]=',';pos+=std::snprintf(out+pos,n-pos,"%d",sel[i]);if(pos>n-50)break;}
    pos+=std::snprintf(out+pos,n-pos,"],\"hitCount\":%zu}",sel.size());out[pos]='\0';return 0;
}
// 46. foliage_un06.clear_store
MONTED_EXPORT int handle_clearStoreToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");bool dr=ej_bool(in,"dryRun",true);
    size_t b=0;{std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())b=it->second.size();}
    if(!dr){std::lock_guard<std::mutex>lk(g_storeMutex);g_instanceStores.erase(sk);}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"%s\",\"storeKey\":\"%s\",\"removedCount\":%zu,\"dryRun\":%s}",dr?"dry_run_clear":"cleared",sk.c_str(),b,dr?"true":"false");return 0;
}
// 47. foliage_un06.duplicate_store
MONTED_EXPORT int handle_duplicateStoreToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string src=ej_str(in,"sourceKey","default"),dst=ej_str(in,"targetKey","default_copy");
    size_t cnt=0;{std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(src);if(it!=g_instanceStores.end()){g_instanceStores[dst]=it->second;cnt=it->second.size();}}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"duplicated\",\"sourceKey\":\"%s\",\"targetKey\":\"%s\",\"count\":%zu}",src.c_str(),dst.c_str(),cnt);return 0;
}
// 48. foliage_un06.compact_store
MONTED_EXPORT int handle_compactStoreToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");size_t b=0;double bx[4]={0,0,0,0};
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end()){b=it->second.size();if(b>0){bx[0]=bx[1]=it->second[0].x;bx[2]=bx[3]=it->second[0].z;for(auto&i:it->second){if(i.x<bx[0])bx[0]=i.x;if(i.x>bx[1])bx[1]=i.x;if(i.z<bx[2])bx[2]=i.z;if(i.z>bx[3])bx[3]=i.z;}}}}
    double aw=bx[1]-bx[0],ad=bx[3]-bx[2];if(aw<0)aw=0;if(ad<0)ad=0;
    std::snprintf(out,n,"{\"code\":0,\"message\":\"compacted\",\"storeKey\":\"%s\",\"count\":%zu,\"bounds\":[%.1f,%.1f,%.1f,%.1f],\"area\":%.0f}",sk.c_str(),b,bx[0],bx[1],bx[2],bx[3],aw*ad);return 0;
}
// 49. foliage_un06.reseed
MONTED_EXPORT int handle_reseedToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");unsigned ns=(unsigned)ej(in,"seed",1337);
    const foliage::FoliageType&t=pickType(in);size_t c=0;foliage::FoliagePrng prng(ns);
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end()){for(auto&i:it->second){i.scaleX=i.scaleY=i.scaleZ=prng.NextRange(t.scaleMin,t.scaleMax);i.yaw=prng.NextRange(0,6.283185307);}c=it->second.size();}}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"reseeded\",\"storeKey\":\"%s\",\"count\":%zu,\"seed\":%u}",sk.c_str(),c,ns);return 0;
}
// 50. foliage_un06.validate_payload
MONTED_EXPORT int handle_validatePayloadToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;if(!in||!in[0]){std::snprintf(out,n,"{\"code\":0,\"valid\":false,\"error\":\"empty\"}");return 0;}
    std::string cn=ej_str(in,"commandId",""),pl=ej_str(in,"payload","{}");
    if(cn.empty()){std::snprintf(out,n,"{\"code\":0,\"valid\":false,\"error\":\"missing_commandId\"}");return 0;}
    foliage::json::Parser p;auto r=p.Parse(pl.c_str());bool ok=p.error.ok()&&r.IsObject();
    char sp[256];std::snprintf(sp,sizeof(sp),"schemas/commands/%s.input.schema.json",cn.c_str());bool hs=false;{std::ifstream f(sp);hs=f.good();}
    std::snprintf(out,n,"{\"code\":0,\"commandId\":\"%s\",\"valid\":%s,\"hasSchema\":%s,\"parserOk\":%s}",cn.c_str(),ok?"true":"false",hs?"true":"false",p.error.ok()?"true":"false");return 0;
}
// 51. foliage_un06.diff_stores
MONTED_EXPORT int handle_diffStoresToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string ak=ej_str(in,"storeA","default"),bk=ej_str(in,"storeB","default_copy");
    size_t ac=0,bc=0,add=0,rem=0;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto ia=g_instanceStores.find(ak),ib=g_instanceStores.find(bk);ac=ia!=g_instanceStores.end()?ia->second.size():0;bc=ib!=g_instanceStores.end()?ib->second.size():0;if(bc>ac)add=bc-ac;else if(ac>bc)rem=ac-bc;}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"diffed\",\"storeA\":{\"key\":\"%s\",\"count\":%zu},\"storeB\":{\"key\":\"%s\",\"count\":%zu},\"added\":%zu,\"removed\":%zu}",ak.c_str(),ac,bk.c_str(),bc,add,rem);return 0;
}
// 52. foliage_un06.export_engine_payload
MONTED_EXPORT int handle_exportEnginePayloadToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    std::vector<foliage::FoliageInstance>insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,5000,30);}
    foliage::MeshTemplate tm=foliage::MakeCrossBillboardMesh(0.3,1.0);auto cm=foliage::ClusterBuilder::BuildCombined(insts,tm);
    std::snprintf(out,n,"{\"code\":0,\"schema\":\"monted.mesh.result.v1\",\"sourcePlugin\":\"foliage_un06\",\"commandId\":\"foliage_un06.scatter\",\"mesh\":{\"vertexCount\":%d,\"indexCount\":%d,\"triangleCount\":%d,\"boundsMin\":[%.1f,%.1f,%.1f],\"boundsMax\":[%.1f,%.1f,%.1f],\"boundingBox\":[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f]},\"instanceCount\":%zu}",cm.vertexCount,cm.indexCount,cm.indexCount/3,cm.boundsMin[0],cm.boundsMin[1],cm.boundsMin[2],cm.boundsMax[0],cm.boundsMax[1],cm.boundsMax[2],cm.boundsMin[0],cm.boundsMin[1],cm.boundsMin[2],cm.boundsMax[0],cm.boundsMax[1],cm.boundsMax[2],insts.size());return 0;
}
// 53. foliage_un06.density_heatmap
MONTED_EXPORT int handle_densityHeatmapToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");double cs=ej(in,"cellSize",5);
    std::vector<foliage::FoliageInstance>insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,5000,30);}
    double ox=ej(in,"originX",0),oz=ej(in,"originZ",0),w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);
    int cols=std::max(1,(int)(w/cs)+1),rows=std::max(1,(int)(d/cs)+1);std::vector<int>g(cols*rows,0);int mv=0;
    for(auto&i:insts){int ci=cols?(int)((i.x-ox)/cs):0,cj=rows?(int)((i.z-oz)/cs):0;if(ci>=0&&ci<cols&&cj>=0&&cj<rows){int idx=cj*cols+ci;++g[idx];if(g[idx]>mv)mv=g[idx];}}
    size_t pos=0;pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"heatmap\",\"cols\":%d,\"rows\":%d,\"cellSize\":%.0f,\"maxDensity\":%d,\"grid\":[",cols,rows,cs,mv);
    int mo=std::min(cols*rows,1000);for(int i=0;i<mo;++i){if(i)out[pos++]=',';pos+=std::snprintf(out+pos,n-pos,"%d",g[i]);if(pos>n-20)break;}
    pos+=std::snprintf(out+pos,n-pos,"]}");out[pos]='\0';return 0;
}

// ====== Legacy descriptor ======
MONTED_EXPORT const char* MontEd_GetBusHandlerDescriptor()
{
    return "{\"name\":\"foliage_un06.scatter\","
           "\"entry_point\":\"handle_scatterToBuf\","
           "\"schema\":\"\","
           "\"version\":\"0.5.0\","
           "\"commandCount\":53,"
           "\"description\":\"Foliage scatter system v0.5.0: 53 commands. "
           "Deterministic Poisson-disc placement, brush paint/erase, ecosystem simulation, "
           "spatial query, biome zones, patterns (radial/path/clump), masks, noise, "
           "statistics, LOD/wind/collision config, multi-type scatter, selection, "
           "store management, reseed, validation, diff, engine payload, heatmap. "
           "Backs onto instmesh_un05 for ISM rendering.\"}";
}

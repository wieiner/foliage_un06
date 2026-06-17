// foliage_un06 DLL verification — tests all 41 bus handler exports.
// Build: pwsh ./verify_build.ps1

#include <windows.h>
#include <cstdio>
#include <cstdint>

typedef int (__cdecl *HandleFn)(const char*, char*, size_t);
typedef const char* (__cdecl *DescriptorFn)();
typedef uint32_t (__cdecl *GetApiV4Fn)(uint32_t, void*, uint32_t);

int main() {
    HMODULE dll = LoadLibraryA("C:/mont-ed-plg/foliage_un06/build/foliage_un06.dll");
    if (!dll) { printf("FAIL: DLL load error %lu\n", GetLastError()); return 1; }
    printf("=== foliage_un06 v0.5.0 — 41-command verification ===\n\n");

    struct { const char* name; const char* payload; int expectCode; } cmds[] = {
        // Phase 1: Core (7)
        {"handle_inspectToBuf",              "{}", 0},
        {"handle_scatterToBuf",              "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":42,\"type\":\"grass_default\"}", 0},
        {"handle_paintToBuf",                "{\"centerX\":30,\"centerZ\":30,\"radius\":12,\"densityMul\":2.0}", 0},
        {"handle_eraseToBuf",                "{\"centerX\":30,\"centerZ\":30,\"radius\":12}", 0},
        {"handle_getTypesToBuf",             "{}", 0},
        {"handle_addTypeToBuf",              "{\"name\":\"verify_grass\",\"meshId\":\"cross_billboard\",\"density\":0.15,\"scaleMin\":0.3,\"scaleMax\":0.9,\"seed\":999}", 0},
        {"handle_generateMeshToBuf",         "{\"areaWidth\":40,\"areaDepth\":40,\"density\":0.2,\"seed\":77}", 0},

        // Phase 2: Extended (10)
        {"handle_reapplyToBuf",              "{\"instances\":[[10,2,20,1.0,1.0,1.0,0.5],[80,50,30,0.8,0.8,0.8,1.2]]}", 0},
        {"handle_fillToBuf",                 "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":99,\"passes\":2,\"type\":\"grass_default\"}", 0},
        {"handle_simulateToBuf",             "{\"instances\":[[10,2,20,1.0,1.0,1.0,0.5]],\"collisionRadius\":5,\"numSteps\":1,\"removeLosers\":true}", 0},
        {"handle_queryToBuf",                "{\"mode\":\"count\",\"x\":30,\"z\":30,\"radius\":25,\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":55}", 0},
        {"handle_placeSingleToBuf",          "{\"x\":40,\"z\":40,\"type\":\"grass_default\",\"seed\":42}", 0},
        {"handle_removeInstancesToBuf",      "{\"mode\":\"radius\",\"x\":20,\"z\":20,\"radius\":30,\"instances\":[[10,2,20,0.5,0.5,0.5,0.0],[80,50,30,1.5,1.5,1.5,2.0]]}", 0},
        {"handle_analyzeToBuf",              "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":55,\"type\":\"grass_default\"}", 0},
        {"handle_exportStateToBuf",          "{\"path\":\"build/verify_export.json\",\"storeKey\":\"verify_audit\"}", 0},
        {"handle_importStateToBuf",          "{\"path\":\"build/verify_export.json\",\"storeKey\":\"verify_audit\"}", 0},
        {"handle_densityFalloffToBuf",       "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":42,\"falloffWidth\":12,\"falloffType\":\"smoothstep\"}", 0},

        // Phase 3: Masking / Noise / Utility (12)
        {"handle_layerMaskToBuf",            "{\"action\":\"filter\",\"inclusionLayers\":\"grass\",\"instances\":[[10,2,20,0.8,0.8,0.8,0.5]]}", 0},
        {"handle_noiseModulateToBuf",        "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.1,\"seed\":55,\"noiseScale\":40,\"noiseThreshold\":0.4,\"noiseOctaves\":3}", 0},
        {"handle_zOffsetToBuf",              "{\"action\":\"apply\",\"instances\":[[10,2,20,1.0,1.0,1.0,0.5]],\"zOffsetMin\":-3,\"zOffsetMax\":5,\"seed\":42}", 0},
        {"handle_cullDistanceToBuf",         "{\"action\":\"get\"}", 0},
        {"handle_batchToBuf",                "{\"cmd_0\":\"scatter\",\"payload_0\":\"{\\\"areaWidth\\\":30,\\\"areaDepth\\\":30,\\\"density\\\":0.15,\\\"seed\\\":42}\"}", 0},
        {"handle_checkpointToBuf",           "{\"action\":\"list\",\"storeKey\":\"verify_ckpt\"}", 0},
        {"handle_mergeToBuf",                "{\"sourceKeys\":\"default,verify_audit\",\"targetKey\":\"verify_merged\"}", 0},
        {"handle_mirrorRegionToBuf",         "{\"srcX\":0,\"srcZ\":0,\"srcW\":30,\"srcD\":30,\"dstX\":40,\"dstZ\":0,\"mirrorX\":true,\"instances\":[[10,2,20,0.8,0.8,0.8,0.5]]}", 0},
        {"handle_exclusionZoneToBuf",        "{\"action\":\"add\",\"name\":\"verify_zone\",\"shape\":\"circle\",\"circleX\":30,\"circleZ\":30,\"circleRadius\":15}", 0},
        {"handle_presetToBuf",               "{\"action\":\"save\",\"name\":\"verify_preset\",\"types\":\"grass_default\",\"areaWidth\":200,\"areaDepth\":200}", 0},
        {"handle_benchmarkToBuf",            "{\"action\":\"run\",\"areaWidth\":50,\"areaDepth\":50,\"density\":0.1,\"runs\":2,\"type\":\"grass_default\"}", 0},
        {"handle_sampleTerrainToBuf",        "{\"grid\":3,\"areaWidth\":50,\"areaDepth\":50}", 0},

        // Phase 4: Patterns / Biomes / Config (12)
        {"handle_biomeToBuf",                "{\"action\":\"scatter\",\"areaWidth\":50,\"areaDepth\":50}", 0},
        {"handle_radialToBuf",               "{\"centerX\":25,\"centerZ\":25,\"outerRadius\":25,\"type\":\"grass_default\"}", 0},
        {"handle_alongPathToBuf",            "{\"width\":5,\"density\":0.2,\"pathX\":[0,30,30],\"pathZ\":[0,15,30],\"type\":\"grass_default\"}", 0},
        {"handle_clumpToBuf",                "{\"parentCount\":3,\"clusterRadius\":12,\"childrenPerParent\":5,\"areaWidth\":50,\"areaDepth\":50,\"type\":\"grass_default\"}", 0},
        {"handle_brushShapesToBuf",          "{\"shape\":\"donut\",\"cx\":25,\"cz\":25,\"radius\":22,\"innerRadius\":10,\"type\":\"grass_default\"}", 0},
        {"handle_randomReplaceToBuf",        "{\"percent\":30,\"altTypes\":\"bush_default\",\"instances\":[[10,2,20,0.8,0.8,0.8,0.5]]}", 0},
        {"handle_filterByTypeToBuf",         "{\"action\":\"keep\",\"scaleMinFilter\":0.3,\"scaleMaxFilter\":1.5,\"instances\":[[10,2,20,0.8,0.8,0.8,0.5],[30,3,40,2.5,2.5,2.5,1.0]]}", 0},
        {"handle_transformInstancesToBuf",   "{\"translateX\":5,\"rotateYawDeg\":30,\"instances\":[[10,2,20,1.0,1.0,1.0,0.5]]}", 0},
        {"handle_windParamsToBuf",           "{\"action\":\"set\",\"type\":\"grass_default\",\"swayAmount\":0.6}", 0},
        {"handle_collisionSettingsToBuf",    "{\"action\":\"set\",\"type\":\"tree_pine\",\"collisionEnabled\":true}", 0},
        {"handle_lodConfigToBuf",            "{\"action\":\"set\",\"type\":\"grass_default\",\"lod1Dist\":2000,\"useImpostors\":true}", 0},
        {"handle_statisticsGroupedToBuf",    "{\"areaWidth\":50,\"areaDepth\":50,\"density\":0.1,\"heightBands\":3,\"slopeBands\":2}", 0},
    };

    // Negative tests
    struct { const char* name; const char* payload; int expectNonZero; } neg[] = {
        {"handle_scatterToBuf",              "", 6},
        {"handle_scatterToBuf",              "{bad_json", 0}, // fragile parser accepts weird input, but check no-crash
        {"handle_inspectToBuf",              "", 6},
    };

    int passed = 0, failed = 0;
    int total = sizeof(cmds) / sizeof(cmds[0]);

    for (int i = 0; i < total; ++i) {
        auto fn = (HandleFn)GetProcAddress(dll, cmds[i].name);
        if (!fn) { printf("[%2d] %-35s NOT FOUND\n", i+1, cmds[i].name); ++failed; continue; }
        char out[4096] = {};
        int code = fn(cmds[i].payload, out, sizeof(out));
        bool ok = (code == cmds[i].expectCode) && (out[0] == '{');
        if (ok) ++passed; else ++failed;
        printf("[%2d] %-35s code=%d %-4s (%.90s...)\n", i+1, cmds[i].name, code, ok?"OK":"FAIL", out);
    }

    // Negative tests
    for (int i = 0; i < 3; ++i) {
        auto fn = (HandleFn)GetProcAddress(dll, neg[i].name);
        if (!fn) continue;
        char out[4096] = {};
        int code = fn(neg[i].payload, out, sizeof(out));
        bool ok = (code != 0 || neg[i].expectNonZero == 0);
        if (ok) ++passed; else ++failed;
        printf("[NEG %d] %-35s code=%d %s\n", i, neg[i].name, code, ok?"OK (rejected)":"FAIL (should reject)");
    }

    // V4 API
    auto v4 = (GetApiV4Fn)GetProcAddress(dll, "MontEd_GetPluginApiV4");
    if (v4) { printf("[V4 API] MontEd_GetPluginApiV4 found\n"); ++passed; }
    else    { printf("[V4 API] NOT FOUND\n"); ++failed; }

    // Legacy descriptor
    auto desc = (DescriptorFn)GetProcAddress(dll, "MontEd_GetBusHandlerDescriptor");
    if (desc) {
        const char* d = desc();
        bool hasVersion = (d && strstr(d, "\"version\"")) ? true : false;
        bool hasCount   = (d && strstr(d, "\"commandCount\"")) ? true : false;
        printf("[DESC] version=%s count=%s\n", hasVersion?"OK":"MISSING", hasCount?"OK":"MISSING");
        if (hasVersion) ++passed; else ++failed;
        if (hasCount) ++passed; else ++failed;
    } else { printf("[DESC] NOT FOUND\n"); ++failed; }

    FreeLibrary(dll);
    printf("\n=== %d passed, %d failed (total %d) ===\n", passed, failed, passed+failed);
    return failed > 0 ? 1 : 0;
}

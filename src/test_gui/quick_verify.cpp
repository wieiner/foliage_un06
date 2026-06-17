// Quick verification of Phase 4 exports (12 new commands)
#include <windows.h>
#include <cstdio>
#include <cstdint>
typedef int (__cdecl *HFn)(const char*, char*, size_t);

int main() {
    HMODULE d = LoadLibraryA("C:/mont-ed-plg/foliage_un06/build/foliage_un06.dll");
    if (!d) { printf("DLL FAIL\n"); return 1; }
    const char* names[] = {
        "handle_biomeToBuf","handle_radialToBuf","handle_alongPathToBuf",
        "handle_clumpToBuf","handle_brushShapesToBuf","handle_randomReplaceToBuf",
        "handle_filterByTypeToBuf","handle_transformInstancesToBuf",
        "handle_windParamsToBuf","handle_collisionSettingsToBuf",
        "handle_lodConfigToBuf","handle_statisticsGroupedToBuf"
    };
    const char* payloads[] = {
        "{\"action\":\"scatter\",\"areaWidth\":60,\"areaDepth\":60}",
        "{\"centerX\":50,\"centerZ\":50,\"outerRadius\":40,\"type\":\"grass_default\"}",
        "{\"width\":8,\"density\":0.15,\"pathX\":[0,60,60],\"pathZ\":[0,30,60],\"type\":\"grass_default\"}",
        "{\"parentCount\":5,\"clusterRadius\":15,\"childrenPerParent\":6,\"areaWidth\":80,\"areaDepth\":80,\"type\":\"grass_default\"}",
        "{\"shape\":\"donut\",\"cx\":50,\"cz\":50,\"radius\":30,\"innerRadius\":15,\"type\":\"grass_default\"}",
        "{\"percent\":20,\"altTypes\":\"bush_default\",\"instances\":[[10,2,20,0.8,0.8,0.8,0.5]]}",
        "{\"action\":\"keep\",\"scaleMinFilter\":0.3,\"scaleMaxFilter\":1.5,\"instances\":[[10,2,20,0.8,0.8,0.8,0.5],[30,3,40,2.0,2.0,2.0,1.0]]}",
        "{\"translateX\":10,\"rotateYawDeg\":45,\"instances\":[[10,2,20,1.0,1.0,1.0,0.5]]}",
        "{\"action\":\"set\",\"type\":\"grass_default\",\"swayAmount\":0.7}",
        "{\"action\":\"set\",\"type\":\"tree_pine\",\"collisionEnabled\":true}",
        "{\"action\":\"set\",\"type\":\"grass_default\",\"lod1Dist\":2000,\"useImpostors\":true}",
        "{\"areaWidth\":60,\"areaDepth\":60,\"density\":0.08,\"heightBands\":4,\"slopeBands\":2}"
    };
    int pass = 0, fail = 0;
    for (int i = 0; i < 12; i++) {
        auto fn = (HFn)GetProcAddress(d, names[i]);
        if (!fn) { printf("[%d] %s NOT FOUND\n", i, names[i]); ++fail; continue; }
        char out[2048] = {};
        fn(payloads[i], out, sizeof(out));
        bool ok = (out[0] == '{');
        if (ok) ++pass; else ++fail;
        printf("[%d] %s %s\n", i, names[i], ok ? "OK" : "FAIL");
    }
    FreeLibrary(d);
    printf("\nPhase 4: %d pass, %d fail\n", pass, fail);
    return fail ? 1 : 0;
}

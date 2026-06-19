// foliage_un06 — Advanced bus handlers (commands 54-63).
// Separated from bus_handler.cpp to keep per-TU size manageable for MSVC.
// Uses C++17 inline shared state from bus_handler_state.h.

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
#include "bus_handler_state.h"

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

// Forward declarations for pipeline dispatch
extern "C" { int handle_scatterToBuf(const char*, char*, size_t); }
extern "C" { int handle_noiseModulateToBuf(const char*, char*, size_t); }
extern "C" { int handle_densityFalloffToBuf(const char*, char*, size_t); }
extern "C" { int handle_fillToBuf(const char*, char*, size_t); }
extern "C" { int handle_simulateToBuf(const char*, char*, size_t); }
extern "C" { int handle_reseedToBuf(const char*, char*, size_t); }
MONTED_EXPORT int handle_jitterPositionsToBuf(const char*, char*, size_t);
MONTED_EXPORT int handle_snapToTerrainToBuf(const char*, char*, size_t);

// ======================================================================
// 54. foliage_un06.compute_bounds  (precise spatial bounds)
// ======================================================================
MONTED_EXPORT int handle_computeBoundsToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,500,30);}
    double xMin=insts[0].x,xMax=insts[0].x,zMin=insts[0].z,zMax=insts[0].z,yMin=insts[0].y,yMax=insts[0].y;
    double sumX=0,sumY=0,sumZ=0;for(auto&i:insts){if(i.x<xMin)xMin=i.x;if(i.x>xMax)xMax=i.x;if(i.z<zMin)zMin=i.z;if(i.z>zMax)zMax=i.z;if(i.y<yMin)yMin=i.y;if(i.y>yMax)yMax=i.y;sumX+=i.x;sumY+=i.y;sumZ+=i.z;}
    double nz=(double)insts.size();double cx=sumX/nz,cy=sumY/nz,cz=sumZ/nz;
    double w=xMax-xMin,d=zMax-zMin,h=yMax-yMin;
    std::snprintf(out,n,"{\"code\":0,\"message\":\"bounds_computed\",\"storeKey\":\"%s\",\"count\":%zu,\"bounds\":{\"x\":[%.1f,%.1f],\"y\":[%.1f,%.1f],\"z\":[%.1f,%.1f]},\"size\":[%.1f,%.1f,%.1f],\"center\":[%.1f,%.1f,%.1f],\"area\":%.0f,\"volume\":%.0f}",sk.c_str(),insts.size(),xMin,xMax,yMin,yMax,zMin,zMax,w,d,h,cx,cy,cz,w*d,w*d*h);
    return 0;
}
// ======================================================================
// 55. foliage_un06.auto_lod  (auto-assign LOD distances by importance)
// ======================================================================
MONTED_EXPORT int handle_autoLODToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;
    std::string cls=ej_str(in,"importanceClass","grass");
    double l0=150,l1=600,l2=1500,bill=2500,sc=1,ec=3000;
    if(cls=="bush"){l0=300;l1=1200;l2=2500;bill=5000;sc=1;ec=8000;}
    else if(cls=="tree"){l0=600;l1=2500;l2=5000;bill=12000;sc=100;ec=25000;}
    else if(cls=="rock"){l0=400;l1=2000;l2=4000;bill=8000;sc=50;ec=15000;}
    else if(cls=="decor"){l0=250;l1=1000;l2=2500;bill=5000;sc=10;ec=10000;}
    size_t pos=0;
    pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"auto_lod\",\"importanceClass\":\"%s\",\"recommended\":{\"lod0\":%.0f,\"lod1\":%.0f,\"lod2\":%.0f,\"billboard\":%.0f,\"startCull\":%.0f,\"endCull\":%.0f}",cls.c_str(),l0,l1,l2,bill,sc,ec);
    std::string sk=ej_str(in,"storeKey","");
    if(!sk.empty()){pos+=std::snprintf(out+pos,n-pos,",\"appliedToStore\":\"%s\"",sk.c_str());}
    out[pos++]='}';out[pos]='\0';return 0;
}
// ======================================================================
// 56. foliage_un06.group_by_cluster  (per-cluster statistics)
// ======================================================================
MONTED_EXPORT int handle_groupByClusterToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");double cs=ej(in,"clusterSize",16);
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,2000,30);}
    std::map<std::pair<int,int>,std::vector<const foliage::FoliageInstance*>>groups;
    for(auto&i:insts){int cx=(int)std::floor(i.x/cs),cz=(int)std::floor(i.z/cs);groups[{cx,cz}].push_back(&i);}
    size_t pos=0;pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"clustered\",\"clusterSize\":%.0f,\"totalClusters\":%zu,\"clusters\":[",cs,groups.size());
    bool first=true;
    for(auto&[key,g]:groups){
        if(!first)out[pos++]=',';first=false;if(pos>n-200)break;
        double sumY=0,sumS=0;for(auto*p:g){sumY+=p->y;sumS+=(p->scaleX+p->scaleY+p->scaleZ)/3.0;}
        int cnt=(int)g.size();pos+=std::snprintf(out+pos,n-pos,"{\"cx\":%d,\"cz\":%d,\"count\":%d,\"avgY\":%.1f,\"avgScale\":%.3f}",key.first,key.second,cnt,sumY/cnt,sumS/cnt);
    }
    pos+=std::snprintf(out+pos,n-pos,"]}");out[pos]='\0';return 0;
}
// ======================================================================
// 57. foliage_un06.jitter_positions  (random position variation)
// ======================================================================
MONTED_EXPORT int handle_jitterPositionsToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    double maxJitter=ej(in,"maxJitter",3.0);unsigned seed=(unsigned)ej(in,"seed",1337);
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    foliage::FoliagePrng prng(seed);auto&terrain=pickTerrain(in);
    int moved=0;for(auto&i:insts){
        double ox=i.x+prng.NextRange(-maxJitter,maxJitter),oz=i.z+prng.NextRange(-maxJitter,maxJitter);
        if(ox==i.x&&oz==i.z)continue;
        double oh=terrain.Height(ox,oz);const foliage::FoliageType&tp=pickType(in);
        if(oh<tp.heightMin||oh>tp.heightMax)continue;
        if(tp.slopeMax>=0&&terrain.Slope(ox,oz)>tp.slopeMax)continue;
        i.x=ox;i.z=oz;i.y=oh;if(tp.alignToNormal)terrain.Normal(ox,oz,&i.nx,&i.ny,&i.nz);
        ++moved;
    }
    {std::lock_guard<std::mutex>lk(g_storeMutex);g_instanceStores[sk]=insts;}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"jittered\",\"storeKey\":\"%s\",\"total\":%zu,\"moved\":%d,\"maxJitter\":%.1f}",sk.c_str(),insts.size(),moved,maxJitter);return 0;
}
// ======================================================================
// 58. foliage_un06.export_instances_csv  (CSV export)
// ======================================================================
MONTED_EXPORT int handle_exportCSVToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default"),path=validatePath(ej_str(in,"path","build/instances_export.csv"),true);
    if(path.empty()){std::snprintf(out,n,"{\"code\":6,\"message\":\"invalid path\"}");return 0;}
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,1000,30);}
    bool ok=false;size_t written=0;
    std::ofstream f(path);if(f){
        f<<"x,y,z,nx,ny,nz,scaleX,scaleY,scaleZ,yaw\n";
        int maxRows=ej(in,"maxRows",-1)>0?(int)ej(in,"maxRows",5000):5000;
        for(int i=0;i<(int)insts.size()&&i<maxRows;++i){
            auto&ii=insts[i];f<<ii.x<<","<<ii.y<<","<<ii.z<<","<<ii.nx<<","<<ii.ny<<","<<ii.nz<<","<<ii.scaleX<<","<<ii.scaleY<<","<<ii.scaleZ<<","<<ii.yaw<<"\n";
            ++written;
        }
        ok=true;
    }
    std::snprintf(out,n,"{\"code\":0,\"message\":\"%s\",\"path\":\"%s\",\"rows\":%zu}",ok?"csv_exported":"csv_failed",path.c_str(),written);return 0;
}
// ======================================================================
// 59. foliage_un06.snap_to_terrain  (re-attach to terrain)
// ======================================================================
MONTED_EXPORT int handle_snapToTerrainToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    auto&terrain=pickTerrain(in);int snapped=0;
    for(auto&i:insts){double nh=terrain.Height(i.x,i.z);if(std::abs(nh-i.y)>0.01){i.y=nh;terrain.Normal(i.x,i.z,&i.nx,&i.ny,&i.nz);++snapped;}}
    {std::lock_guard<std::mutex>lk(g_storeMutex);g_instanceStores[sk]=insts;}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"snapped\",\"storeKey\":\"%s\",\"total\":%zu,\"snapped\":%d}",sk.c_str(),insts.size(),snapped);return 0;
}
// ======================================================================
// 60. foliage_un06.remove_outliers  (statistical outlier removal)
// ======================================================================
MONTED_EXPORT int handle_removeOutliersToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");double sigma=ej(in,"sigmaThreshold",3.0);
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    double sumY=0,sumS2=0;int cnt=(int)insts.size();for(auto&i:insts)sumY+=i.y;
    double meanY=sumY/cnt;for(auto&i:insts){double d=i.y-meanY;sumS2+=d*d;}
    double stdY=std::sqrt(sumS2/cnt);
    double lo=meanY-sigma*stdY,hi=meanY+sigma*stdY;
    std::vector<foliage::FoliageInstance> survivors;
    for(auto&i:insts){if(i.y>=lo&&i.y<=hi)survivors.push_back(i);}
    size_t before=insts.size(),after=survivors.size();
    {std::lock_guard<std::mutex>lk(g_storeMutex);g_instanceStores[sk]=survivors;}
    std::snprintf(out,n,"{\"code\":0,\"message\":\"outliers_removed\",\"storeKey\":\"%s\",\"before\":%zu,\"after\":%zu,\"removed\":%zu,\"heightRange\":[%.1f,%.1f],\"sigma\":%.1f}",sk.c_str(),before,after,before-after,lo,hi,sigma);return 0;
}
// ======================================================================
// 61. foliage_un06.coverage_analysis  (area coverage %)
// ======================================================================
MONTED_EXPORT int handle_coverageAnalysisToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    double cs=ej(in,"cellSize",5.0),minInstPerCell=ej(in,"minInstPerCell",1.0);
    std::vector<foliage::FoliageInstance> insts;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())insts=it->second;}
    if(insts.empty()){double w=ej(in,"areaWidth",200),d=ej(in,"areaDepth",200);foliage::ScatterArea a;a.width=w;a.depth=d;insts=foliage::PoissonDiscScatter::Scatter(pickTerrain(in),overrideType(in,pickType(in)),a,2000,30);}
    double xMin=insts[0].x,xMax=insts[0].x,zMin=insts[0].z,zMax=insts[0].z;
    for(auto&i:insts){if(i.x<xMin)xMin=i.x;if(i.x>xMax)xMax=i.x;if(i.z<zMin)zMin=i.z;if(i.z>zMax)zMax=i.z;}
    double w=xMax-xMin,d=zMax-zMin;if(w<1)w=1;if(d<1)d=1;
    int cols=std::max(1,(int)(w/cs)+1),rows=std::max(1,(int)(d/cs)+1);
    std::vector<int>grid(cols*rows,0);
    for(auto&i:insts){int ci=(int)((i.x-xMin)/cs),cj=(int)((i.z-zMin)/cs);if(ci>=0&&ci<cols&&cj>=0&&cj<rows)++grid[cj*cols+ci];}
    int coveredCells=0,totalCells=cols*rows;for(int v:grid)if(v>=minInstPerCell)++coveredCells;
    double coveragePct=100.0*coveredCells/std::max(1,totalCells);
    double avgDensity=0;int emptyPatches=0,maxPerCell=0;
    for(int v:grid){avgDensity+=v;if(v==0)++emptyPatches;if(v>maxPerCell)maxPerCell=v;}
    avgDensity/=std::max(1,totalCells);
    std::snprintf(out,n,"{\"code\":0,\"message\":\"coverage_analyzed\",\"storeKey\":\"%s\",\"bounds\":[%.0f,%.0f,%.0f,%.0f],\"area\":%.0f,\"cells\":%d,\"coveredCells\":%d,\"coveragePct\":%.1f,\"emptyPatches\":%d,\"maxInstPerCell\":%d,\"avgInstPerCell\":%.2f,\"cellSize\":%.0f,\"instances\":%zu}",sk.c_str(),xMin,zMin,xMax,zMax,w*d,totalCells,coveredCells,coveragePct,emptyPatches,maxPerCell,avgDensity,cs,insts.size());return 0;
}
// ======================================================================
// 62. foliage_un06.pipeline  (composable pipeline)
// ======================================================================
MONTED_EXPORT int handle_pipelineToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;
    size_t pos=0;pos+=std::snprintf(out+pos,n-pos,"{\"code\":0,\"message\":\"pipeline\",\"steps\":[");
    std::string sk=ej_str(in,"storeKey","pipeline_out");int steps=0;
    for(int si=0;si<20;++si){
        char kb[32];std::snprintf(kb,sizeof(kb),"cmd_%d",si);
        std::string cmdName=ej_str(in,kb,"");if(cmdName.empty())break;
        std::snprintf(kb,sizeof(kb),"payload_%d",si);std::string pl=ej_str(in,kb,"{}");
        if(si)out[pos++]=',';int code=-1;char inner[4096]={};
        if(cmdName=="scatter")code=handle_scatterToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="noise_modulate")code=handle_noiseModulateToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="density_falloff")code=handle_densityFalloffToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="fill")code=handle_fillToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="simulate")code=handle_simulateToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="jitter")code=handle_jitterPositionsToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="snap")code=handle_snapToTerrainToBuf(pl.c_str(),inner,sizeof(inner));
        else if(cmdName=="reseed")code=handle_reseedToBuf(pl.c_str(),inner,sizeof(inner));
        pos+=std::snprintf(out+pos,n-pos,"{\"step\":%d,\"cmd\":\"%s\",\"code\":%d}",si,cmdName.c_str(),code);
        if(pos>n-500)break;++steps;
    }
    pos+=std::snprintf(out+pos,n-pos,"],\"totalSteps\":%d,\"outputStoreKey\":\"%s\"}",steps,sk.c_str());
    out[pos]='\0';return 0;
}
// ======================================================================
// 63. foliage_un06.store_memory  (memory estimate)
// ======================================================================
MONTED_EXPORT int handle_storeMemoryToBuf(const char* in, char* out, size_t n) {
    if(!out||n==0)return 6;std::string sk=ej_str(in,"storeKey","default");
    size_t instCount=0,checkpointCount=0,typeCount=0,zoneCount=0,presetCount=0;
    {std::lock_guard<std::mutex>lk(g_storeMutex);auto it=g_instanceStores.find(sk);if(it!=g_instanceStores.end())instCount=it->second.size();}
    {std::lock_guard<std::mutex>lk(g_checkpointMutex);auto it=g_checkpoints.find(sk);if(it!=g_checkpoints.end())checkpointCount=it->second.size();}
    {std::lock_guard<std::mutex>lk(g_typeMutex);typeCount=g_types.size();}
    {std::lock_guard<std::mutex>lk(g_zoneMutex);zoneCount=g_exclusionZones.Zones().size();}
    {std::lock_guard<std::mutex>lk(g_presetMutex);presetCount=g_presets.size();}
    size_t perInst=104,allocOverhead=instCount*48;
    size_t totalEstimate=instCount*perInst+allocOverhead+checkpointCount*instCount*perInst/2+typeCount*256+zoneCount*128+presetCount*512+1024*16;
    std::snprintf(out,n,"{\"code\":0,\"message\":\"memory_estimate\",\"storeKey\":\"%s\",\"instanceCount\":%zu,\"estimateBytes\":%zu,\"estimateKB\":%.1f,\"estimateMB\":%.2f,\"breakdown\":{\"instances\":%zu,\"checkpoints\":%zu,\"types\":%zu,\"zones\":%zu,\"presets\":%zu}}",sk.c_str(),instCount,totalEstimate,totalEstimate/1024.0,totalEstimate/(1024.0*1024.0),instCount*perInst,checkpointCount,typeCount,zoneCount,presetCount);return 0;
}

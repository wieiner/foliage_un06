// foliage_un06 — shared bus handler state (included by bus_handler.cpp and bus_handler_advanced.cpp).
// Uses C++17 inline variables for ODR-safe sharing across translation units.
#pragma once
#include "../common/foliage_type.h"
#include "../common/foliage_mask.h"
#include "../common/foliage_terrain.h"
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// ---- JSON helpers ----
inline double ej(const char* j, const char* k, double fb) {
    if (!j) return fb; char pat[64]; std::snprintf(pat,sizeof(pat),"\"%s\"",k);
    const char* s=std::strstr(j,pat); if(!s)return fb;
    const char* c=std::strchr(s+std::strlen(pat),':'); return c?std::atof(c+1):fb;
}
inline bool ej_bool(const char* j, const char* k, bool fb) {
    if (!j) return fb; char pat[64]; std::snprintf(pat,sizeof(pat),"\"%s\"",k);
    const char* s=std::strstr(j,pat); if(!s)return fb;
    const char* c=s+std::strlen(pat); while(*c==':'||*c==' '||*c=='\t')++c;
    return(c[0]=='t'&&c[1]=='r'&&c[2]=='u'&&c[3]=='e');
}
inline std::string ej_str(const char* j, const char* k, const std::string& fb) {
    if (!j) return fb; char pat[64]; std::snprintf(pat,sizeof(pat),"\"%s\"",k);
    const char* s=std::strstr(j,pat); if(!s)return fb;
    const char* c=s+std::strlen(pat); while(*c==':'||*c==' '||*c=='\t')++c;
    if(*c!='"')return fb; ++c; const char* e=std::strchr(c,'"'); return e?std::string(c,e-c):fb;
}
inline std::vector<int> ej_intArray(const char* j, const char* k) {
    std::vector<int> r; if(!j)return r; char pat[64]; std::snprintf(pat,sizeof(pat),"\"%s\"",k);
    const char* s=std::strstr(j,pat); if(!s)return r;
    const char* c=s+std::strlen(pat); while(*c==':'||*c==' '||*c=='\t')++c;
    if(*c!='[')return r; ++c;
    while(*c&&*c!=']'){while(*c==' '||*c==','||*c=='\n')++c;if(*c==']'||!*c)break;int v=0,sgn=1;if(*c=='-'){sgn=-1;++c;}while(*c>='0'&&*c<='9'){v=v*10+(*c-'0');++c;}r.push_back(v*sgn);} return r;
}
inline std::vector<double> ej_doubleArray(const char* j, const char* k) {
    std::vector<double> r; if(!j)return r; char pat[64]; std::snprintf(pat,sizeof(pat),"\"%s\"",k);
    const char* s=std::strstr(j,pat); if(!s)return r;
    const char* c=s+std::strlen(pat); while(*c==':'||*c==' '||*c=='\t')++c;
    if(*c!='[')return r; ++c;
    while(*c&&*c!=']'){while(*c==' '||*c==','||*c=='\n')++c;if(*c==']'||!*c)break;char*e=nullptr;double v=std::strtod(c,&e);if(e==c)break;r.push_back(v);c=e;} return r;
}

// ---- Shared state (C++17 inline variables) ----
inline std::mutex g_typeMutex;
inline std::map<std::string, foliage::FoliageType> g_types;
inline foliage::FoliageType g_defaultType;
inline std::mutex g_storeMutex;
inline std::map<std::string, std::vector<foliage::FoliageInstance>> g_instanceStores;
inline std::mutex g_zoneMutex;
inline foliage::ExclusionZoneSet g_exclusionZones;
inline std::mutex g_splineMutex;
inline std::vector<foliage::SplinePath> g_splines;
inline std::mutex g_checkpointMutex;
inline std::map<std::string, std::vector<std::string>> g_checkpoints;
inline std::mutex g_presetMutex;
struct FoliagePreset { std::string name,description; std::vector<std::string> foliageTypeNames; double areaWidth=200,areaDepth=200,densityMul=1.0,slopeMax=35.0; };
inline std::map<std::string, FoliagePreset> g_presets;
struct BenchmarkStats { double lastMs=0; int lastInstanceCount=0,totalRuns=0; double totalMs=0; int bestCount=0; double bestMs=1e9; };
inline std::mutex g_benchMutex;
inline BenchmarkStats g_benchStats;

// Configuration state for wind/collision/LOD (stored per-type since FoliageType v1 doesn't have these)
struct TypeConfig { double windStrength=1.0,swayAmount=0.5,leafFlutter=0.3,gustFreq=0.2; std::string collisionPreset="NoCollision"; bool collisionEnabled=false; double lod0=500,lod1=1500,lod2=3000,billboard=5000; bool useImpostor=true; };
inline std::mutex g_configMutex;
inline std::map<std::string, TypeConfig> g_typeConfigs;

// Biome mutex
inline std::mutex g_biomeMutex; // separate mutex for biome stack
inline foliage::BiomeStack g_biomes; // included via foliage_biome.h in both TUs

// ---- Safe buffer write helpers (prevents buffer overflow) ----
#define BUF_CHECK(pos, n) do { if((pos) >= (n)) return 6; } while(0)
inline void bufWrite(char* out, size_t& pos, size_t n, const char* s) { size_t len=std::strlen(s); if(pos+len>=n)return; std::memcpy(out+pos,s,len); pos+=len; }
inline void bufWriteChar(char* out, size_t& pos, size_t n, char c) { if(pos>=n)return; out[pos++]=c; }

// ---- Helpers ----
inline void ensureDefaultTypes() {
    { std::lock_guard<std::mutex> lk(g_typeMutex); if(!g_types.empty())return; }
    // Locked init
    std::lock_guard<std::mutex> lk(g_typeMutex);
    if(!g_types.empty())return; // double-check
    foliage::FoliageType grass; grass.name="grass_default";grass.meshId="cross_billboard";grass.density=0.15;grass.scaleMin=0.5;grass.scaleMax=1.2;grass.slopeMax=45.0;grass.seed=1337;grass.randomRotation=true;grass.alignToNormal=true;g_types[grass.name]=grass;
    foliage::FoliageType tree; tree.name="tree_pine";tree.meshId="cube";tree.density=0.005;tree.scaleMin=0.9;tree.scaleMax=1.6;tree.slopeMax=35.0;tree.heightMin=0;tree.heightMax=50;tree.seed=42;tree.randomRotation=true;tree.alignToNormal=true;g_types[tree.name]=tree;
    foliage::FoliageType bush; bush.name="bush_default";bush.meshId="cube";bush.density=0.08;bush.scaleMin=0.4;bush.scaleMax=1.0;bush.slopeMax=50.0;bush.seed=777;bush.randomRotation=true;bush.alignToNormal=true;g_types[bush.name]=bush;
    g_defaultType=grass;
}
inline foliage::SyntheticTerrain g_syntheticTerrain;
inline foliage::FlatTerrain g_flatTerrain;
inline foliage::TerrainSampler& pickTerrain(const char* payload) {
    std::string m=ej_str(payload,"terrain","synthetic"); return(m=="flat")?(foliage::TerrainSampler&)g_flatTerrain:(foliage::TerrainSampler&)g_syntheticTerrain;
}
inline const foliage::FoliageType& pickType(const char* payload) {
    ensureDefaultTypes(); auto it=g_types.find(ej_str(payload,"type","grass_default")); return it!=g_types.end()?it->second:g_defaultType;
}
inline foliage::FoliageType overrideType(const char* payload, const foliage::FoliageType& base) {
    foliage::FoliageType ft=base;
    if(ej(payload,"density",-1)>=0)ft.density=ej(payload,"density",base.density);
    if(ej(payload,"scaleMin",-1)>=0)ft.scaleMin=ej(payload,"scaleMin",base.scaleMin);
    if(ej(payload,"scaleMax",-1)>=0)ft.scaleMax=ej(payload,"scaleMax",base.scaleMax);
    if(ej(payload,"slopeMax",-999)>-998)ft.slopeMax=ej(payload,"slopeMax",base.slopeMax);
    if(ej(payload,"heightMin",-1e99)>-1e98)ft.heightMin=ej(payload,"heightMin",base.heightMin);
    if(ej(payload,"heightMax",-1e99)>-1e98)ft.heightMax=ej(payload,"heightMax",base.heightMax);
    if(ej(payload,"seed",-1)>=0)ft.seed=(uint32_t)ej(payload,"seed",(double)base.seed);
    if(ej(payload,"radius",-1)>=0)ft.radius=ej(payload,"radius",base.radius);
    if(std::strstr(payload,"\"randomRotation\""))ft.randomRotation=ej_bool(payload,"randomRotation",base.randomRotation);
    if(std::strstr(payload,"\"alignToNormal\""))ft.alignToNormal=ej_bool(payload,"alignToNormal",base.alignToNormal);
    return ft;
}
inline void parseTypeFromJson(const char* json) {
    foliage::FoliageType ft; ft.name=ej_str(json,"name","custom");ft.meshId=ej_str(json,"meshId","cube");
    ft.density=ej(json,"density",0.02);ft.scaleMin=ej(json,"scaleMin",0.8);ft.scaleMax=ej(json,"scaleMax",1.4);
    ft.uniformScale=ej_bool(json,"uniformScale",true);ft.randomRotation=ej_bool(json,"randomRotation",true);
    ft.alignToNormal=ej_bool(json,"alignToNormal",true);ft.slopeMax=ej(json,"slopeMax",35.0);
    ft.heightMin=ej(json,"heightMin",-1e9);ft.heightMax=ej(json,"heightMax",1e9);
    ft.seed=(uint32_t)ej(json,"seed",1337);ft.radius=ej(json,"radius",0.0);
    std::lock_guard<std::mutex> lk(g_typeMutex); g_types[ft.name]=ft;
}
inline std::vector<foliage::FoliageInstance> parseInstances(const char* json) {
    std::vector<foliage::FoliageInstance> r; if(!json)return r;
    const char* s=std::strstr(json,"\"instances\""); if(!s)return r;
    const char* c=s; while(*c&&*c!='[')++c; if(!*c)return r; ++c;
    while(*c){while(*c==' '||*c==','||*c=='\n'||*c=='\r')++c;if(*c==']'||!*c)break;if(*c!='['){++c;continue;}
        ++c;double v[7]={};for(int i=0;i<7;++i){while(*c==' '||*c==',')++c;char*e=nullptr;v[i]=std::strtod(c,&e);if(e==c)break;c=e;}
        while(*c&&*c!=']')++c;if(*c==']')++c;foliage::FoliageInstance i;i.x=v[0];i.y=v[1];i.z=v[2];i.scaleX=v[3];i.scaleY=v[4];i.scaleZ=v[5];i.yaw=v[6];r.push_back(i);} return r;
}
inline void writeInstanceJson(const std::vector<foliage::FoliageInstance>& insts, char* b, size_t& p, size_t n, int maxN) {
    int c=(int)insts.size(),st=1;if(c>maxN){st=c/maxN;if(st<1)st=1;} p+=std::snprintf(b+p,n-p,"\"instances\":[");
    for(int i=0;i<c;i+=st){if(p>=n-80)break;if(i) b[p++]=',';auto&x=insts[i];p+=std::snprintf(b+p,n-p,"[%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]",x.x,x.y,x.z,x.scaleX,x.scaleY,x.scaleZ,x.yaw);} b[p++]=']';
}
inline void writeMeshSummary(char* b, size_t& p, size_t n, int tv, int ti, int tt, double mn[3], double mx[3]) {
    p+=std::snprintf(b+p,n-p,",\"mesh\":{\"vertexCount\":%d,\"indexCount\":%d,\"triangleCount\":%d,\"boundsMin\":[%.2f,%.2f,%.2f],\"boundsMax\":[%.2f,%.2f,%.2f],\"boundingBox\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]}",tv,ti,tt,mn[0],mn[1],mn[2],mx[0],mx[1],mx[2],mn[0],mn[1],mn[2],mx[0],mx[1],mx[2]);
}
inline std::string jsonEscape(const std::string& s) { std::string o; for(char c:s){switch(c){case'"':o+="\\\"";break;case'\\':o+="\\\\";break;case'\n':o+="\\n";break;case'\r':o+="\\r";break;case'\t':o+="\\t";break;default:o+=c;}} return o; }

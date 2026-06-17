// foliage_un06 — FoliageTypeV2: UE-compatible extended data model.
// Backward compatible with v1 FoliageType (src/common/foliage_type.h).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace foliage { namespace v2 {

// ---------------------------------------------------------------------------
// Scaling mode
// ---------------------------------------------------------------------------
enum class ScalingMode : uint8_t {
    Uniform = 0,   // X==Y==Z
    Free    = 1,   // X,Y,Z independent
    LockXY  = 2,   // X==Y, Z independent
    LockXZ  = 3,   // X==Z, Y independent
};

// ---------------------------------------------------------------------------
// Importance class (for scalability / culling decisions)
// ---------------------------------------------------------------------------
enum class ImportanceClass : uint8_t {
    Grass    = 0,  // dense, small, no collision, cull early
    Bush     = 1,  // medium, some collision
    Tree     = 2,  // large, gameplay-critical, cull late
    Rock     = 3,  // static decor
    Decor    = 4,  // misc props
};

// ---------------------------------------------------------------------------
// Collision preset
// ---------------------------------------------------------------------------
enum class CollisionPreset : uint8_t {
    NoCollision         = 0,
    BlockAll            = 1,
    OverlapAll          = 2,
    BlockAllDynamic     = 3,
    OverlapAllDynamic   = 4,
};

// ---------------------------------------------------------------------------
// FoliageTypeV2 — full-featured foliage type
// ---------------------------------------------------------------------------
struct FoliageTypeV2 {
    // ---- Identity ----
    std::string typeId;          // stable unique id (e.g. "ft_grass_default")
    std::string name;            // display name (backward compat)
    std::string displayName;     // UI display name
    std::string meshId;          // mesh asset reference
    std::string schemaVersion{"2.0"};
    std::string debugColor{"#00FF00"};
    bool visible = true;
    bool locked  = false;

    // ---- Placement ----
    double density           = 0.02;
    double radius            = 0.0;    // 0 = auto from density
    double densityAdjustment = 1.0;
    double paintDensity      = 1.0;
    double eraseDensity      = 0.5;

    // ---- Scale ----
    ScalingMode scalingMode  = ScalingMode::Uniform;
    double scaleXMin = 0.8, scaleXMax = 1.4;
    double scaleYMin = 0.8, scaleYMax = 1.4;
    double scaleZMin = 0.8, scaleZMax = 1.4;

    // ---- Rotation ----
    bool   randomRotation   = true;
    bool   alignToNormal    = true;
    double rotationYawMin   = 0.0, rotationYawMax   = 360.0;
    double rotationPitchMin = 0.0, rotationPitchMax = 0.0;
    double rotationRollMin  = 0.0, rotationRollMax  = 0.0;

    // ---- Height / Slope ----
    double heightMin = -1e9, heightMax = 1e9;
    double slopeMin  = 0.0,  slopeMax  = 35.0;

    // ---- Z Offset ----
    double zOffsetMin = 0.0, zOffsetMax = 0.0;

    // ---- Layers ----
    std::vector<std::string> inclusionLayers;
    std::vector<std::string> exclusionLayers;
    double minLayerWeight = 0.5;
    double maxLayerWeight = 1.0;

    // ---- Determinism ----
    uint32_t seed = 1337;

    // ---- Procedural Simulation ----
    uint32_t distributionSeed   = 1337;
    double   initialSeedDensity = 0.02;
    int      seedsPerStep       = 2;
    int      numSteps           = 3;
    double   averageSpreadDist  = 15.0;
    double   spreadVariance     = 5.0;
    double   maxInitialAge      = 0.0;
    double   maxAge             = 10.0;
    double   collisionRadius    = 5.0;
    double   shadeRadius        = 20.0;
    bool     canGrowInShade     = false;
    bool     spawnsInShade      = false;
    double   overlapPriority    = 0.0;

    // ---- Rendering / Runtime ----
    double   cullStart     = 1000.0;
    double   cullEnd       = 5000.0;
    double   lod0Distance  = 500.0;
    double   lod1Distance  = 1500.0;
    double   lod2Distance  = 3000.0;
    double   billboardDist = 5000.0;
    bool     useImpostor   = true;
    bool     castShadow    = true;
    bool     castDynamicShadow = true;
    bool     castStaticShadow  = false;

    // ---- Wind ----
    double windStrength   = 1.0;
    double swayAmount     = 0.5;
    double leafFlutter    = 0.3;
    double gustFrequency  = 0.2;

    // ---- Collision ----
    CollisionPreset collisionPreset = CollisionPreset::NoCollision;
    bool collisionWithWorld = false;

    // ---- Scalability ----
    ImportanceClass importanceClass = ImportanceClass::Grass;
    bool densityScalingEnabled = true;

    // ---- Backward compat helpers ----
    double scaleMin() const { return scaleXMin; }
    double scaleMax() const { return scaleXMax; }
    void SetUniformScale(double mn, double mx) {
        scalingMode=ScalingMode::Uniform;
        scaleXMin=scaleYMin=scaleZMin=mn;
        scaleXMax=scaleYMax=scaleZMax=mx;
    }
};

// ---------------------------------------------------------------------------
// FoliageInstanceV2 — instance with stable ID
// ---------------------------------------------------------------------------
struct FoliageInstanceV2 {
    uint64_t id       = 0;    // stable across export/import
    uint64_t typeId   = 0;    // hash of type name
    std::string typeName;     // FoliageType name
    double x=0,y=0,z=0;      // position
    double nx=0,ny=1,nz=0;   // normal
    double sx=1,sy=1,sz=1;   // scale
    double yaw=0,pitch=0,roll=0; // rotation (radians)
    double age=0;             // procedural age
    uint32_t randomSeed=0;    // per-instance seed
    int32_t  clusterId=-1;    // spatial cluster index
    uint8_t  flags=0;         // selected=1, hidden=2
};

// ---------------------------------------------------------------------------
// InstanceStore — persistent store with spatial index
// ---------------------------------------------------------------------------
struct InstanceStore {
    std::string storeKey{"default"};
    std::string schemaVersion{"2.0"};
    std::vector<FoliageInstanceV2> instances;
    uint64_t nextId = 1;
    uint64_t createdCounter = 0;
    uint64_t modifiedCounter = 0;
    double boundsMin[3] = {0,0,0};
    double boundsMax[3] = {100,100,100};

    uint64_t AllocId() { return nextId++; }
    void Add(const FoliageInstanceV2& inst);
    void AddMany(const std::vector<FoliageInstanceV2>& batch);
    bool Remove(uint64_t id);
    FoliageInstanceV2* Find(uint64_t id);
    const FoliageInstanceV2* Find(uint64_t id) const;
    void Clear();
    size_t Count() const { return instances.size(); }
    void UpdateBounds();
};

} } // namespace foliage::v2

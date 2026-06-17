// foliage_un06 — biome zone system.
// Height/slope-based type assignment: different foliage types at different
// elevations and slopes, with smooth blending between zones.
#pragma once

#include "foliage_type.h"
#include "foliage_terrain.h"
#include <string>
#include <vector>
#include <map>

namespace foliage {

// ---------------------------------------------------------------------------
// BiomeZone — one layer of the biome stack
// ---------------------------------------------------------------------------
struct BiomeZone {
    std::string name;
    // Filter: which terrain areas this zone covers
    double heightMin = -1e9, heightMax = 1e9;
    double slopeMin  = 0.0, slopeMax = 90.0;
    double blendWidth = 5.0;    // smooth transition width at zone boundaries

    // Which foliage types belong to this zone
    std::vector<std::string> typeNames;
    double densityMul = 1.0;    // multiplier for each type's density

    // Check if (h, slope) is inside this zone
    bool Contains(double height, double slope) const;
    // Blend weight [0,1] at boundary (1 = fully inside, 0 = fully outside)
    double BlendWeight(double height, double slope) const;
};

// ---------------------------------------------------------------------------
// BiomeStack — ordered list of zones; later zones override earlier ones
// ---------------------------------------------------------------------------
class BiomeStack {
public:
    void AddZone(const BiomeZone& zone);
    void RemoveZone(const std::string& name);
    void Clear();

    // Get all foliage types that apply at (x,z), with their blend weights
    struct TypeOverride {
        std::string typeName;
        double      weight = 1.0;   // blend weight [0,1]
        double      densityMul = 1.0;
    };
    std::vector<TypeOverride> GetTypesAt(double x, double z, const TerrainSampler& terrain) const;

    // Get all zones
    const std::vector<BiomeZone>& Zones() const { return zones_; }

    // Multi-zone scatter: run scatter per zone across area, merge results
    struct ZoneResult {
        std::string zoneName;
        std::string typeName;
        int         instanceCount = 0;
    };
    static std::vector<FoliageInstance> ScatterAllZones(
        const BiomeStack&       biomes,
        const TerrainSampler&   terrain,
        const ScatterArea&      area,
        const std::map<std::string, FoliageType>& typeRegistry);

private:
    std::vector<BiomeZone> zones_;
};

} // namespace foliage

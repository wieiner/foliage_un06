// foliage_un06 — biome system implementation.
#include "foliage_biome.h"
#include "foliage_scatter.h"
#include <algorithm>
#include <cmath>

namespace foliage {

// ---------------------------------------------------------------------------
// BiomeZone
// ---------------------------------------------------------------------------
bool BiomeZone::Contains(double height, double slope) const {
    return height >= heightMin && height <= heightMax &&
           slope >= slopeMin && slope <= slopeMax;
}

double BiomeZone::BlendWeight(double height, double slope) const {
    // Compute distance to zone boundary (0 = outside, 1 = deep inside)
    double bestDist = 1e9;
    // Height boundaries
    if (height < heightMin) { double d = heightMin - height; if (d < bestDist) bestDist = d; }
    else if (height > heightMax) { double d = height - heightMax; if (d < bestDist) bestDist = d; }
    // Slope boundaries
    if (slope < slopeMin) { double d = slopeMin - slope; if (d < bestDist) bestDist = d; }
    else if (slope > slopeMax) { double d = slope - slopeMax; if (d < bestDist) bestDist = d; }

    if (bestDist > 1e8) return 1.0; // inside all boundaries
    if (blendWidth < 0.01) return bestDist <= 0 ? 1.0 : 0.0;

    double t = 1.0 - bestDist / blendWidth;
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    // Smoothstep transition
    return t * t * (3.0 - 2.0 * t);
}

// ---------------------------------------------------------------------------
// BiomeStack
// ---------------------------------------------------------------------------
void BiomeStack::AddZone(const BiomeZone& zone) {
    for (auto& z : zones_) { if (z.name == zone.name) { z = zone; return; } }
    zones_.push_back(zone);
}

void BiomeStack::RemoveZone(const std::string& name) {
    zones_.erase(std::remove_if(zones_.begin(), zones_.end(),
        [&](const BiomeZone& z) { return z.name == name; }), zones_.end());
}

void BiomeStack::Clear() { zones_.clear(); }

std::vector<BiomeStack::TypeOverride> BiomeStack::GetTypesAt(
    double x, double z, const TerrainSampler& terrain) const
{
    std::vector<TypeOverride> result;
    double h = terrain.Height(x, z);
    double slope = terrain.Slope(x, z);

    for (const auto& zone : zones_) {
        double w = zone.BlendWeight(h, slope);
        if (w <= 0.0) continue;
        for (const auto& tn : zone.typeNames) {
            TypeOverride to;
            to.typeName = tn;
            to.weight = w;
            to.densityMul = zone.densityMul * w;
            // If same type already added from a lower zone, max the weight
            bool found = false;
            for (auto& existing : result) {
                if (existing.typeName == tn) {
                    existing.weight = std::max(existing.weight, w);
                    existing.densityMul = std::max(existing.densityMul, zone.densityMul * w);
                    found = true;
                    break;
                }
            }
            if (!found) result.push_back(to);
        }
    }
    return result;
}

std::vector<FoliageInstance> BiomeStack::ScatterAllZones(
    const BiomeStack&       biomes,
    const TerrainSampler&   terrain,
    const ScatterArea&      area,
    const std::map<std::string, FoliageType>& typeRegistry)
{
    std::vector<FoliageInstance> allInstances;
    if (biomes.zones_.empty()) return allInstances;

    // For each zone, scatter each of its types over the whole area
    // (filtering happens per-instance in the scatter algorithm)
    for (const auto& zone : biomes.zones_) {
        for (const auto& tn : zone.typeNames) {
            auto it = typeRegistry.find(tn);
            if (it == typeRegistry.end()) continue;
            FoliageType ft = it->second;
            ft.density *= zone.densityMul;

            auto instances = PoissonDiscScatter::Scatter(terrain, ft, area, 50000, 30);
            // Filter: only keep instances that pass this zone's blend
            std::vector<FoliageInstance> filtered;
            for (auto& inst : instances) {
                double w = zone.BlendWeight(inst.y, terrain.Slope(inst.x, inst.z));
                if (w > 0.1) {
                    // Blend weight affects scale (weaker at boundaries)
                    // and survival probability
                    FoliagePrng prng(ft.seed ^ (unsigned)(inst.x * 1000 + inst.z));
                    if (prng.NextDouble() < w) {
                        inst.scaleX *= (0.7 + 0.3 * w);
                        inst.scaleY *= (0.7 + 0.3 * w);
                        inst.scaleZ *= (0.7 + 0.3 * w);
                        filtered.push_back(inst);
                    }
                }
            }
            allInstances.insert(allInstances.end(), filtered.begin(), filtered.end());
        }
    }
    return allInstances;
}

} // namespace foliage

// foliage_un06 — ecosystem simulation implementation.
#include "foliage_simulate.h"
#include "foliage_query.h"
#include "foliage_terrain.h"
#include <algorithm>
#include <cmath>

namespace foliage {

EcoSimResult EcoSim::Compete(
    const std::vector<FoliageInstance>& instances,
    const SimulationParams&             params,
    double                              basePriority,
    bool                                scaleBonus,
    double                              scaleWeight)
{
    EcoSimResult result;
    result.total = static_cast<std::uint32_t>(instances.size());

    if (instances.empty()) return result;

    // Compute priority per instance
    struct Prio { int idx; double priority; bool alive; };
    std::vector<Prio> prios(instances.size());
    for (size_t i = 0; i < instances.size(); ++i) {
        double p = basePriority;
        if (scaleBonus) {
            double avgScale = (instances[i].scaleX + instances[i].scaleY + instances[i].scaleZ) / 3.0;
            p += avgScale * scaleWeight;
        }
        prios[i] = {static_cast<int>(i), p, true};
    }

    // Build spatial index
    if (instances.empty()) return result;
    double xMin = instances[0].x, xMax = instances[0].x;
    double zMin = instances[0].z, zMax = instances[0].z;
    for (auto& inst : instances) {
        if (inst.x < xMin) xMin = inst.x; if (inst.x > xMax) xMax = inst.x;
        if (inst.z < zMin) zMin = inst.z; if (inst.z > zMax) zMax = inst.z;
    }
    SpatialIndex idx(std::max(1.0, params.collisionRadius));
    idx.Build(instances, xMin, zMin, xMax, zMax);

    double cr2 = params.collisionRadius * params.collisionRadius;
    double sr2 = params.shadeRadius * params.shadeRadius;

    // Multiple passes
    for (int step = 0; step < params.numSteps; ++step) {
        for (size_t i = 0; i < instances.size(); ++i) {
            if (!prios[i].alive) continue;
            auto hits = idx.RadiusSearch(instances[i].x, instances[i].z,
                                         params.shadeRadius > 0 ? params.shadeRadius : params.collisionRadius,
                                         100);
            for (auto& hit : hits) {
                int j = hit.index;
                if (j <= static_cast<int>(i)) continue; // each pair once
                if (!prios[j].alive) continue;

                double dx = instances[i].x - instances[j].x;
                double dz = instances[i].z - instances[j].z;
                double d2 = dx*dx + dz*dz;

                // Collision check
                if (d2 <= cr2) {
                    if (prios[i].priority >= prios[j].priority) {
                        prios[j].alive = false;
                    } else {
                        prios[i].alive = false;
                    }
                }
                // Shade check (only if collision didn't resolve)
                else if (params.shadeRadius > 0 && d2 <= sr2 && prios[i].alive && prios[j].alive) {
                    // Lower priority loses in shade competition
                    if (prios[i].priority < prios[j].priority) {
                        prios[i].alive = false;
                    } else {
                        prios[j].alive = false;
                    }
                }
            }
        }
    }

    // Collect survivors
    for (size_t i = 0; i < instances.size(); ++i) {
        if (prios[i].alive)
            result.survivors.push_back(instances[i]);
        else
            ++result.removed;
    }

    return result;
}

void EcoSim::Age(std::vector<FoliageInstance>& instances,
                 double growthRate, double targetScale)
{
    for (auto& inst : instances) {
        double avg = (inst.scaleX + inst.scaleY + inst.scaleZ) / 3.0;
        if (avg < targetScale) {
            double factor = 1.0 + growthRate;
            inst.scaleX = std::min(inst.scaleX * factor, targetScale * (inst.scaleX / avg));
            inst.scaleY = std::min(inst.scaleY * factor, targetScale * (inst.scaleY / avg));
            inst.scaleZ = std::min(inst.scaleZ * factor, targetScale * (inst.scaleZ / avg));
        }
    }
}

std::vector<FoliageInstance> EcoSim::Spread(
    const std::vector<FoliageInstance>& parents,
    const TerrainSampler&              terrain,
    const FoliageType&                 type,
    int                                 seedsPerStep,
    double                              spreadDist,
    double                              spreadVariance,
    std::uint32_t                       seed)
{
    std::vector<FoliageInstance> children;
    if (parents.empty()) return children;

    FoliagePrng prng(seed);
    double r2 = spreadDist * spreadDist;

    // Build spatial index of parents for overlap avoidance
    double xMin = parents[0].x, xMax = parents[0].x;
    double zMin = parents[0].z, zMax = parents[0].z;
    for (auto& p : parents) {
        if (p.x < xMin) xMin = p.x; if (p.x > xMax) xMax = p.x;
        if (p.z < zMin) zMin = p.z; if (p.z > zMax) zMax = p.z;
    }
    // Extend bounds for potential spread
    xMin -= spreadDist; xMax += spreadDist;
    zMin -= spreadDist; zMax += spreadDist;

    for (const auto& parent : parents) {
        for (int s = 0; s < seedsPerStep; ++s) {
            double angle = prng.NextRange(0.0, 6.283185307);
            double dist  = prng.NextRange(
                std::max(0.0, spreadDist - spreadVariance),
                spreadDist + spreadVariance);
            double wx = parent.x + dist * std::cos(angle);
            double wz = parent.z + dist * std::sin(angle);
            double wy = terrain.Height(wx, wz);

            // Basic filter
            if (wy < type.heightMin || wy > type.heightMax) continue;
            if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

            FoliageInstance child;
            child.x = wx; child.y = wy; child.z = wz;
            double scale = prng.NextRange(type.scaleMin * 0.5, type.scaleMin); // young = smaller
            child.scaleX = scale;
            child.scaleY = type.uniformScale ? scale : prng.NextRange(type.scaleMin * 0.5, type.scaleMin);
            child.scaleZ = type.uniformScale ? scale : prng.NextRange(type.scaleMin * 0.5, type.scaleMin);
            if (type.alignToNormal) terrain.Normal(wx, wz, &child.nx, &child.ny, &child.nz);
            if (type.randomRotation) child.yaw = prng.NextRange(0.0, 6.283185307);

            children.push_back(child);
        }
    }
    return children;
}

} // namespace foliage

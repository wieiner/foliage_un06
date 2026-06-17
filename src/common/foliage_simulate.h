// foliage_un06 — competition / ecosystem simulation.
// Replicates UE's ProceduralFoliageTile simulation: collision radius,
// shade radius, overlap priority → cull overlapping instances.
#pragma once

#include "foliage_type.h"
#include "foliage_prng.h"
#include <vector>
#include <cstdint>

namespace foliage {

// ---------------------------------------------------------------------------
// SimulationParams — controls the competition pass
// ---------------------------------------------------------------------------
struct SimulationParams {
    double collisionRadius = 10.0;    // instances closer than this collide
    double shadeRadius     = 25.0;    // shade influence radius (0 = disabled)
    int    numSteps        = 3;       // simulation iterations
    bool   removeLosers    = true;    // actually remove overlapped instances
    std::uint32_t seed     = 1337;
};

// ---------------------------------------------------------------------------
// EcoSim — removes overlapping instances using priority-based competition.
//
// For each pair within collisionRadius, the one with higher OverlapPriority
// survives.  Tie-break: larger scale wins, then coin flip.
// ---------------------------------------------------------------------------
struct EcoSimResult {
    std::vector<FoliageInstance> survivors;
    std::uint32_t removed = 0;
    std::uint32_t total   = 0;
};

class EcoSim {
public:
    // Run competition on `instances`. Assigns priority:
    //   basePriority + (scaleBonus ? avgScale * scaleWeight : 0)
    // Instances with HIGHER priority survive collisions.
    static EcoSimResult Compete(
        const std::vector<FoliageInstance>& instances,
        const SimulationParams&             params,
        double                              basePriority = 1.0,
        bool                                scaleBonus   = true,
        double                              scaleWeight  = 2.0);

    // Age all instances: increase scale toward `targetScale` by `growthRate`
    static void Age(std::vector<FoliageInstance>& instances,
                    double growthRate = 0.05,
                    double targetScale = 2.0);

    // Spread: for each instance, spawn `seedsPerStep` new instances
    // within `spreadDist` ± variance.  Respects terrain.
    static std::vector<FoliageInstance> Spread(
        const std::vector<FoliageInstance>& parents,
        const TerrainSampler&              terrain,
        const FoliageType&                 type,
        int                                 seedsPerStep = 2,
        double                              spreadDist   = 15.0,
        double                              spreadVariance = 5.0,
        std::uint32_t                       seed = 1337);
};

} // namespace foliage

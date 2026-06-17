// foliage_un06 — Poisson-disc foliage scatter algorithm.
// Deterministic: same FoliageType + seed → same output everywhere.
#pragma once

#include "foliage_type.h"
#include "foliage_prng.h"
#include "foliage_terrain.h"

#include <cmath>
#include <vector>

namespace foliage {

// ---------------------------------------------------------------------------
// PoissonDiscScatter — priority-queue–based dart throwing
//
// Uses Bridson's algorithm (fast Poisson disc sampling).  The seed is
// consumed deterministically via FoliagePrng.  Every candidate goes
// through slope / height / alignment filters before being accepted.
// ---------------------------------------------------------------------------
class PoissonDiscScatter {
public:
    struct Stats {
        std::uint32_t candidates = 0;
        std::uint32_t kept       = 0;
        std::uint32_t slopeRej   = 0;
        std::uint32_t heightRej  = 0;
        double        avgScale   = 0.0;
        double        firstX = 0.0, firstZ = 0.0, firstScale = 0.0;
        bool          hasFirst   = false;
    };

    // Run scatter.  Returns kept instances.  `maxInstances` caps output
    // (0 = unlimited).  `k` = samples-before-rejection (default 30).
    static std::vector<FoliageInstance> Scatter(
        const TerrainSampler&      terrain,
        const FoliageType&         type,
        const ScatterArea&         area,
        std::uint32_t              maxInstances = 0,
        int                        k = 30);

    // Run scatter and also collect statistics.
    static ScatterResult ScatterWithStats(
        const TerrainSampler&      terrain,
        const FoliageType&         type,
        const ScatterArea&         area,
        std::uint32_t              maxInstances = 0,
        int                        k = 30);

private:
    struct GridCell;

    static double EffectiveRadius(const FoliageType& type);
    static bool   PassesFilters(const FoliageInstance& inst,
                                const TerrainSampler& terrain,
                                const FoliageType& type);
};

} // namespace foliage

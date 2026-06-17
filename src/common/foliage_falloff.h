// foliage_un06 — density falloff / edge feathering.
// Probabilistically removes instances near area boundaries.
#pragma once

#include "foliage_type.h"
#include "foliage_prng.h"
#include <vector>
#include <cmath>

namespace foliage {

// ---------------------------------------------------------------------------
// DensityFalloff — apply a distance-to-edge probability curve
//
// falloffWidth = distance from edge where falloff begins
// falloffType  = "linear" | "smoothstep" | "sqrt" | "inverse"
// preserveCenter = fraction [0,1] of center area where no removal happens
// ---------------------------------------------------------------------------
struct FalloffParams {
    double falloffWidth   = 20.0;    // edge zone width
    double preserveCenter = 0.0;     // [0,1] center preservation fraction
    std::string falloffType = "smoothstep";
    std::uint32_t seed = 1337;
};

// Returns survivors after probabilistic edge removal.
// Instances closer to the edge are more likely to be removed.
std::vector<FoliageInstance> ApplyDensityFalloff(
    const std::vector<FoliageInstance>& instances,
    const ScatterArea&                  area,
    const FalloffParams&                params);

// Apply a custom falloff curve defined by control points [{distance,probability},...]
// distance = normalized [0,1] from edge; probability = survival chance [0,1]
std::vector<FoliageInstance> ApplyFalloffCurve(
    const std::vector<FoliageInstance>& instances,
    const ScatterArea&                  area,
    const std::vector<std::pair<double,double>>& curve,
    std::uint32_t seed = 1337);

} // namespace foliage

// foliage_un06 — brush paint / erase operations.
#pragma once

#include "foliage_type.h"
#include "foliage_prng.h"
#include "foliage_terrain.h"

#include <vector>

namespace foliage {

// ---------------------------------------------------------------------------
// FoliagePaint — interactive brush-based paint/erase
// ---------------------------------------------------------------------------
class FoliagePaint {
public:
    // Paint `brush` instances using `type` params.  The brush scatters
    // random candidates inside the circle but respects Poisson spacing
    // *among newly added instances* (existing instances are ignored).
    //
    // Existing instances are passed so we can seed the PRNG from type.seed
    // plus the existing count (avoiding duplicates).
    static std::vector<FoliageInstance> PaintBrush(
        const TerrainSampler&               terrain,
        const FoliageType&                  type,
        const BrushParams&                  brush,
        const std::vector<FoliageInstance>& existing = {});

    // Erase instances whose XZ projection falls inside `brush.radius`.
    // Returns the filtered list (survivors).  Does NOT modify `instances`
    // in-place; callers should replace.
    static std::vector<FoliageInstance> EraseBrush(
        const BrushParams&                  brush,
        const std::vector<FoliageInstance>& instances);
};

} // namespace foliage

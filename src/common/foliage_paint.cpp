// foliage_un06 — brush paint / erase implementation.
#include "foliage_paint.h"
#include "foliage_prng.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace foliage {

std::vector<FoliageInstance> FoliagePaint::PaintBrush(
    const TerrainSampler&               terrain,
    const FoliageType&                  type,
    const BrushParams&                  brush,
    const std::vector<FoliageInstance>& /*existing*/)
{
    std::vector<FoliageInstance> added;

    // Number of candidates to try
    double area = M_PI * brush.radius * brush.radius;
    int target = std::max(1, static_cast<int>(area * type.density * brush.densityMul));
    if (target > 10000) target = 10000;

    // Seed = type.seed XOR brush position hash (deterministic for same brush location)
    std::uint64_t bx = static_cast<std::uint64_t>(std::abs(brush.centerX) * 1000.0);
    std::uint64_t bz = static_cast<std::uint64_t>(std::abs(brush.centerZ) * 1000.0);
    std::uint64_t brushSeed = (type.seed * 6364136223846793005ULL) ^ (bx * 0x9E3779B9ULL) ^ (bz * 0x517CC1B7ULL);
    FoliagePrng prng(brushSeed);

    // Poisson disc inside a circle: same algorithm, smaller domain
    double r = 1.0 / std::sqrt(type.density * brush.densityMul * 1.1547);
    if (r < 1.0) r = 1.0;
    double r2 = r * r;

    // Simple grid for spatial rejection within the brush
    struct GridCell { double x, z; bool used = false; };
    double cellSize = r;
    int gs = static_cast<int>(brush.radius * 2.0 / cellSize) + 1;
    std::vector<GridCell> grid(gs * gs);
    auto cellXZ = [&](double x, double z) -> int {
        int cx = static_cast<int>((x - brush.centerX + brush.radius) / cellSize);
        int cz = static_cast<int>((z - brush.centerZ + brush.radius) / cellSize);
        if (cx < 0 || cx >= gs || cz < 0 || cz >= gs) return -1;
        return cz * gs + cx;
    };

    std::vector<std::pair<double,double>> active;

    // Place first
    for (int at = 0; at < 30; ++at) {
        double angle = prng.NextRange(0.0, 2.0 * M_PI);
        double dist  = prng.NextRange(0.0, brush.radius);
        double wx = brush.centerX + dist * std::cos(angle);
        double wz = brush.centerZ + dist * std::sin(angle);
        double wy = terrain.Height(wx, wz);

        FoliageInstance inst;
        inst.x = wx; inst.y = wy; inst.z = wz;

        // Filters
        if (wy < type.heightMin || wy > type.heightMax) continue;
        if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

        double scale = prng.NextRange(type.scaleMin, type.scaleMax);
        inst.scaleX = scale;
        inst.scaleY = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);
        inst.scaleZ = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);

        if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
        if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);

        added.push_back(inst);
        int ci = cellXZ(wx, wz);
        if (ci >= 0) { grid[ci].x = wx; grid[ci].z = wz; grid[ci].used = true; }
        active.push_back({wx, wz});
        break;
    }

    // Fill
    while (!active.empty() && static_cast<int>(added.size()) < target) {
        int ai = static_cast<int>(prng.NextU64() % active.size());
        auto [cx, cz] = active[ai];
        bool found = false;

        for (int k = 0; k < 20; ++k) {
            double angle = prng.NextRange(0.0, 2.0 * M_PI);
            double dist  = prng.NextRange(r, 2.0 * r);
            double wx = cx + dist * std::cos(angle);
            double wz = cz + dist * std::sin(angle);

            // Within brush?
            double dx = wx - brush.centerX, dz = wz - brush.centerZ;
            if (dx * dx + dz * dz > brush.radius * brush.radius) continue;

            // Too close to existing brush points?
            int ci = cellXZ(wx, wz);
            if (ci < 0) continue;
            bool tooClose = false;
            for (int dci = -1; dci <= 1 && !tooClose; ++dci) {
                for (int dcj = -1; dcj <= 1 && !tooClose; ++dcj) {
                    int nci = ci + dci * gs + dcj;
                    if (nci < 0 || nci >= static_cast<int>(grid.size())) continue;
                    if (!grid[nci].used) continue;
                    double dx2 = wx - grid[nci].x, dz2 = wz - grid[nci].z;
                    if (dx2 * dx2 + dz2 * dz2 < r2) tooClose = true;
                }
            }
            if (tooClose) continue;

            double wy = terrain.Height(wx, wz);
            if (wy < type.heightMin || wy > type.heightMax) continue;
            if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

            FoliageInstance inst;
            inst.x = wx; inst.y = wy; inst.z = wz;
            double scale = prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleX = scale;
            inst.scaleY = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleZ = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);

            if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
            if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);

            added.push_back(inst);
            grid[ci].x = wx; grid[ci].z = wz; grid[ci].used = true;
            active.push_back({wx, wz});
            found = true;
            break;
        }
        if (!found) {
            active[ai] = active.back();
            active.pop_back();
        }
    }

    return added;
}

std::vector<FoliageInstance> FoliagePaint::EraseBrush(
    const BrushParams&                  brush,
    const std::vector<FoliageInstance>& instances)
{
    double r2 = brush.radius * brush.radius;
    std::vector<FoliageInstance> survivors;
    survivors.reserve(instances.size());
    for (const auto& inst : instances) {
        double dx = inst.x - brush.centerX;
        double dz = inst.z - brush.centerZ;
        if (dx * dx + dz * dz > r2) {
            survivors.push_back(inst);
        }
    }
    return survivors;
}

} // namespace foliage

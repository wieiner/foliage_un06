// foliage_un06 — Poisson-disc foliage scatter implementation.
#include "foliage_scatter.h"

#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace foliage {

namespace {

constexpr double kDegToRad = M_PI / 180.0;

struct Point2D {
    double x, z;
};

// Simple 2D grid for fast spatial queries during Poisson disc sampling.
struct PoissonGrid {
    double  cellSize;
    double  invCellSize;
    int     cols, rows;
    std::vector<int> cells; // -1 = empty

    PoissonGrid(double w, double d, double cellSz)
        : cellSize(cellSz), invCellSize(1.0 / cellSz)
        , cols(static_cast<int>(w * invCellSize) + 1)
        , rows(static_cast<int>(d * invCellSize) + 1)
        , cells(cols * rows, -1) {}

    int  CellIndex(double x, double z) const {
        int cx = static_cast<int>(x * invCellSize);
        int cz = static_cast<int>(z * invCellSize);
        if (cx < 0) cx = 0; if (cx >= cols) cx = cols - 1;
        if (cz < 0) cz = 0; if (cz >= rows) cz = rows - 1;
        return cz * cols + cx;
    }

    void Insert(int instanceIdx, double x, double z) {
        cells[CellIndex(x, z)] = instanceIdx;
    }

    // Check if a candidate point is too close to any existing instance.
    // Searches the cell and 8 neighbours.
    bool TooClose(double x, double z, double minDist,
                  const std::vector<FoliageInstance>& instances) const {
        int cx = static_cast<int>(x * invCellSize);
        int cz = static_cast<int>(z * invCellSize);
        double d2 = minDist * minDist;
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nc = (cz + dz) * cols + (cx + dx);
                if (nc < 0 || nc >= static_cast<int>(cells.size())) continue;
                int idx = cells[nc];
                if (idx < 0) continue;
                const auto& inst = instances[idx];
                double dxv = x - inst.x;
                double dzv = z - inst.z;
                if (dxv * dxv + dzv * dzv < d2) return true;
            }
        }
        return false;
    }
};

} // namespace

double PoissonDiscScatter::EffectiveRadius(const FoliageType& type) {
    if (type.radius > 0.0) return type.radius;
    // Auto-radius from density:  r = 1 / sqrt(density * 2/√3)
    // Hexagonal packing gives ~0.9069 coverage; the formula below
    // produces spacing that avoids obvious clumping.
    if (type.density <= 0.0) return 5.0;
    return 1.0 / std::sqrt(type.density * 1.1547); // 2/√3 ≈ 1.1547
}

bool PoissonDiscScatter::PassesFilters(
    const FoliageInstance& inst,
    const TerrainSampler&  terrain,
    const FoliageType&     type)
{
    // Height filter
    if (inst.y < type.heightMin || inst.y > type.heightMax) {
        return false;
    }
    // Slope filter (disabled when slopeMax < 0)
    if (type.slopeMax >= 0.0) {
        double slope = terrain.Slope(inst.x, inst.z);
        if (slope > type.slopeMax) return false;
    }
    return true;
}

std::vector<FoliageInstance> PoissonDiscScatter::Scatter(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const ScatterArea&     area,
    std::uint32_t          maxInstances,
    int                    k)
{
    auto result = ScatterWithStats(terrain, type, area, maxInstances, k);
    return std::move(result.instances);
}

ScatterResult PoissonDiscScatter::ScatterWithStats(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const ScatterArea&     area,
    std::uint32_t          maxInstances,
    int                    k)
{
    ScatterResult result;
    result.foliageTypeName = type.name;
    result.area  = area;
    result.seed  = type.seed;

    FoliagePrng prng(type.seed);
    double r = EffectiveRadius(type);
    double r2 = r * r;

    PoissonGrid grid(area.width, area.depth, r);
    std::vector<FoliageInstance>& instances = result.instances;
    std::vector<Point2D> active; // active list

    Stats st{};

    // --- Place first point ---
    {
        double x0, z0;
        for (int attempt = 0; attempt < 30; ++attempt) {
            prng.NextPoint(area.width, area.depth, &x0, &z0);
            FoliageInstance inst;
            inst.x = area.originX + x0;
            inst.z = area.originZ + z0;
            inst.y = terrain.Height(inst.x, inst.z);
            if (inst.y < -1e8) continue; // void area

            if (!st.hasFirst) {
                st.firstX = inst.x; st.firstZ = inst.z; st.hasFirst = true;
            }

            if (!PassesFilters(inst, terrain, type)) {
                ++st.candidates;
                continue;
            }

            // Apply transforms
            double scale = prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleX = scale;
            inst.scaleY = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleZ = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);

            if (type.alignToNormal) {
                terrain.Normal(inst.x, inst.z, &inst.nx, &inst.ny, &inst.nz);
            }
            if (type.randomRotation) {
                inst.yaw = prng.NextRange(type.rotationMin, type.rotationMax) * kDegToRad;
            }

            if (!st.hasFirst) {
                st.firstScale = scale;
            }

            int idx = static_cast<int>(instances.size());
            instances.push_back(inst);
            grid.Insert(idx, x0, z0);
            active.push_back({x0, z0});
            st.avgScale += scale;
            ++st.kept;
            ++st.candidates;
            break;
        }
    }

    // --- Poisson disc fill ---
    while (!active.empty() &&
           (maxInstances == 0 || st.kept < maxInstances)) {
        // Pick random active point
        int ai = static_cast<int>(prng.NextU64() % active.size());
        Point2D center = active[ai];
        bool found = false;

        for (int i = 0; i < k; ++i) {
            // Random point in annulus [r, 2r]
            double angle = prng.NextRange(0.0, 2.0 * M_PI);
            double dist  = prng.NextRange(r, 2.0 * r);
            double nx = center.x + dist * std::cos(angle);
            double nz = center.z + dist * std::sin(angle);

            ++st.candidates;

            // Bounds check
            if (nx < 0.0 || nx >= area.width || nz < 0.0 || nz >= area.depth)
                continue;

            // Too close to existing?
            if (grid.TooClose(nx, nz, r, instances))
                continue;

            // World position
            double wx = area.originX + nx;
            double wz = area.originZ + nz;
            double wy = terrain.Height(wx, wz);

            FoliageInstance inst;
            inst.x = wx; inst.y = wy; inst.z = wz;

            if (!PassesFilters(inst, terrain, type)) {
                if (wy < type.heightMin || wy > type.heightMax) ++st.heightRej;
                else if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax)
                    ++st.slopeRej;
                continue;
            }

            // Transform
            double scale = prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleX = scale;
            inst.scaleY = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);
            inst.scaleZ = type.uniformScale ? scale : prng.NextRange(type.scaleMin, type.scaleMax);

            if (type.alignToNormal) {
                terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
            }
            if (type.randomRotation) {
                inst.yaw = prng.NextRange(type.rotationMin, type.rotationMax) * kDegToRad;
            }

            int idx = static_cast<int>(instances.size());
            instances.push_back(inst);
            grid.Insert(idx, nx, nz);
            active.push_back({nx, nz});
            st.avgScale += scale;
            ++st.kept;
            found = true;
            break;
        }

        if (!found) {
            // Remove this active point — it's saturated
            active[ai] = active.back();
            active.pop_back();
        }
    }

    if (st.kept > 0) {
        st.avgScale /= st.kept;
    }

    result.candidates = st.candidates;
    result.kept       = st.kept;
    result.avgScale   = st.avgScale;
    return result;
}

} // namespace foliage

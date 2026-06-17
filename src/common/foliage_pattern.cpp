// foliage_un06 — placement patterns implementation.
#include "foliage_pattern.h"
#include "foliage_scatter.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace foliage {

// ---------------------------------------------------------------------------
// Radial scatter
// ---------------------------------------------------------------------------
std::vector<FoliageInstance> ScatterRadial(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const RadialParams&    params)
{
    std::vector<FoliageInstance> result;
    FoliagePrng prng(type.seed ^ 0x123457);
    double area_ring = M_PI * (params.outerRadius*params.outerRadius - params.innerRadius*params.innerRadius);
    int target = std::max(1, (int)(area_ring * type.density * params.densityMul));
    if (target > 50000) target = 50000;
    double minR2 = params.innerRadius * params.innerRadius;
    double maxR2 = params.outerRadius * params.outerRadius;
    if (minR2 >= maxR2) return result;

    // Simple rejection-based placement in annulus/full disc
    for (int attempt = 0; attempt < target * 3 && (int)result.size() < target; ++attempt) {
        double r = std::sqrt(prng.NextRange(minR2, maxR2));
        double angle = prng.NextRange(0.0, 2.0 * M_PI);
        double wx = params.centerX + r * std::cos(angle);
        double wz = params.centerZ + r * std::sin(angle);
        double wy = terrain.Height(wx, wz);

        if (wy < type.heightMin || wy > type.heightMax) continue;
        if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

        // If rings only, skip fill areas between rings
        if (!params.fillDisc && params.ringCount > 1) {
            double ringWidth = (params.outerRadius - params.innerRadius) / params.ringCount;
            double localR = (r - params.innerRadius);
            int ringIdx = (int)(localR / ringWidth);
            double ringCenter = (ringIdx + 0.5) * ringWidth;
            double distFromRing = std::abs(localR - ringCenter);
            if (distFromRing > ringWidth * 0.2) continue; // only near ring centers
        }

        FoliageInstance inst;
        inst.x = wx; inst.y = wy; inst.z = wz;
        inst.scaleX = inst.scaleY = inst.scaleZ = prng.NextRange(type.scaleMin, type.scaleMax);
        if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
        if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);
        result.push_back(inst);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Along-path scatter
// ---------------------------------------------------------------------------
std::vector<FoliageInstance> ScatterAlongPath(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const PathParams&      params)
{
    std::vector<FoliageInstance> result;
    if (params.points.size() < 2) return result;
    FoliagePrng prng(params.seed);

    // Compute total path length
    double totalLen = 0.0;
    std::vector<double> segLengths;
    size_t n = params.closed ? params.points.size() : params.points.size() - 1;
    for (size_t i = 0; i < n; ++i) {
        auto& a = params.points[i];
        auto& b = params.points[(i + 1) % params.points.size()];
        double dx = b.first - a.first, dz = b.second - a.second;
        double len = std::sqrt(dx*dx + dz*dz);
        segLengths.push_back(len);
        totalLen += len;
    }

    int target = std::max(1, (int)(totalLen * params.density));
    if (target > 50000) target = 50000;

    for (int i = 0; i < target; ++i) {
        double t = prng.NextRange(0.0, totalLen);
        // Find segment
        double accum = 0;
        size_t segIdx = 0;
        for (; segIdx < segLengths.size(); ++segIdx) {
            if (accum + segLengths[segIdx] >= t) break;
            accum += segLengths[segIdx];
        }
        if (segIdx >= segLengths.size()) segIdx = segLengths.size() - 1;
        double localT = (segLengths[segIdx] > 0.001) ? (t - accum) / segLengths[segIdx] : 0.5;

        auto& a = params.points[segIdx];
        auto& b = params.points[(segIdx + 1) % params.points.size()];
        double px = a.first + localT * (b.first - a.first);
        double pz = a.second + localT * (b.second - a.second);

        // Perpendicular offset
        double dx = b.first - a.first, dz = b.second - a.second;
        double len = std::sqrt(dx*dx + dz*dz) + 0.0001;
        double nx = -dz / len, nz = dx / len; // perpendicular
        double offset = prng.NextRange(-params.width, params.width);
        double wx = px + nx * offset;
        double wz = pz + nz * offset;
        double wy = terrain.Height(wx, wz);

        if (wy < type.heightMin || wy > type.heightMax) continue;
        if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

        FoliageInstance inst;
        inst.x = wx; inst.y = wy; inst.z = wz;
        inst.scaleX = inst.scaleY = inst.scaleZ = prng.NextRange(type.scaleMin, type.scaleMax);
        if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
        if (params.parallel) inst.yaw = std::atan2(dz, dx); // align to path direction
        else if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);
        result.push_back(inst);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Clumped scatter (Thomas cluster process)
// ---------------------------------------------------------------------------
std::vector<FoliageInstance> ScatterClumped(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const ScatterArea&     area,
    const ClumpParams&     params)
{
    std::vector<FoliageInstance> result;
    FoliagePrng prng(params.seed);

    // Place parent clusters via loose Poisson disc
    double parentR = std::sqrt(area.width * area.depth / (params.parentCount * 3.14159));
    std::vector<std::pair<double,double>> parents;
    for (int i = 0; i < params.parentCount && i < 500; ++i) {
        for (int att = 0; att < 20; ++att) {
            double px = area.originX + prng.NextRange(0, area.width);
            double pz = area.originZ + prng.NextRange(0, area.depth);
            double wy = terrain.Height(px, pz);
            if (wy < type.heightMin || wy > type.heightMax) continue;
            bool tooClose = false;
            for (auto& p : parents) {
                double dx = px - p.first, dz = pz - p.second;
                if (dx*dx + dz*dz < parentR*parentR*0.25) { tooClose = true; break; }
            }
            if (!tooClose) { parents.push_back({px, pz}); break; }
        }
    }

    // For each parent, place children with Gaussian falloff
    // Box-Muller for Gaussian distribution
    for (auto& parent : parents) {
        int nChildren = std::max(1, params.childrenPerParent +
            (int)(prng.NextRange(-2, 2))); // random variation
        for (int c = 0; c < nChildren; ++c) {
            // Box-Muller transform
            double u1 = prng.NextDouble(), u2 = prng.NextDouble();
            if (u1 < 0.001) u1 = 0.001;
            double r = params.childSpread * std::sqrt(-2.0 * std::log(u1));
            double angle = 2.0 * M_PI * u2;
            double wx = parent.first + r * std::cos(angle);
            double wz = parent.second + r * std::sin(angle);

            // Clamp to cluster radius
            r = std::sqrt((wx-parent.first)*(wx-parent.first) + (wz-parent.second)*(wz-parent.second));
            if (r > params.clusterRadius) {
                double scale = params.clusterRadius / r;
                wx = parent.first + (wx - parent.first) * scale;
                wz = parent.second + (wz - parent.second) * scale;
            }

            double wy = terrain.Height(wx, wz);
            if (wy < type.heightMin || wy > type.heightMax) continue;
            if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

            FoliageInstance inst;
            inst.x = wx; inst.y = wy; inst.z = wz;
            inst.scaleX = inst.scaleY = inst.scaleZ = prng.NextRange(type.scaleMin, type.scaleMax);
            // Children near center are larger
            double distFromCenter = r / params.clusterRadius;
            double sizeBonus = 1.0 + (1.0 - distFromCenter) * 0.3;
            inst.scaleX *= sizeBonus; inst.scaleY *= sizeBonus; inst.scaleZ *= sizeBonus;
            if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
            if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);
            result.push_back(inst);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Brush shapes
// ---------------------------------------------------------------------------
bool PointInBrush(double x, double z, const BrushShapeParams& brush) {
    double dx = x - brush.cx, dz = z - brush.cz;
    double d2 = dx*dx + dz*dz;

    switch (brush.shapeType) {
    case BrushShapeType::Circle:
        return d2 <= brush.radius * brush.radius;

    case BrushShapeType::Donut: {
        double r2 = brush.radius * brush.radius;
        double inner2 = brush.innerRadius * brush.innerRadius;
        return d2 <= r2 && d2 >= inner2;
    }
    case BrushShapeType::Square:
        return std::abs(dx) <= brush.halfWidth && std::abs(dz) <= brush.halfWidth;

    case BrushShapeType::Line: {
        // Distance to line segment
        double lx = brush.lineEndX - brush.lineStartX;
        double lz = brush.lineEndZ - brush.lineStartZ;
        double len2 = lx*lx + lz*lz;
        if (len2 < 0.0001) return d2 <= brush.radius * brush.radius;
        double t = ((x - brush.lineStartX) * lx + (z - brush.lineStartZ) * lz) / len2;
        if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
        double cx = brush.lineStartX + t * lx;
        double cz = brush.lineStartZ + t * lz;
        double dist2 = (x-cx)*(x-cx) + (z-cz)*(z-cz);
        return dist2 <= brush.radius * brush.radius;
    }
    case BrushShapeType::Gaussian: {
        double sigma2 = 2.0 * brush.gaussianSigma * brush.gaussianSigma;
        double g = std::exp(-d2 / sigma2);
        // Probabilistic: g > threshold
        return g > 0.05; // 2 sigma cutoff
    }
    }
    return false;
}

std::vector<FoliageInstance> ScatterInBrushShape(
    const TerrainSampler&    terrain,
    const FoliageType&       type,
    const BrushShapeParams&  brush,
    const BrushParams&       density,
    int                      maxInstances)
{
    std::vector<FoliageInstance> result;
    FoliagePrng prng(type.seed ^ 0xCAFEBABE);

    double area = M_PI * brush.radius * brush.radius;
    if (brush.shapeType == BrushShapeType::Square) area = 4.0 * brush.halfWidth * brush.halfWidth;
    int target = std::max(1, std::min(maxInstances, (int)(area * type.density * density.densityMul)));

    double searchR = brush.radius;
    if (brush.shapeType == BrushShapeType::Square) searchR = brush.halfWidth * 1.42;
    if (brush.shapeType == BrushShapeType::Line) {
        double lx = brush.lineEndX - brush.lineStartX;
        double lz = brush.lineEndZ - brush.lineStartZ;
        searchR = std::sqrt(lx*lx + lz*lz) * 0.5 + brush.radius;
    }

    for (int i = 0; i < target * 3 && (int)result.size() < target; ++i) {
        double wx = brush.cx + prng.NextRange(-searchR, searchR);
        double wz = brush.cz + prng.NextRange(-searchR, searchR);
        if (!PointInBrush(wx, wz, brush)) continue;

        double wy = terrain.Height(wx, wz);
        if (wy < type.heightMin || wy > type.heightMax) continue;
        if (type.slopeMax >= 0.0 && terrain.Slope(wx, wz) > type.slopeMax) continue;

        FoliageInstance inst;
        inst.x = wx; inst.y = wy; inst.z = wz;
        inst.scaleX = inst.scaleY = inst.scaleZ = prng.NextRange(type.scaleMin, type.scaleMax);
        if (type.alignToNormal) terrain.Normal(wx, wz, &inst.nx, &inst.ny, &inst.nz);
        if (type.randomRotation) inst.yaw = prng.NextRange(0.0, 2.0 * M_PI);
        result.push_back(inst);
    }
    return result;
}

} // namespace foliage

// foliage_un06 — statistical analysis implementation.
#include "foliage_analyze.h"
#include "foliage_query.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace foliage {

ScatterAnalysis Analyze(const std::vector<FoliageInstance>& instances,
                         const ScatterArea&                   area,
                         double                               densityCellSize)
{
    ScatterAnalysis sa;
    sa.totalInstances = static_cast<std::uint32_t>(instances.size());
    if (instances.empty()) return sa;

    // Bounds
    sa.xMin = sa.xMax = instances[0].x;
    sa.zMin = sa.zMax = instances[0].z;
    sa.yMin = sa.yMax = instances[0].y;

    double sumX = 0, sumZ = 0, sumY = 0, sumScale = 0, sumYaw = 0;
    double sumSX = 0, sumSZ = 0, sumSY = 0, sumSS = 0;

    for (auto& inst : instances) {
        sumX += inst.x; sumZ += inst.z; sumY += inst.y;
        double s = (inst.scaleX + inst.scaleY + inst.scaleZ) / 3.0;
        sumScale += s;
        sumYaw += inst.yaw;

        if (inst.x < sa.xMin) sa.xMin = inst.x;
        if (inst.x > sa.xMax) sa.xMax = inst.x;
        if (inst.z < sa.zMin) sa.zMin = inst.z;
        if (inst.z > sa.zMax) sa.zMax = inst.z;
        if (inst.y < sa.yMin) sa.yMin = inst.y;
        if (inst.y > sa.yMax) sa.yMax = inst.y;
    }

    double n = static_cast<double>(instances.size());
    sa.avgX = sumX / n; sa.avgZ = sumZ / n; sa.avgY = sumY / n;
    sa.avgScale = sumScale / n;
    sa.avgYaw = sumYaw / n;

    // Standard deviations
    for (auto& inst : instances) {
        sumSX += (inst.x - sa.avgX) * (inst.x - sa.avgX);
        sumSZ += (inst.z - sa.avgZ) * (inst.z - sa.avgZ);
        sumSY += (inst.y - sa.avgY) * (inst.y - sa.avgY);
        double s = (inst.scaleX + inst.scaleY + inst.scaleZ) / 3.0;
        sumSS += (s - sa.avgScale) * (s - sa.avgScale);
    }
    sa.stdX = std::sqrt(sumSX / n);
    sa.stdZ = std::sqrt(sumSZ / n);
    sa.stdY = std::sqrt(sumSY / n);
    sa.stdScale = std::sqrt(sumSS / n);

    // Nearest neighbour
    if (instances.size() >= 2) {
        SpatialIndex idx(8.0);
        idx.Build(instances, sa.xMin, sa.zMin, sa.xMax, sa.zMax);
        double sumNN = 0;
        sa.nearestNeighbourMin = 1e9;
        sa.nearestNeighbourMax = 0;
        for (size_t i = 0; i < instances.size(); ++i) {
            double dist = 0;
            int nn = idx.Nearest(instances[i].x, instances[i].z, 1e9, &dist);
            if (nn >= 0 && nn != static_cast<int>(i) && dist > 0.01) {
                sumNN += dist;
                if (dist < sa.nearestNeighbourMin) sa.nearestNeighbourMin = dist;
                if (dist > sa.nearestNeighbourMax) sa.nearestNeighbourMax = dist;
            }
        }
        sa.nearestNeighbourAvg = sumNN / n;
    }

    // Clark-Evans
    sa.clarkEvansIndex = ClarkEvansIndex(instances, area);

    // Height histogram
    sa.heightHist = BuildHeightHistogram(instances, 20);

    // Density grid
    sa.densityGrid = BuildDensityGrid(instances, area, densityCellSize);

    return sa;
}

DensityGrid BuildDensityGrid(const std::vector<FoliageInstance>& instances,
                              const ScatterArea&                 area,
                              double                             cellSize)
{
    DensityGrid dg;
    dg.originX = area.originX; dg.originZ = area.originZ;
    dg.cellSize = cellSize;
    dg.cols = std::max(1, static_cast<int>(area.width / cellSize) + 1);
    dg.rows = std::max(1, static_cast<int>(area.depth / cellSize) + 1);
    dg.counts.resize(dg.cols * dg.rows, 0);
    dg.density.resize(dg.cols * dg.rows, 0.0);

    for (auto& inst : instances) {
        int cx = static_cast<int>((inst.x - area.originX) / cellSize);
        int cz = static_cast<int>((inst.z - area.originZ) / cellSize);
        if (cx >= 0 && cx < dg.cols && cz >= 0 && cz < dg.rows)
            dg.counts[cz * dg.cols + cx]++;
    }

    double cellArea = cellSize * cellSize;
    for (int i = 0; i < dg.cols * dg.rows; ++i)
        dg.density[i] = dg.counts[i] / cellArea;

    return dg;
}

HeightHistogram BuildHeightHistogram(const std::vector<FoliageInstance>& instances,
                                       int numBins)
{
    HeightHistogram hh;
    hh.numBins = numBins;
    if (instances.empty()) return hh;

    double yMin = instances[0].y, yMax = instances[0].y;
    for (auto& inst : instances) {
        if (inst.y < yMin) yMin = inst.y;
        if (inst.y > yMax) yMax = inst.y;
    }
    double range = (yMax - yMin);
    if (range < 0.01) range = 1.0;
    hh.binMin = yMin; hh.binMax = yMax;
    hh.binSize = range / numBins;
    hh.counts.resize(numBins, 0);
    hh.binCenters.resize(numBins);

    for (int b = 0; b < numBins; ++b) {
        hh.binCenters[b] = yMin + (b + 0.5) * hh.binSize;
    }

    for (auto& inst : instances) {
        int b = static_cast<int>((inst.y - yMin) / hh.binSize);
        if (b < 0) b = 0;
        if (b >= numBins) b = numBins - 1;
        hh.counts[b]++;
    }
    return hh;
}

double ClarkEvansIndex(const std::vector<FoliageInstance>& instances,
                       const ScatterArea& area)
{
    if (instances.size() < 2) return 0.0;
    double n = static_cast<double>(instances.size());
    double areaSize = area.width * area.depth;
    double density = n / areaSize;

    // Expected mean NN distance for CSR (complete spatial randomness)
    double expectedNN = 1.0 / (2.0 * std::sqrt(density));

    // Observed mean NN
    double xMin = instances[0].x, xMax = instances[0].x;
    double zMin = instances[0].z, zMax = instances[0].z;
    for (auto& inst : instances) {
        if (inst.x < xMin) xMin = inst.x; if (inst.x > xMax) xMax = inst.x;
        if (inst.z < zMin) zMin = inst.z; if (inst.z > zMax) zMax = inst.z;
    }

    SpatialIndex idx(8.0);
    idx.Build(instances, xMin, zMin, xMax, zMax);
    double sumNN = 0;
    int valid = 0;
    for (size_t i = 0; i < instances.size(); ++i) {
        double dist = 0;
        int nn = idx.Nearest(instances[i].x, instances[i].z, 1e9, &dist);
        if (nn >= 0 && nn != static_cast<int>(i) && dist > 0.001) {
            sumNN += dist;
            ++valid;
        }
    }

    if (valid == 0 || expectedNN < 0.0001) return 0.0;
    double observedNN = sumNN / valid;
    return observedNN / expectedNN;
}

} // namespace foliage

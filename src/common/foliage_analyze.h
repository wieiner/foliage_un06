// foliage_un06 — statistical analysis of scatter results.
#pragma once

#include "foliage_type.h"
#include <vector>
#include <cstdint>

namespace foliage {

struct HeightHistogram {
    double binMin = 0, binMax = 0, binSize = 1.0;
    int    numBins = 20;
    std::vector<int> counts;
    std::vector<double> binCenters;
};

struct DensityGrid {
    double originX = 0, originZ = 0;
    double cellSize = 10.0;
    int    cols = 0, rows = 0;
    std::vector<int> counts;       // instances per cell
    std::vector<double> density;   // instances per unit^2
};

struct ScatterAnalysis {
    // Basic stats
    std::uint32_t totalInstances = 0;
    double avgX = 0, avgZ = 0, avgY = 0;
    double stdX = 0, stdZ = 0, stdY = 0;
    double avgScale = 0, stdScale = 0;
    double avgYaw = 0;

    // Spatial
    double nearestNeighbourAvg = 0;
    double nearestNeighbourMin = 0;
    double nearestNeighbourMax = 0;
    double clarkEvansIndex = 0;    // >1 = regular, =1 = random, <1 = clustered

    // Bounds
    double xMin = 0, xMax = 0, zMin = 0, zMax = 0, yMin = 0, yMax = 0;

    // Sub-maps
    HeightHistogram heightHist;
    DensityGrid     densityGrid;
};

// ---------------------------------------------------------------------------
// Analyze — compute full statistical analysis
// ---------------------------------------------------------------------------
ScatterAnalysis Analyze(const std::vector<FoliageInstance>& instances,
                        const ScatterArea&                   area,
                        double                               densityCellSize = 10.0);

// Build 2D density grid
DensityGrid BuildDensityGrid(const std::vector<FoliageInstance>& instances,
                              const ScatterArea&                 area,
                              double                             cellSize = 10.0);

// Height histogram
HeightHistogram BuildHeightHistogram(const std::vector<FoliageInstance>& instances,
                                       int numBins = 20);

// Compute Clark-Evans index (spatial regularity measure)
double ClarkEvansIndex(const std::vector<FoliageInstance>& instances,
                       const ScatterArea& area);

} // namespace foliage

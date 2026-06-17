// foliage_un06 — spatial query subsystem.
// Grid-hash based nearest-neighbour, radius search, and point lookup.
#pragma once

#include "foliage_type.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace foliage {

// ---------------------------------------------------------------------------
// SpatialIndex — grid-hash accelerator for 2D (XZ) queries
// ---------------------------------------------------------------------------
class SpatialIndex {
public:
    SpatialIndex(double cellSize = 8.0) : cellSize_(cellSize), invCellSize_(1.0 / cellSize) {}

    // Build index from instance array.  Indices in `instances` become the keys.
    void Build(const std::vector<FoliageInstance>& instances, double xMin, double zMin, double xMax, double zMax);

    // Find nearest instance to (x, z).  Returns -1 if none within maxDist.
    int  Nearest(double x, double z, double maxDist = 1e9, double* outDist = nullptr) const;

    // Find all instances within `radius` of (x, z), sorted by distance.
    struct Hit { int index = -1; double dist = 0.0; };
    std::vector<Hit> RadiusSearch(double x, double z, double radius, int maxHits = 500) const;

    // Find instance at exact position (within tolerance)
    int  AtPosition(double x, double z, double tol = 0.5) const;

    // Count in cell
    int  CountInRadius(double x, double z, double radius) const;

    // Clear
    void Clear();

    // Access
    const std::vector<FoliageInstance>* instances = nullptr;

private:
    struct Cell { std::vector<int> indices; };
    int  CellIndex(double x, double z) const;

    double cellSize_;
    double invCellSize_;
    int    cols_ = 0, rows_ = 0;
    double originX_ = 0, originZ_ = 0;
    std::vector<Cell> cells_;
};

// ---------------------------------------------------------------------------
// Free-standing query functions (no index needed for small sets)
// ---------------------------------------------------------------------------
struct QueryResult {
    int    index = -1;
    double distance = 0.0;
    const FoliageInstance* instance = nullptr;
};

// Nearest among `instances` to (x, z)
QueryResult QueryNearest(const std::vector<FoliageInstance>& instances,
                         double x, double z, double maxDist = 1e9);

// All instances within `radius` of (x, z)
std::vector<QueryResult> QueryRadius(const std::vector<FoliageInstance>& instances,
                                     double x, double z, double radius, int maxHits = 500);

} // namespace foliage

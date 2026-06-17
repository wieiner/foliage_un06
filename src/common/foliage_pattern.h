// foliage_un06 — placement patterns: radial, along-path, clumping, brush shapes.
// Replicates advanced UE brush/distribution modes.
#pragma once

#include "foliage_type.h"
#include "foliage_prng.h"
#include "foliage_terrain.h"
#include <cmath>
#include <vector>
#include <utility>

namespace foliage {

// ---------------------------------------------------------------------------
// RadialPattern — concentric rings / disc placement
// ---------------------------------------------------------------------------
struct RadialParams {
    double centerX  = 0.0;
    double centerZ = 0.0;
    double innerRadius = 0.0;     // donut inner radius
    double outerRadius = 50.0;    // donut outer radius
    int    ringCount   = 5;       // number of concentric rings
    double densityMul  = 1.0;
    bool   fillDisc    = true;    // false = only rings, true = full disc
};

// Generate instances in radial pattern.
std::vector<FoliageInstance> ScatterRadial(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const RadialParams&    params);

// ---------------------------------------------------------------------------
// PathParams — scatter along a polyline/spline
// ---------------------------------------------------------------------------
struct PathParams {
    std::vector<std::pair<double,double>> points;
    double width     = 10.0;    // half-width from path center
    double density   = 0.1;     // instances per unit length along path
    bool   closed    = false;
    bool   parallel  = true;    // instances parallel to path tangent
    unsigned seed    = 1337;
};

std::vector<FoliageInstance> ScatterAlongPath(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const PathParams&      params);

// ---------------------------------------------------------------------------
// ClumpParams — Thomas cluster process (natural clumping)
// ---------------------------------------------------------------------------
struct ClumpParams {
    int    parentCount  = 10;      // number of cluster centers
    double clusterRadius = 15.0;   // radius around each parent
    int    childrenPerParent = 8;  // avg children per parent
    double childSpread  = 5.0;     // stddev of child position within cluster
    unsigned seed       = 1337;
};

// Generate instances in natural clumps (Matern/Thomas cluster process).
// First places parent seeds via Poisson disc, then scatters children
// around each parent with Gaussian falloff.
std::vector<FoliageInstance> ScatterClumped(
    const TerrainSampler&  terrain,
    const FoliageType&     type,
    const ScatterArea&     area,
    const ClumpParams&     params);

// ---------------------------------------------------------------------------
// BrushShape — different brush footprints
// ---------------------------------------------------------------------------
enum class BrushShapeType { Circle, Square, Donut, Line, Gaussian };

struct BrushShapeParams {
    BrushShapeType shapeType = BrushShapeType::Circle;
    double cx = 0, cz = 0;          // center
    double radius = 20.0;            // radius (Circle, Donut)
    double innerRadius = 10.0;       // inner radius (Donut)
    double halfWidth = 20.0;         // half-width (Square)
    double lineStartX = 0, lineStartZ = 0;  // line endpoints
    double lineEndX = 50, lineEndZ = 50;
    double gaussianSigma = 8.0;      // sigma for Gaussian falloff
};

// Check if a point is inside the brush shape.
bool PointInBrush(double x, double z, const BrushShapeParams& brush);

// Scatter inside a brush shape (for paint operations).
std::vector<FoliageInstance> ScatterInBrushShape(
    const TerrainSampler&    terrain,
    const FoliageType&       type,
    const BrushShapeParams&  brush,
    const BrushParams&       density,
    int                      maxInstances = 5000);

} // namespace foliage

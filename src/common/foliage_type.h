// foliage_un06 — FoliageType + FoliageInstance core data model.
// Shared by all protocol adapters; no engine or renderer dependency.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace foliage {

// Forward
struct TerrainSampler;

// ---------------------------------------------------------------------------
// FoliageType — what to scatter
// ---------------------------------------------------------------------------
struct FoliageType {
    std::string name;
    std::string meshId;          // reference to mesh asset (instmesh_un05 key)

    // Placement
    double density     = 0.02;   // avg instances per unit^2
    double radius      = 0.0;    // min distance between instances (0 = auto from density)

    // Scale
    double scaleMin    = 0.8;
    double scaleMax    = 1.4;
    bool   uniformScale = true;   // X==Y==Z when true; otherwise X,Y,Z sampled independently

    // Rotation
    bool randomRotation  = true;   // random yaw when true
    bool alignToNormal   = true;   // instance up = terrain normal
    double rotationMin   = 0.0;    // extra yaw offset min (degrees)
    double rotationMax   = 360.0;  // extra yaw offset max (degrees)

    // Filters
    double slopeMax      = 35.0;   // degrees; -1 = disabled
    double heightMin     = -1e9;   // world height
    double heightMax     = 1e9;

    // Determinism
    std::uint32_t seed   = 1337;

    // Misc
    double lodCullDistance = 5000.0; // end cull distance
};

// ---------------------------------------------------------------------------
// FoliageInstance — one placed instance (world-space transform)
// ---------------------------------------------------------------------------
struct FoliageInstance {
    double x = 0.0, y = 0.0, z = 0.0;    // position
    double nx = 0.0, ny = 1.0, nz = 0.0;  // terrain normal at placement
    double scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
    double yaw = 0.0;  // rotation around up-axis (radians)

    // Convenience
    double Height() const { return y; }
};

// ---------------------------------------------------------------------------
// ScatterArea — input region descriptor
// ---------------------------------------------------------------------------
struct ScatterArea {
    double originX = 0.0;
    double originZ = 0.0;
    double width   = 100.0;   // X extent
    double depth   = 100.0;   // Z extent
};

// ---------------------------------------------------------------------------
// ScatterResult — output of a scatter operation
// ---------------------------------------------------------------------------
struct ScatterResult {
    std::string foliageTypeName;
    ScatterArea area;
    std::uint32_t seed     = 0;
    std::uint32_t candidates = 0;   // total attempts
    std::uint32_t kept       = 0;    // instances that passed filters
    double avgScale          = 1.0;
    std::vector<FoliageInstance> instances;
};

// ---------------------------------------------------------------------------
// PaintBrush / EraseBrush parameters
// ---------------------------------------------------------------------------
struct BrushParams {
    double centerX  = 0.0;
    double centerZ  = 0.0;
    double radius   = 20.0;
    double densityMul = 1.0;   // multiplier on FoliageType.density
};

// ---------------------------------------------------------------------------
// MeshTemplate — simple mesh data for instance rendering
// ---------------------------------------------------------------------------
struct MeshTemplate {
    std::string name;
    // Vertices: position (local space) for a unit-scale reference mesh
    std::vector<double> vertices;  // flat: x0,y0,z0, x1,y1,z1, ...
    std::vector<std::uint32_t> indices; // triangle indices
    double radiusXY = 1.0;  // bounding radius in XZ plane (for overlap tests)
    double height   = 2.0;  // bounding height Y
};

// Default mesh templates
inline MeshTemplate MakeCubeMesh(double halfSize = 0.5) {
    MeshTemplate m;
    m.name = "cube";
    m.radiusXY = halfSize * 1.42;
    m.height = halfSize * 2.0;
    // 8 vertices, 36 indices (12 triangles)
    double hs = halfSize;
    m.vertices = {
        -hs,-hs,-hs,  hs,-hs,-hs,  hs, hs,-hs, -hs, hs,-hs,  // -Z face
        -hs,-hs, hs,  hs,-hs, hs,  hs, hs, hs, -hs, hs, hs   // +Z face
    };
    m.indices = {
        0,1,2, 0,2,3,   // -Z
        4,6,5, 4,7,6,   // +Z
        0,4,5, 0,5,1,   // -Y
        3,2,6, 3,6,7,   // +Y
        0,3,7, 0,7,4,   // -X
        1,5,6, 1,6,2    // +X
    };
    return m;
}

inline MeshTemplate MakeCrossBillboardMesh(double halfSize = 0.5, double height = 1.5) {
    MeshTemplate m;
    m.name = "cross_billboard";
    m.radiusXY = halfSize * 1.42;
    m.height = height;
    double hs = halfSize;
    double h2 = height * 0.5;
    // Two quads at 45° to each other
    m.vertices = {
        -hs, -h2, 0.0f,  hs, -h2, 0.0f,  hs, h2, 0.0f, -hs, h2, 0.0f,      // quad 1
        0.0f, -h2, -hs,  0.0f, -h2, hs,  0.0f, h2, hs,  0.0f, h2, -hs       // quad 2
    };
    m.indices = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7
    };
    return m;
}

} // namespace foliage

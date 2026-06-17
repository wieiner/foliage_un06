// foliage_un06 — instance → mesh cluster output.
// Groups instances into spatial clusters and produces per-cluster vert/index
// arrays suitable for instanced mesh rendering or JSON export.
#pragma once

#include "foliage_type.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace foliage {

// ---------------------------------------------------------------------------
// ClusterMesh — one cluster worth of geometry
// ---------------------------------------------------------------------------
struct ClusterMesh {
    int    clusterX  = 0;
    int    clusterZ  = 0;
    int    instanceCount = 0;
    int    vertexCount   = 0;
    int    indexCount    = 0;
    double boundsMin[3]  = {0,0,0};
    double boundsMax[3]  = {0,0,0};
    std::vector<double>          vertices;  // flat: x0,y0,z0, nx0,ny0,nz0 (interleaved)
    std::vector<std::uint32_t>   indices;
};

// ---------------------------------------------------------------------------
// ClusterBuilder — bakes instanced mesh data from scatter output
// ---------------------------------------------------------------------------
class ClusterBuilder {
public:
    // Group instances by spatial cell and build per-cluster geometry.
    // Each instance transforms `mesh` vertices into world space.
    static std::vector<ClusterMesh> BuildClusters(
        const std::vector<FoliageInstance>& instances,
        const MeshTemplate&                mesh,
        double                             clusterSize = 16.0);

    // Generate a single combined mesh (all instances merged).
    static ClusterMesh BuildCombined(
        const std::vector<FoliageInstance>& instances,
        const MeshTemplate&                mesh);

    // Number of clusters that would be produced.
    static int ClusterCount(
        const std::vector<FoliageInstance>& instances,
        double                             clusterSize = 16.0);
};

} // namespace foliage

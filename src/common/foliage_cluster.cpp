// foliage_un06 — cluster mesh builder implementation.
#include "foliage_cluster.h"

#include <algorithm>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace foliage {

namespace {

// Build 3x3 rotation matrix from yaw angle + optional normal alignment.
// Simplified: rotate around up-axis by yaw, then tilt to match normal.
void MakeInstanceMatrix(const FoliageInstance& inst,
                        double m[9]) // 3x3 row-major
{
    double cy = std::cos(inst.yaw);
    double sy = std::sin(inst.yaw);

    // Start with yaw rotation around Y
    // The up vector is (inst.nx, inst.ny, inst.nz)
    // We build a basis with:
    //   Y' = normal (nx, ny, nz)
    //   X' = perpendicular to Y' in the world XZ plane, rotated by yaw
    //   Z' = X' × Y'

    double nyx = inst.nx, nyy = inst.ny, nyz = inst.nz;

    // World up
    double wx = 0.0, wy = 1.0, wz = 0.0;

    // If normal is nearly world-up, use simpler math
    if (nyy > 0.999) {
        // Yaw rotation only
        m[0] = cy;  m[1] = 0.0; m[2] = sy;
        m[3] = 0.0; m[4] = 1.0; m[5] = 0.0;
        m[6] = -sy; m[7] = 0.0; m[8] = cy;
        return;
    }
    if (nyy < -0.999) {
        m[0] = cy;  m[1] = 0.0; m[2] = -sy;
        m[3] = 0.0; m[4] = -1.0; m[5] = 0.0;
        m[6] = sy;  m[7] = 0.0; m[8] = cy;
        return;
    }

    // X' axis: cross(up, normal) — perpendicular to normal in XZ-ish plane
    double xx = wy * nyz - wz * nyy;
    double xy = wz * nyx - wx * nyz;
    double xz = wx * nyy - wy * nyx;
    double xlen = std::sqrt(xx*xx + xy*xy + xz*xz);
    xx /= xlen; xy /= xlen; xz /= xlen;

    // Apply yaw: rotate X' by yaw around normal
    // X'' = X' * cos(yaw) + (normal × X') * sin(yaw)
    double cx = nyy * xz - nyz * xy; // normal cross X'
    double cy2 = nyz * xx - nyx * xz;
    double cz = nyx * xy - nyy * xx;
    double rx0 = xx * cy + cx * sy;
    double ry0 = xy * cy + cy2 * sy;
    double rz0 = xz * cy + cz * sy;

    // Z' axis: X'' × normal
    double zx = ry0 * nyz - rz0 * nyy;
    double zy = rz0 * nyx - rx0 * nyz;
    double zz = rx0 * nyy - ry0 * nyx;

    m[0] = rx0; m[1] = nyx; m[2] = zx;
    m[3] = ry0; m[4] = nyy; m[5] = zy;
    m[6] = rz0; m[7] = nyz; m[8] = zz;
}

struct IntPair {
    int x, z;
    bool operator<(const IntPair& o) const {
        return x < o.x || (x == o.x && z < o.z);
    }
};

} // namespace

std::vector<ClusterMesh> ClusterBuilder::BuildClusters(
    const std::vector<FoliageInstance>& instances,
    const MeshTemplate&                mesh,
    double                             clusterSize)
{
    std::map<IntPair, std::vector<const FoliageInstance*>> groups;

    for (const auto& inst : instances) {
        int cx = static_cast<int>(std::floor(inst.x / clusterSize));
        int cz = static_cast<int>(std::floor(inst.z / clusterSize));
        groups[{cx, cz}].push_back(&inst);
    }

    int vertsPerInstance = static_cast<int>(mesh.vertices.size() / 3);
    int idxsPerInstance  = static_cast<int>(mesh.indices.size());

    std::vector<ClusterMesh> clusters;
    clusters.reserve(groups.size());

    for (const auto& [key, group] : groups) {
        ClusterMesh cm;
        cm.clusterX = key.x;
        cm.clusterZ = key.z;
        cm.instanceCount = static_cast<int>(group.size());
        cm.vertexCount = cm.instanceCount * vertsPerInstance;
        cm.indexCount  = cm.instanceCount * idxsPerInstance;
        cm.vertices.reserve(cm.vertexCount * 6); // pos3 + normal3
        cm.indices.reserve(cm.indexCount);

        // Init bounds
        cm.boundsMin[0] = cm.boundsMin[1] = cm.boundsMin[2] = 1e30;
        cm.boundsMax[0] = cm.boundsMax[1] = cm.boundsMax[2] = -1e30;

        for (const auto* inst : group) {
            double m[9];
            MakeInstanceMatrix(*inst, m);
            double sx = inst->scaleX, sy = inst->scaleY, sz = inst->scaleZ;

            int baseV = static_cast<int>(cm.vertices.size() / 6);

            for (int v = 0; v < vertsPerInstance; ++v) {
                double lx = mesh.vertices[v*3 + 0];
                double ly = mesh.vertices[v*3 + 1];
                double lz = mesh.vertices[v*3 + 2];

                // Scale
                lx *= sx; ly *= sy; lz *= sz;

                // Rotate
                double wx = m[0]*lx + m[1]*ly + m[2]*lz;
                double wy = m[3]*lx + m[4]*ly + m[5]*lz;
                double wz = m[6]*lx + m[7]*ly + m[8]*lz;

                // Translate
                wx += inst->x; wy += inst->y; wz += inst->z;

                cm.vertices.push_back(wx);
                cm.vertices.push_back(wy);
                cm.vertices.push_back(wz);
                // Store normal too
                cm.vertices.push_back(inst->nx);
                cm.vertices.push_back(inst->ny);
                cm.vertices.push_back(inst->nz);

                // Bounds
                if (wx < cm.boundsMin[0]) cm.boundsMin[0] = wx;
                if (wy < cm.boundsMin[1]) cm.boundsMin[1] = wy;
                if (wz < cm.boundsMin[2]) cm.boundsMin[2] = wz;
                if (wx > cm.boundsMax[0]) cm.boundsMax[0] = wx;
                if (wy > cm.boundsMax[1]) cm.boundsMax[1] = wy;
                if (wz > cm.boundsMax[2]) cm.boundsMax[2] = wz;
            }

            for (std::uint32_t idx : mesh.indices) {
                cm.indices.push_back(baseV + idx);
            }
        }
        clusters.push_back(std::move(cm));
    }
    return clusters;
}

ClusterMesh ClusterBuilder::BuildCombined(
    const std::vector<FoliageInstance>& instances,
    const MeshTemplate&                mesh)
{
    // Just use a huge cluster size so everything goes in one bucket
    double maxExtent = 0.0;
    for (const auto& inst : instances) {
        maxExtent = std::max(maxExtent, std::abs(inst.x));
        maxExtent = std::max(maxExtent, std::abs(inst.z));
    }
    auto clusters = BuildClusters(instances, mesh, maxExtent * 3.0);
    if (clusters.empty()) {
        ClusterMesh empty;
        empty.instanceCount = 0;
        return empty;
    }
    return clusters[0];
}

int ClusterBuilder::ClusterCount(
    const std::vector<FoliageInstance>& instances,
    double                             clusterSize)
{
    std::map<IntPair, int> groups;
    for (const auto& inst : instances) {
        int cx = static_cast<int>(std::floor(inst.x / clusterSize));
        int cz = static_cast<int>(std::floor(inst.z / clusterSize));
        groups[{cx, cz}]++;
    }
    return static_cast<int>(groups.size());
}

} // namespace foliage

// foliage_un06 — spatial query implementation.
#include "foliage_query.h"

namespace foliage {

// ---------------------------------------------------------------------------
// SpatialIndex
// ---------------------------------------------------------------------------
void SpatialIndex::Build(const std::vector<FoliageInstance>& insts,
                         double xMin, double zMin, double xMax, double zMax)
{
    instances = &insts;
    originX_ = xMin; originZ_ = zMin;
    double w = (xMax - xMin) + cellSize_;
    double d = (zMax - zMin) + cellSize_;
    cols_ = std::max(1, static_cast<int>(w * invCellSize_));
    rows_ = std::max(1, static_cast<int>(d * invCellSize_));
    cells_.clear();
    cells_.resize(cols_ * rows_);

    for (int i = 0; i < static_cast<int>(insts.size()); ++i) {
        int ci = CellIndex(insts[i].x, insts[i].z);
        if (ci >= 0 && ci < static_cast<int>(cells_.size()))
            cells_[ci].indices.push_back(i);
    }
}

void SpatialIndex::Clear() {
    cells_.clear();
    cols_ = rows_ = 0;
    instances = nullptr;
}

int SpatialIndex::CellIndex(double x, double z) const {
    int cx = static_cast<int>((x - originX_) * invCellSize_);
    int cz = static_cast<int>((z - originZ_) * invCellSize_);
    if (cx < 0) cx = 0; if (cx >= cols_) cx = cols_ - 1;
    if (cz < 0) cz = 0; if (cz >= rows_) cz = rows_ - 1;
    return cz * cols_ + cx;
}

int SpatialIndex::Nearest(double x, double z, double maxDist, double* outDist) const {
    if (!instances) return -1;
    int best = -1;
    double bestD2 = maxDist * maxDist;
    int ccx = static_cast<int>((x - originX_) * invCellSize_);
    int ccz = static_cast<int>((z - originZ_) * invCellSize_);
    // Spiral search outward
    for (int ring = 0; ring <= 3; ++ring) {
        bool anyCell = false;
        for (int dcx = -ring; dcx <= ring; ++dcx) {
            for (int dcz = -ring; dcz <= ring; ++dcz) {
                if (std::abs(dcx) < ring && std::abs(dcz) < ring) continue;
                int ci = (ccz + dcz) * cols_ + (ccx + dcx);
                if (ci < 0 || ci >= static_cast<int>(cells_.size())) continue;
                anyCell = true;
                for (int idx : cells_[ci].indices) {
                    const auto& inst = (*instances)[idx];
                    double dx = inst.x - x, dz = inst.z - z;
                    double d2 = dx*dx + dz*dz;
                    if (d2 < bestD2) { bestD2 = d2; best = idx; }
                }
            }
        }
        if (best >= 0 || (!anyCell && ring > 0)) break;
    }
    if (outDist && best >= 0) *outDist = std::sqrt(bestD2);
    return best;
}

std::vector<SpatialIndex::Hit> SpatialIndex::RadiusSearch(double x, double z, double radius, int maxHits) const {
    std::vector<Hit> results;
    if (!instances) return results;
    double r2 = radius * radius;
    int ccx = static_cast<int>((x - originX_) * invCellSize_);
    int ccz = static_cast<int>((z - originZ_) * invCellSize_);
    int cellRad = static_cast<int>(radius * invCellSize_) + 1;
    for (int dcx = -cellRad; dcx <= cellRad; ++dcx) {
        for (int dcz = -cellRad; dcz <= cellRad; ++dcz) {
            int ci = (ccz + dcz) * cols_ + (ccx + dcx);
            if (ci < 0 || ci >= static_cast<int>(cells_.size())) continue;
            for (int idx : cells_[ci].indices) {
                const auto& inst = (*instances)[idx];
                double dx = inst.x - x, dz = inst.z - z;
                double d2 = dx*dx + dz*dz;
                if (d2 <= r2) {
                    results.push_back({idx, std::sqrt(d2)});
                }
            }
        }
    }
    std::sort(results.begin(), results.end(), [](const Hit& a, const Hit& b) { return a.dist < b.dist; });
    if (static_cast<int>(results.size()) > maxHits) results.resize(maxHits);
    return results;
}

int SpatialIndex::AtPosition(double x, double z, double tol) const {
    if (!instances) return -1;
    double t2 = tol * tol;
    int ccx = static_cast<int>((x - originX_) * invCellSize_);
    int ccz = static_cast<int>((z - originZ_) * invCellSize_);
    int ci = ccz * cols_ + ccx;
    if (ci >= 0 && ci < static_cast<int>(cells_.size())) {
        for (int idx : cells_[ci].indices) {
            const auto& inst = (*instances)[idx];
            double dx = inst.x - x, dz = inst.z - z;
            if (dx*dx + dz*dz <= t2) return idx;
        }
    }
    return -1;
}

int SpatialIndex::CountInRadius(double x, double z, double radius) const {
    if (!instances) return 0;
    int count = 0;
    double r2 = radius * radius;
    int ccx = static_cast<int>((x - originX_) * invCellSize_);
    int ccz = static_cast<int>((z - originZ_) * invCellSize_);
    int cellRad = static_cast<int>(radius * invCellSize_) + 1;
    for (int dcx = -cellRad; dcx <= cellRad; ++dcx) {
        for (int dcz = -cellRad; dcz <= cellRad; ++dcz) {
            int ci = (ccz + dcz) * cols_ + (ccx + dcx);
            if (ci < 0 || ci >= static_cast<int>(cells_.size())) continue;
            for (int idx : cells_[ci].indices) {
                const auto& inst = (*instances)[idx];
                double dx = inst.x - x, dz = inst.z - z;
                if (dx*dx + dz*dz <= r2) ++count;
            }
        }
    }
    return count;
}

// ----- free functions -----
QueryResult QueryNearest(const std::vector<FoliageInstance>& instances,
                         double x, double z, double maxDist) {
    QueryResult best;
    best.distance = maxDist;
    double maxD2 = maxDist * maxDist;
    for (int i = 0; i < static_cast<int>(instances.size()); ++i) {
        double dx = instances[i].x - x, dz = instances[i].z - z;
        double d2 = dx*dx + dz*dz;
        if (d2 < maxD2) { maxD2 = d2; best.index = i; best.distance = std::sqrt(d2); best.instance = &instances[i]; }
    }
    return best;
}

std::vector<QueryResult> QueryRadius(const std::vector<FoliageInstance>& instances,
                                     double x, double z, double radius, int maxHits) {
    std::vector<QueryResult> results;
    double r2 = radius * radius;
    for (int i = 0; i < static_cast<int>(instances.size()); ++i) {
        double dx = instances[i].x - x, dz = instances[i].z - z;
        double d2 = dx*dx + dz*dz;
        if (d2 <= r2) {
            results.push_back({i, std::sqrt(d2), &instances[i]});
        }
    }
    std::sort(results.begin(), results.end(),
              [](const QueryResult& a, const QueryResult& b) { return a.distance < b.distance; });
    if (static_cast<int>(results.size()) > maxHits) results.resize(maxHits);
    return results;
}

} // namespace foliage

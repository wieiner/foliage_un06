// foliage_un06 — mask system implementation.
#include "foliage_mask.h"
#include <cmath>
#include <algorithm>

namespace foliage {

// ---------------------------------------------------------------------------
// LayerMap
// ---------------------------------------------------------------------------
bool LayerMap::Passes(double x, double z) const {
    // No layers defined → always pass
    if (inclusionLayers.empty() && exclusionLayers.empty()) return true;

    // Check exclusions first (fail-fast)
    for (const auto& name : exclusionLayers) {
        double w = 1.0;
        if (weightFn) w = weightFn(x, z, name, userData);
        if (w >= minExclusionWeight) return false;
    }

    // Check inclusions
    if (inclusionLayers.empty()) return true;
    for (const auto& name : inclusionLayers) {
        double w = 1.0;
        if (weightFn) w = weightFn(x, z, name, userData);
        if (w >= minInclusionWeight) return true;
    }
    return false; // no inclusion layer matched
}

// ---------------------------------------------------------------------------
// ExclusionZone
// ---------------------------------------------------------------------------
bool ExclusionZone::Contains(double x, double z) const {
    if (shape == Circle) {
        double dx = x - circleX, dz = z - circleZ;
        return (dx * dx + dz * dz) <= (circleRadius * circleRadius);
    } else {
        return (x >= rectX && x <= rectX + rectW && z >= rectZ && z <= rectZ + rectD);
    }
}

double ExclusionZone::Softness(double x, double z) const {
    if (!soft) return Contains(x, z) ? 0.0 : 1.0;

    if (shape == Circle) {
        double dx = x - circleX, dz = z - circleZ;
        double dist = std::sqrt(dx * dx + dz * dz);
        double t = dist / circleRadius;
        if (t < 0.9) return 0.0; // fully inside
        if (t > 1.1) return 1.0; // fully outside
        // Smoothstep falloff in [0.9, 1.1]
        double s = (t - 0.9) / 0.2;
        return s * s * (3.0 - 2.0 * s);
    } else {
        double dx = std::max(0.0, std::max(rectX - x, x - (rectX + rectW)));
        double dz = std::max(0.0, std::max(rectZ - z, z - (rectZ + rectD)));
        if (dx == 0.0 && dz == 0.0) return 0.0;
        double dist = std::sqrt(dx * dx + dz * dz);
        if (dist > 5.0) return 1.0;
        return dist / 5.0;
    }
}

// ---------------------------------------------------------------------------
// ExclusionZoneSet
// ---------------------------------------------------------------------------
void ExclusionZoneSet::AddZone(const ExclusionZone& zone) {
    // Replace existing by name
    for (auto& z : zones_) {
        if (z.name == zone.name) { z = zone; return; }
    }
    zones_.push_back(zone);
}

void ExclusionZoneSet::RemoveZone(const std::string& name) {
    zones_.erase(std::remove_if(zones_.begin(), zones_.end(),
        [&](const ExclusionZone& z) { return z.name == name; }), zones_.end());
}

void ExclusionZoneSet::Clear() { zones_.clear(); }

bool ExclusionZoneSet::IsAllowed(double x, double z) const {
    // Inclusion zones (invert=true) take priority: if any exist, point MUST be in at least one
    bool hasInclusions = false;
    for (const auto& zone : zones_) {
        if (zone.invert) { hasInclusions = true; break; }
    }

    if (hasInclusions) {
        bool inAny = false;
        for (const auto& zone : zones_) {
            if (!zone.invert) continue;
            if (zone.soft) {
                if (zone.Softness(x, z) > 0.0) { inAny = true; break; }
            } else {
                if (zone.Contains(x, z)) { inAny = true; break; }
            }
        }
        if (!inAny) return false;
    }

    // Check exclusion zones
    for (const auto& zone : zones_) {
        if (zone.invert) continue;
        if (zone.soft) {
            if (zone.Softness(x, z) < 1.0) return false;
        } else {
            if (zone.Contains(x, z)) return false;
        }
    }
    return true;
}

std::vector<FoliageInstance> ExclusionZoneSet::Filter(
    const std::vector<FoliageInstance>& instances) const
{
    if (zones_.empty()) return instances;
    std::vector<FoliageInstance> survivors;
    for (const auto& inst : instances) {
        if (IsAllowed(inst.x, inst.z)) survivors.push_back(inst);
    }
    return survivors;
}

// ---------------------------------------------------------------------------
// SplinePath
// ---------------------------------------------------------------------------
namespace {
double SegmentDist(double px, double pz,
                   double ax, double az, double bx, double bz) {
    double abx = bx - ax, abz = bz - az;
    double len2 = abx * abx + abz * abz;
    if (len2 < 0.0001) {
        double dx = px - ax, dz = pz - az;
        return std::sqrt(dx * dx + dz * dz);
    }
    double t = ((px - ax) * abx + (pz - az) * abz) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double cx = ax + t * abx, cz = az + t * abz;
    double dx = px - cx, dz = pz - cz;
    return std::sqrt(dx * dx + dz * dz);
}
} // namespace

double SplinePath::Distance(double x, double z) const {
    if (points.size() < 2) return 1e9;
    double best = 1e9;
    size_t n = closed ? points.size() : points.size() - 1;
    for (size_t i = 0; i < n; ++i) {
        auto& a = points[i];
        auto& b = points[(i + 1) % points.size()];
        double d = SegmentDist(x, z, a.first, a.second, b.first, b.second);
        if (d < best) best = d;
    }
    return best;
}

} // namespace foliage

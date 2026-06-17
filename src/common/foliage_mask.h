// foliage_un06 — layer masks & exclusion zones.
// Replicates UE's LandscapeLayer / ExclusionLandscapeLayer + blocking volumes.
#pragma once

#include "foliage_type.h"
#include <string>
#include <vector>
#include <map>

namespace foliage {

// ---------------------------------------------------------------------------
// LayerMask — inclusion/exclusion by named layer
// ---------------------------------------------------------------------------
struct LayerDef {
    std::string name;
    double      weight = 1.0;      // layer weight at this point
    bool        active = true;
};

// LayerMap: maps world (x,z) → set of layer weights.
// Production implementation queries landscape/terrain; test impl is dictionary.
struct LayerMap {
    // Inclusion layers: if non-empty, instance must match at least one.
    std::vector<std::string> inclusionLayers;
    // Exclusion layers: instance must NOT match any.
    std::vector<std::string> exclusionLayers;
    double minInclusionWeight  = 0.5;
    double minExclusionWeight  = 0.3;

    // Query callback
    using WeightFn = double (*)(double x, double z, const std::string& layerName, void* user);
    WeightFn weightFn = nullptr;
    void*    userData = nullptr;

    // Check if a position passes layer filter
    bool Passes(double x, double z) const;
};

// ---------------------------------------------------------------------------
// ExclusionZone — blocking volume (rectangular or circular)
// ---------------------------------------------------------------------------
struct ExclusionZone {
    std::string name;
    enum Shape { Rectangle, Circle } shape = Circle;

    // Rectangle
    double rectX = 0, rectZ = 0, rectW = 50, rectD = 50;

    // Circle
    double circleX = 0, circleZ = 0, circleRadius = 30;

    bool invert = false;   // true = this is an INCLUSION zone (only place inside)
    bool soft  = true;     // soft edge falloff (smoothstep at boundary)

    bool Contains(double x, double z) const;
    double Softness(double x, double z) const; // 1.0 = fully inside, 0.0 = fully outside
};

// ---------------------------------------------------------------------------
// ExclusionZoneSet — collection of zones + batch filtering
// ---------------------------------------------------------------------------
class ExclusionZoneSet {
public:
    void AddZone(const ExclusionZone& zone);
    void RemoveZone(const std::string& name);
    void Clear();
    const std::vector<ExclusionZone>& Zones() const { return zones_; }

    // Check if a position is blocked by any zone.
    // Returns true if placement is allowed.
    bool IsAllowed(double x, double z) const;

    // Filter instances: keep those not in any exclusion zone
    std::vector<FoliageInstance> Filter(
        const std::vector<FoliageInstance>& instances) const;

private:
    std::vector<ExclusionZone> zones_;
};

// ---------------------------------------------------------------------------
// SplinePath — linear path for exclusion near roads/paths
// ---------------------------------------------------------------------------
struct SplinePath {
    std::string name;
    std::vector<std::pair<double,double>> points;
    double exclusionWidth = 5.0;    // distance from path to exclude
    double falloffWidth   = 2.0;    // soft edge width
    bool   closed         = false;

    // Signed distance from (x,z) to nearest segment (positive = outside)
    double Distance(double x, double z) const;
};

} // namespace foliage

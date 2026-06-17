// foliage_un06 — density falloff implementation.
#include "foliage_falloff.h"
#include <algorithm>

namespace foliage {

namespace {

double EdgeDistance(double x, double z, const ScatterArea& area) {
    double dx = std::min(x - area.originX, area.originX + area.width - x);
    double dz = std::min(z - area.originZ, area.originZ + area.depth - z);
    if (dx < 0) dx = 0; if (dz < 0) dz = 0;
    return std::min(dx, dz);
}

double ApplyFalloffType(double t, const std::string& type) {
    // t = normalized edge distance [0,1] (1 = at edge, 0 = deep inside)
    if (type == "linear") {
        return t;
    } else if (type == "smoothstep") {
        return t * t * (3.0 - 2.0 * t);
    } else if (type == "sqrt") {
        return std::sqrt(t);
    } else if (type == "inverse") {
        return t < 0.01 ? 0.0 : 1.0 - t;
    } else {
        return t; // default linear
    }
}

} // namespace

std::vector<FoliageInstance> ApplyDensityFalloff(
    const std::vector<FoliageInstance>& instances,
    const ScatterArea&                  area,
    const FalloffParams&                params)
{
    std::vector<FoliageInstance> survivors;
    if (instances.empty()) return survivors;

    FoliagePrng prng(params.seed);
    double fw = params.falloffWidth;
    if (fw <= 0.0) fw = 1.0;

    // Precompute which area fraction is preserved
    double preserveDist = params.preserveCenter * std::min(area.width, area.depth) * 0.5;

    for (const auto& inst : instances) {
        double ed = EdgeDistance(inst.x, inst.z, area);

        // Fully preserved center zone
        if (ed >= preserveDist + fw) {
            survivors.push_back(inst);
            continue;
        }

        // Compute normalized distance inside falloff zone
        double t = 1.0;
        if (ed > preserveDist) {
            t = 1.0 - (ed - preserveDist) / fw;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
        } else {
            t = 0.0; // in preserve zone but close to it — keep
            survivors.push_back(inst);
            continue;
        }

        double killProb = ApplyFalloffType(t, params.falloffType);
        double roll = prng.NextDouble();
        if (roll >= killProb) {
            survivors.push_back(inst);
        }
    }
    return survivors;
}

std::vector<FoliageInstance> ApplyFalloffCurve(
    const std::vector<FoliageInstance>& instances,
    const ScatterArea&                  area,
    const std::vector<std::pair<double,double>>& curve,
    std::uint32_t seed)
{
    std::vector<FoliageInstance> survivors;
    if (instances.empty() || curve.empty()) {
        survivors = instances;
        return survivors;
    }

    FoliagePrng prng(seed);
    double maxDim = std::max(area.width, area.depth) * 0.5;

    for (const auto& inst : instances) {
        double ed = EdgeDistance(inst.x, inst.z, area);
        double normDist = ed / maxDim;
        if (normDist > 1.0) normDist = 1.0;
        if (normDist < 0.0) normDist = 0.0;

        // Interpolate survival probability from curve
        double survivalProb = 1.0;
        if (normDist <= curve[0].first) {
            survivalProb = curve[0].second;
        } else if (normDist >= curve.back().first) {
            survivalProb = curve.back().second;
        } else {
            for (size_t i = 0; i + 1 < curve.size(); ++i) {
                if (normDist >= curve[i].first && normDist <= curve[i+1].first) {
                    double t = (normDist - curve[i].first) / (curve[i+1].first - curve[i].first);
                    survivalProb = curve[i].second + t * (curve[i+1].second - curve[i].second);
                    break;
                }
            }
        }

        if (prng.NextDouble() <= survivalProb)
            survivors.push_back(inst);
    }
    return survivors;
}

} // namespace foliage

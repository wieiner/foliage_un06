// foliage_un06 — terrain sampling interface + synthetic test terrain.
// Pluggable: swap the sampler to match a real engine heightfield.
#pragma once

#include <cmath>

namespace foliage {

// ---------------------------------------------------------------------------
// TerrainSampler — abstract callback for height + normal queries
// ---------------------------------------------------------------------------
struct TerrainSampler {
    virtual ~TerrainSampler() = default;

    // Return world height at (x, z). Negative infinity = no surface (void).
    virtual double Height(double x, double z) const = 0;

    // Return slope angle in degrees at (x, z). Computed via finite differences.
    virtual double Slope(double x, double z) const {
        const double h = Height(x, z);
        const double dx = Height(x + 1.0, z) - h;
        const double dz = Height(x, z + 1.0) - h;
        return std::atan(std::sqrt(dx * dx + dz * dz)) * 57.29577951308232;
    }

    // Normal at (x, z). Default: finite-difference normal, pointing up.
    virtual void Normal(double x, double z, double* nx, double* ny, double* nz) const {
        const double eps = 0.5;
        const double h = Height(x, z);
        const double dx = h - Height(x + eps, z);
        const double dz = h - Height(x, z + eps);
        const double len = std::sqrt(dx * dx + dz * dz + 1.0);
        *nx = dx / len;
        *ny = 1.0 / len;
        *nz = dz / len;
    }
};

// ---------------------------------------------------------------------------
// SyntheticTerrain — sine-wave hills (matches existing bus_handler terrain)
// ---------------------------------------------------------------------------
struct SyntheticTerrain : TerrainSampler {
    double Height(double x, double z) const override {
        return 4.0 * std::sin(x * 0.07) * std::cos(z * 0.06)
             + 2.0 * std::sin(x * 0.013 + z * 0.017);
    }
};

// ---------------------------------------------------------------------------
// FlatTerrain — zero-height plane (useful for testing)
// ---------------------------------------------------------------------------
struct FlatTerrain : TerrainSampler {
    double Height(double, double) const override { return 0.0; }
    double Slope(double, double) const override { return 0.0; }
    void Normal(double, double, double* nx, double* ny, double* nz) const override {
        *nx = 0.0; *ny = 1.0; *nz = 0.0;
    }
};

} // namespace foliage

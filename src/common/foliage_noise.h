// foliage_un06 — Perlin noise for density modulation.
// 2D/3D noise functions for natural-looking scatter variation.
#pragma once

#include "foliage_type.h"
#include <cmath>
#include <vector>

namespace foliage {

// ---------------------------------------------------------------------------
// PerlinNoise — 2D Perlin noise generator
// ---------------------------------------------------------------------------
class PerlinNoise {
public:
    PerlinNoise(unsigned seed = 1337);

    // 2D Perlin noise value in [0,1]
    double Noise(double x, double y) const;

    // Fractal Brownian Motion (octaves of Perlin, weighted)
    double Fbm(double x, double y, int octaves = 4, double lacunarity = 2.0, double gain = 0.5) const;

    // Ridged multifractal (absolute value of fbm) — creates ridge-like patterns
    double Ridged(double x, double y, int octaves = 4, double lacunarity = 2.0, double gain = 0.5) const;

    // Set seed (reinitialises permutation table)
    void Seed(unsigned seed);

private:
    static constexpr int kSize = 256;
    static constexpr int kMask = kSize - 1;
    unsigned char perm_[kSize * 2];

    double Fade(double t) const;
    double Lerp(double a, double b, double t) const;
    double Grad(int hash, double x, double y) const;
    double RawNoise(double x, double y) const;
};

// ---------------------------------------------------------------------------
// NoiseDensityModulator — applies noise map to instance placement
// ---------------------------------------------------------------------------
struct NoiseModulationParams {
    double scale      = 50.0;   // noise frequency scale (larger = smoother)
    int    octaves    = 4;      // FBM octaves
    double lacunarity = 2.0;    // frequency multiplier per octave
    double gain       = 0.5;    // amplitude multiplier per octave
    double threshold  = 0.5;    // keep instances where noise > threshold
    double power      = 1.0;    // boost: noise_value ^ power
    bool   invert     = false;  // invert threshold
    bool   ridged     = false;  // use ridged noise instead of FBM
    unsigned seed     = 1337;
};

// Filter instances based on noise threshold at their position.
// Each instance is evaluated against the noise field; those below
// threshold are removed (probabilistically near the threshold).
std::vector<FoliageInstance> ApplyNoiseModulation(
    const std::vector<FoliageInstance>& instances,
    const NoiseModulationParams&        params);

// Generate a noise density multiplier at a point (for placement-time use).
// Returns [0,1] — multiply FoliageType.density by this for natural variation.
double NoiseDensityMultiplier(double x, double z,
                               const NoiseModulationParams& params);

} // namespace foliage

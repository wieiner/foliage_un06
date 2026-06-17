// foliage_un06 — Perlin noise implementation.
#include "foliage_noise.h"
#include "foliage_prng.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace foliage {

// ---------------------------------------------------------------------------
// PerlinNoise
// ---------------------------------------------------------------------------
namespace {

unsigned char MakePerm(unsigned seed, int i) {
    // Quick perm table via xorshift
    unsigned s = seed + static_cast<unsigned>(i) * 0x9E3779B9u;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return static_cast<unsigned char>(s & 0xFF);
}

} // namespace

PerlinNoise::PerlinNoise(unsigned seed) { Seed(seed); }

void PerlinNoise::Seed(unsigned seed) {
    unsigned char p[kSize];
    for (int i = 0; i < kSize; ++i) p[i] = static_cast<unsigned char>(i);
    // Fisher-Yates shuffle via PRNG
    FoliagePrng prng(seed);
    for (int i = kSize - 1; i > 0; --i) {
        int j = static_cast<int>(prng.NextU64() % (static_cast<unsigned>(i) + 1));
        std::swap(p[i], p[j]);
    }
    for (int i = 0; i < kSize; ++i) perm_[i] = perm_[i + kSize] = p[i];
}

double PerlinNoise::Fade(double t) const { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
double PerlinNoise::Lerp(double a, double b, double t) const { return a + t * (b - a); }

double PerlinNoise::Grad(int hash, double x, double y) const {
    int h = hash & 7;
    double u = (h < 4) ? x : y;
    double v = (h < 4) ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
}

double PerlinNoise::RawNoise(double x, double y) const {
    int ix = static_cast<int>(std::floor(x)) & kMask;
    int iy = static_cast<int>(std::floor(y)) & kMask;
    double xf = x - std::floor(x), yf = y - std::floor(y);
    double u = Fade(xf), v = Fade(yf);
    int aa = perm_[perm_[ix] + iy], ab = perm_[perm_[ix] + iy + 1];
    int ba = perm_[perm_[ix + 1] + iy], bb = perm_[perm_[ix + 1] + iy + 1];
    return Lerp(Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u),
                Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u), v);
}

double PerlinNoise::Noise(double x, double y) const {
    return (RawNoise(x, y) + 1.0) * 0.5; // [-1,1] → [0,1]
}

double PerlinNoise::Fbm(double x, double y, int octaves, double lacunarity, double gain) const {
    double value = 0.0, amplitude = 1.0, frequency = 1.0, maxVal = 0.0;
    for (int i = 0; i < octaves; ++i) {
        value += amplitude * RawNoise(x * frequency, y * frequency);
        maxVal += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return (value / maxVal + 1.0) * 0.5; // normalise to [0,1]
}

double PerlinNoise::Ridged(double x, double y, int octaves, double lacunarity, double gain) const {
    double value = 0.0, amplitude = 1.0, frequency = 1.0, maxVal = 0.0;
    for (int i = 0; i < octaves; ++i) {
        double n = std::abs(RawNoise(x * frequency, y * frequency));
        value += amplitude * (1.0 - n); // invert: ridges have higher density
        maxVal += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return value / maxVal; // already [0,1]
}

// ---------------------------------------------------------------------------
// NoiseDensityModulator
// ---------------------------------------------------------------------------
double NoiseDensityMultiplier(double x, double z, const NoiseModulationParams& params) {
    PerlinNoise noise(params.seed);
    double n;
    if (params.ridged) {
        n = noise.Ridged(x / params.scale, z / params.scale,
                         params.octaves, params.lacunarity, params.gain);
    } else {
        n = noise.Fbm(x / params.scale, z / params.scale,
                      params.octaves, params.lacunarity, params.gain);
    }
    // Apply power curve
    if (params.power != 1.0) n = std::pow(n, params.power);
    // Apply threshold
    if (params.invert) n = 1.0 - n;
    return n;
}

std::vector<FoliageInstance> ApplyNoiseModulation(
    const std::vector<FoliageInstance>& instances,
    const NoiseModulationParams&        params)
{
    PerlinNoise noise(params.seed);
    std::vector<FoliageInstance> survivors;
    FoliagePrng prng(params.seed ^ 0xABCD);

    for (const auto& inst : instances) {
        double n;
        if (params.ridged) {
            n = noise.Ridged(inst.x / params.scale, inst.z / params.scale,
                             params.octaves, params.lacunarity, params.gain);
        } else {
            n = noise.Fbm(inst.x / params.scale, inst.z / params.scale,
                          params.octaves, params.lacunarity, params.gain);
        }
        if (params.power != 1.0) n = std::pow(n, params.power);
        if (params.invert) n = 1.0 - n;

        // Probabilistic around threshold for smoother transitions
        if (n >= params.threshold) {
            survivors.push_back(inst);
        } else if (n >= params.threshold * 0.7) {
            // Transition zone: keep with probability proportional to noise
            double chance = (n - params.threshold * 0.7) / (params.threshold * 0.3);
            if (prng.NextDouble() < chance) survivors.push_back(inst);
        }
    }
    return survivors;
}

} // namespace foliage

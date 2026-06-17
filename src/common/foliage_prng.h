// foliage_un06 — deterministic PRNG (xoshiro256**).
// Seed-stable across platforms; identical seed → identical scatter.
#pragma once

#include <cstdint>

namespace foliage {

class FoliagePrng {
public:
    explicit FoliagePrng(std::uint64_t seed = 1337) { Seed(seed); }

    void Seed(std::uint64_t seed) {
        // SplitMix64 to seed xoshiro state from a single 64-bit value
        for (int i = 0; i < 4; ++i) {
            seed += 0x9E3779B97F4A7C15ULL;
            std::uint64_t z = seed;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            s_[i] = z ^ (z >> 31);
        }
    }

    // Uniform [0, 1) double
    double NextDouble() {
        return static_cast<double>(NextU64() >> 11) * 0x1.0p-53;
    }

    // Uniform [a, b) double
    double NextRange(double a, double b) {
        return a + (b - a) * NextDouble();
    }

    std::uint64_t NextU64() {
        const std::uint64_t result = Rotl(s_[1] * 5, 7) * 9;
        const std::uint64_t t = s_[1] << 17;

        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];

        s_[2] ^= t;
        s_[3] = Rotl(s_[3], 45);

        return result;
    }

    // Poisson disc candidate: uniform point in [0,w)×[0,d)
    void NextPoint(double w, double d, double* x, double* z) {
        *x = NextRange(0.0, w);
        *z = NextRange(0.0, d);
    }

private:
    static std::uint64_t Rotl(std::uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    std::uint64_t s_[4] = {0, 0, 0, 0};
};

} // namespace foliage

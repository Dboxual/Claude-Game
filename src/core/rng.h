// Small deterministic RNG (PCG32). Seeded generators keep world generation
// reproducible — the same seed must always build the same zone.
#pragma once
#include <cstdint>

class Rng {
public:
    explicit Rng(uint64_t seed = 0x853c49e6748fea9bULL) { Reseed(seed); }

    void Reseed(uint64_t seed) {
        state = 0u; inc = (seed << 1u) | 1u;
        NextU32(); state += seed; NextU32();
    }

    uint32_t NextU32() {
        uint64_t old = state;
        state = old * 6364136223846793005ULL + inc;
        uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = (uint32_t)(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    // [0, 1)
    float Next01() { return (NextU32() >> 8) * (1.0f / 16777216.0f); }
    float Range(float lo, float hi) { return lo + (hi - lo) * Next01(); }
    int   RangeI(int lo, int hi) { // inclusive both ends
        return lo + (int)(NextU32() % (uint32_t)(hi - lo + 1));
    }
    bool Chance(float p) { return Next01() < p; }

private:
    uint64_t state = 0, inc = 0;
};

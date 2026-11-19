/**
 * @file paper_rng.c
 * @brief Deterministic PRNG (PCG32) and 2D value noise for procedural
 * paper textures. Same seed => identical output.
 */

#include "paper_internals.h"

/* ------------------------------------------------------------------ */
/* PCG32                                                              */
/* ------------------------------------------------------------------ */

void paper_rng_seed(paper_rng_t *rng, uint32_t seed) {
    rng->state = 0u;
    rng->inc = ((uint64_t)seed * 2u + 1u);
    (void)paper_rng_next(rng); /* one warmup step */
    rng->state += ((uint64_t)seed * 0x9E3779B97F4A7C15ULL);
    (void)paper_rng_next(rng);
}

uint32_t paper_rng_next(paper_rng_t *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    int rot = (int)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
}

double paper_rng_01(paper_rng_t *rng) {
    return (double)paper_rng_next(rng) * (1.0 / 4294967296.0);
}

/* ------------------------------------------------------------------ */
/* Integer hash -> spatial noise (seedable, no lookup table)         */
/* ------------------------------------------------------------------ */

static uint32_t paper_hash32(uint32_t a, uint32_t b, uint32_t seed) {
    uint32_t h = a * 0x9E3779B1u + seed;
    h ^= b * 0x85EBCA77u + 0x1234567Fu;
    h ^= h >> 16u;
    h *= 0x21F0AAADu;
    h ^= h >> 15u;
    return h;
}

static double paper_hash01(int32_t a, int32_t b, uint32_t seed) {
    return (double)paper_hash32((uint32_t)a, (uint32_t)b, seed) *
           (1.0 / 4294967296.0);
}

/* ------------------------------------------------------------------ */
/* 2D value noise                                                     */
/* ------------------------------------------------------------------ */

static double paper_lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

static double paper_smoothstep(double t) {
    return t * t * (3.0 - 2.0 * t);
}

/** Value noise at pixel coordinates (x, y), range [0,1). */
double paper_value_noise(int32_t x, int32_t y, uint32_t seed) {
    int32_t ix = x;
    int32_t iy = y;
    double fx = 0.0, fy = 0.0; /* cell-aligned: no sub-pixel here */
    double n00 = paper_hash01(ix,     iy,     seed);
    double n10 = paper_hash01(ix + 1, iy,     seed);
    double n01 = paper_hash01(ix,     iy + 1, seed);
    double n11 = paper_hash01(ix + 1, iy + 1, seed);
    double tx = paper_smoothstep(fx);
    double ty = paper_smoothstep(fy);
    double n0 = paper_lerp(n00, n10, tx);
    double n1 = paper_lerp(n01, n11, tx);
    return paper_lerp(n0, n1, ty);
}

/** Fractional Brownian motion over value noise, range [0,1). */
double paper_fbm(int32_t x, int32_t y, uint32_t seed,
                        int octaves, double lacunarity, double gain) {
    double total = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double maxamp = 0.0;
    int i;
    for (i = 0; i < octaves; i++) {
        int32_t sx = (int32_t)(x * freq);
        int32_t sy = (int32_t)(y * freq);
        total += paper_value_noise(sx, sy, seed) * amp;
        maxamp += amp;
        amp *= gain;
        freq = (int32_t)(freq * lacunarity);
    }
    if (maxamp <= 0.0) return 0.0;
    return paper_clamp01(total / maxamp);
}

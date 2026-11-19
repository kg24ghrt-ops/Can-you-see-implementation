#ifndef PAPER_INTERNALS_H
#define PAPER_INTERNALS_H

#include "paper.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Platform export macros. On Windows DLLs these would be __declspec; on
 * Android/WASM shared builds they map to visibility attributes. */
#ifdef _WIN32
#   ifdef PAPER_BUILDING_DLL
#       define PAPER_API __declspec(dllexport)
#   else
#       define PAPER_API __declspec(dllimport)
#   endif
#else
#   if defined(PAPER_BUILDING_DLL) && defined(__GNUC__)
#       define PAPER_API __attribute__((visibility("default")))
#   else
#       define PAPER_API
#   endif
#endif

/* ------------------------------------------------------------------ */
/* Internal fixed-point-friendly helpers                              */
/* ------------------------------------------------------------------ */

#define PAPER_MM_PER_INCH 25.4

static inline double paper_mm_to_px(double mm, double dpi) {
    return mm * dpi / PAPER_MM_PER_INCH;
}

static inline double paper_px_to_mm(double px, double dpi) {
    return px * PAPER_MM_PER_INCH / dpi;
}

static inline double paper_clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static inline int paper_round(double v) {
    return (int)(v + 0.5);
}

static inline uint8_t paper_d2u(double v) {
    if (v < 0.0) return 0;
    if (v > 1.0) return 255;
    return (uint8_t)(v * 255.0 + 0.5);
}

/* ------------------------------------------------------------------ */
/* Deterministic PRNG (PCG32) - paper_rng.c                           */
/* ------------------------------------------------------------------ */

typedef struct {
    uint64_t state;
    uint64_t inc;
} paper_rng_t;

void paper_rng_seed(paper_rng_t *rng, uint32_t seed);
uint32_t paper_rng_next(paper_rng_t *rng);
double paper_rng_01(paper_rng_t *rng);

/** Value noise at pixel coordinates (x, y), range [0,1). */
double paper_value_noise(int32_t x, int32_t y, uint32_t seed);

/** Fractional Brownian motion over value noise, range [0,1). */
double paper_fbm(int32_t x, int32_t y, uint32_t seed,
                 int octaves, double lacunarity, double gain);

size_t paper_format_bytes(paper_pixel_format_t fmt);

/* ------------------------------------------------------------------ */
/* Material / layout / strokes (implementation modules)               */
/* ------------------------------------------------------------------ */

paper_status_t paper_material_render(paper_engine_t *e,
                                     uint8_t *out, int w, int h, int stride);
paper_status_t paper_layout_render(paper_engine_t *e,
                                   uint8_t *out, int w, int h, int stride);
paper_status_t paper_stroke_render_all(paper_engine_t *e,
                                       uint8_t *out, int w, int h, int stride);

/* ------------------------------------------------------------------ */
/* Renderer (paper_renderer.c)                                        */
/* ------------------------------------------------------------------ */

paper_status_t paper_renderer_prepare(paper_engine_t *e);
paper_status_t paper_renderer_render(paper_engine_t *e,
                                     paper_pixel_buffer_t *buf);
void paper_renderer_clear_canvas(paper_engine_t *e);

/* ------------------------------------------------------------------ */
/* Internal engine state                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    paper_size_id_t size_id;
    paper_dimensions_t dims;        /**< effective physical dims (mm) */
    paper_orientation_t orientation;
    double dpi;
    uint32_t seed;
    paper_material_id_t material;
    paper_material_params_t material_params;
    paper_layout_id_t layout;
    paper_layout_params_t layout_params;
    paper_pixel_format_t pixel_format;
} paper_config_t;

struct paper_engine {
    paper_config_t cfg;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint8_t *pixels;       /**< engine-owned RGBA8888 working canvas */
    size_t pixels_size;

    /* Queued strokes (simple dynamic array). */
    paper_stroke_t *strokes;
    size_t stroke_count;
    size_t stroke_capacity;

    int configured;        /**< non-zero once dims/dpi resolved */
    int dirty;             /**< non-zero when a re-render is required */
};

#endif /* PAPER_INTERNALS_H */

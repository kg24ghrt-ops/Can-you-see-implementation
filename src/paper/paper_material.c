/**
 * @file paper_material.c
 * @brief Procedural paper material generation.
 *
 * Produces a natural, non-uniform sheet: base colour, low-frequency
 * luminance variation, fine grain and scattered fibres. Fully deterministic
 * for a given seed. No platform rendering calls.
 */

#include "paper_internals.h"

typedef struct {
    double r, g, b;
    double grain, fiber, luminance_var, edge_var;
} paper_material_desc_t;

static void paper_material_describe(paper_material_id_t id,
                                    const paper_material_params_t *custom,
                                    paper_material_desc_t *d) {
    if (id == PAPER_MATERIAL_CUSTOM && custom != NULL) {
        d->r = custom->r; d->g = custom->g; d->b = custom->b;
        d->grain = custom->grain; d->fiber = custom->fiber;
        d->luminance_var = custom->luminance_var;
        d->edge_var = custom->edge_var;
        return;
    }
    switch (id) {
        case PAPER_MATERIAL_NATURAL_WHITE:
            d->r = 0.975; d->g = 0.972; d->b = 0.962;
            d->grain = 0.035; d->fiber = 0.06; d->luminance_var = 0.03; d->edge_var = 0.015; break;
        case PAPER_MATERIAL_WARM_WHITE:
            d->r = 0.985; d->g = 0.950; d->b = 0.895;
            d->grain = 0.04; d->fiber = 0.055; d->luminance_var = 0.035; d->edge_var = 0.015; break;
        case PAPER_MATERIAL_CREAM:
            d->r = 1.000; d->g = 0.970; d->b = 0.850;
            d->grain = 0.045; d->fiber = 0.07; d->luminance_var = 0.04; d->edge_var = 0.02; break;
        case PAPER_MATERIAL_IVORY:
            d->r = 1.000; d->g = 0.990; d->b = 0.900;
            d->grain = 0.03; d->fiber = 0.05; d->luminance_var = 0.025; d->edge_var = 0.01; break;
        case PAPER_MATERIAL_AGED:
            d->r = 0.945; d->g = 0.875; d->b = 0.710;
            d->grain = 0.05; d->fiber = 0.09; d->luminance_var = 0.05; d->edge_var = 0.025; break;
        case PAPER_MATERIAL_RECYCLED:
            d->r = 0.900; d->g = 0.875; d->b = 0.840;
            d->grain = 0.06; d->fiber = 0.10; d->luminance_var = 0.04; d->edge_var = 0.02; break;
        default: /* NATURAL_WHITE */
            d->r = 0.975; d->g = 0.972; d->b = 0.962;
            d->grain = 0.035; d->fiber = 0.06; d->luminance_var = 0.03; d->edge_var = 0.015; break;
    }
    d->grain = paper_clamp01(d->grain);
    d->fiber = paper_clamp01(d->fiber);
    d->luminance_var = paper_clamp01(d->luminance_var);
    d->edge_var = paper_clamp01(d->edge_var);
}

/** Antialiased darkening brush: add a tint along a short segment. */
static void paper_draw_fiber(uint8_t *out, int w, int h, int stride,
                             double x0, double y0, double x1, double y1,
                             double r, double g, double b, double opacity) {
    double dx = x1 - x0, dy = y1 - y0;
    double len = sqrt(dx * dx + dy * dy);
    if (len <= 0.0) return;
    double ux = dx / len, uy = dy / len;
    double nx = -uy, ny = ux; /* normal */
    int minx = (int)floor(fmin(x0, x1) - 2.0);
    int maxx = (int)ceil(fmax(x0, x1) + 2.0);
    int miny = (int)floor(fmin(y0, y1) - 2.0);
    int maxy = (int)ceil(fmax(y0, y1) + 2.0);
    if (minx < 0) minx = 0; if (miny < 0) miny = 0;
    if (maxx > w - 1) maxx = w - 1; if (maxy > h - 1) maxy = h - 1;
    double halfw = 0.7;
    int y, x;
    for (y = miny; y <= maxy; y++) {
        for (x = minx; x <= maxx; x++) {
            double px = (double)x + 0.5, py = (double)y + 0.5;
            double proj = (px - x0) * ux + (py - y0) * uy;
            if (proj < 0.0 || proj > len) continue;
            double dist = fabs((px - x0) * nx + (py - y0) * ny);
            if (dist > halfw + 1.0) continue;
            double cover = 1.0 - paper_clamp01((dist - halfw) * 1.5);
            if (cover <= 0.0) continue;
            double a = cover * opacity;
            if (a <= 0.0) continue;
            size_t off = (size_t)y * stride + (size_t)x * 4;
            double inv = 1.0 - a;
            out[off + 0] = (uint8_t)(out[off + 0] * inv + paper_d2u(r) * a);
            out[off + 1] = (uint8_t)(out[off + 1] * inv + paper_d2u(g) * a);
            out[off + 2] = (uint8_t)(out[off + 2] * inv + paper_d2u(b) * a);
        }
    }
}

paper_status_t paper_material_render(paper_engine_t *e,
                                     uint8_t *out, int w, int h, int stride) {
    if (e == NULL || out == NULL || w <= 0 || h <= 0 || stride <= 0)
        return PAPER_ERR_INVALID_ARGUMENT;

    paper_material_desc_t m;
    paper_material_describe(e->cfg.material, &e->cfg.material_params, &m);

    uint32_t seed = e->cfg.seed;
    int y, x;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            /* Low-frequency luminance variation (non-uniform colouration). */
            double lv = paper_fbm(x / 8, y / 8, seed, 2, 2.0, 0.5);
            lv = 1.0 + (lv - 0.5) * 2.0 * m.luminance_var;
            if (lv < 0.0) lv = 0.0;
            if (lv > 1.0) lv = 1.0;

            /* Fine grain. */
            double grain = paper_value_noise(x, y, seed);
            grain = 1.0 + (grain - 0.5) * 2.0 * m.grain;
            if (grain < 0.0) grain = 0.0;
            if (grain > 1.0) grain = 1.0;

            double factor = lv * grain;
            uint8_t pr = paper_d2u(m.r * factor);
            uint8_t pg = paper_d2u(m.g * factor);
            uint8_t pb = paper_d2u(m.b * factor);

            /* Edge darkening with irregularity. */
            double dxn = (double)x / (double)(w - 1);
            double dyn = (double)y / (double)(h - 1);
            double edge = fmin(fmin(dxn, 1.0 - dxn), fmin(dyn, 1.0 - dyn));
            double edge_noise = paper_value_noise(x / 3, y / 3, seed + 1);
            edge = fmin(edge + edge_noise * 0.05, 1.0);
            double edge_darken = 1.0 - edge * m.edge_var * 0.4;
            if (edge_darken < 0.0) edge_darken = 0.0;

            pr = (uint8_t)((double)pr * edge_darken + 0.5);
            pg = (uint8_t)((double)pg * edge_darken + 0.5);
            pb = (uint8_t)((double)pb * edge_darken + 0.5);

            size_t off = (size_t)y * stride + (size_t)x * 4;
            out[off + 0] = pr;
            out[off + 1] = pg;
            out[off + 2] = pb;
            out[off + 3] = 255;
        }
    }

    /* Scattered fibres. Count scales with area but stays bounded. */
    double area = (double)w * (double)h;
    int fiber_count = (int)(area * m.fiber * 0.0000015);
    if (fiber_count < 4) fiber_count = 4;
    if (fiber_count > 400) fiber_count = 400;

    paper_rng_t rng;
    paper_rng_seed(&rng, seed);
    int i;
    for (i = 0; i < fiber_count; i++) {
        double fx = paper_rng_01(&rng) * (double)(w - 1);
        double fy = paper_rng_01(&rng) * (double)(h - 1);
        double angle = paper_rng_01(&rng) * 6.28318530718;
        double flen = 1.0 + paper_rng_01(&rng) * 3.0;
        double x1 = fx + cos(angle) * flen;
        double y1 = fy + sin(angle) * flen;
        double r = m.r * 0.55;
        double g = m.g * 0.55;
        double b = m.b * 0.55;
        double opacity = 0.25 + paper_rng_01(&rng) * 0.2;
        paper_draw_fiber(out, w, h, stride, fx, fy, x1, y1, r, g, b, opacity);
    }

    (void)e;
    return PAPER_OK;
}

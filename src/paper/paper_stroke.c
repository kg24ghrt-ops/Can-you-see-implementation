/**
 * @file paper_stroke.c
 * @brief Handwriting stroke rendering: variable-width, pressure-sensitive,
 * anti-aliased strokes that blend into the paper material rather than sitting
 * as a flat overlay.
 */

#include "paper_internals.h"

static double paper_seg_dist(double px, double py,
                             double x0, double y0, double x1, double y1) {
    double dx = x1 - x0, dy = y1 - y0;
    double len2 = dx * dx + dy * dy;
    if (len2 == 0.0) {
        double tdx = px - x0, tdy = py - y0;
        return sqrt(tdx * tdx + tdy * tdy);
    }
    double t = ((px - x0) * dx + (py - y0) * dy) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double qx = x0 + t * dx;
    double qy = y0 + t * dy;
    double tdx = px - qx, tdy = py - qy;
    return sqrt(tdx * tdx + tdy * tdy);
}

static double paper_sat(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/** Blend an ink colour into one RGBA pixel with premultiplied alpha `a`. */
static void paper_blend_pixel(uint8_t *out, size_t off,
                              double r, double g, double b, double a) {
    if (a <= 0.0) return;
    if (a > 1.0) a = 1.0;
    uint8_t cr = paper_d2u(r), cg = paper_d2u(g), cb = paper_d2u(b);
    double inv = 1.0 - a;
    out[off + 0] = (uint8_t)(out[off + 0] * inv + cr * a);
    out[off + 1] = (uint8_t)(out[off + 1] * inv + cg * a);
    out[off + 2] = (uint8_t)(out[off + 2] * inv + cb * a);
}

static void paper_render_segment(uint8_t *out, int w, int h, int stride,
                                 double x0, double y0, double w0, double p0,
                                 double x1, double y1, double w1, double p1,
                                 double r, double g, double b,
                                 double opacity, int pencil) {
    double dx = x1 - x0, dy = y1 - y0;
    double seg_len = sqrt(dx * dx + dy * dy);
    if (seg_len <= 0.0) return;

    double ux = dx / seg_len, uy = dy / seg_len;

    double hw0 = w0 * 0.5;
    double hw1 = w1 * 0.5;
    double pad = 1.5;
    double minx = fmin(x0, x1) - fmax(hw0, hw1) - pad;
    double maxx = fmax(x0, x1) + fmax(hw0, hw1) + pad;
    double miny = fmin(y0, y1) - fmax(hw0, hw1) - pad;
    double maxy = fmax(y0, y1) + fmax(hw0, hw1) + pad;

    int ix0 = (int)floor(minx), ix1 = (int)ceil(maxx);
    int iy0 = (int)floor(miny), iy1 = (int)ceil(maxy);
    if (ix0 < 0) ix0 = 0; if (iy0 < 0) iy0 = 0;
    if (ix1 > w - 1) ix1 = w - 1; if (iy1 > h - 1) iy1 = h - 1;

    /* Optional ink spreading (bleed) pass: faint, wider, drawn first. */
    int bleed = 0;

    int y, x;
    for (y = iy0; y <= iy1; y++) {
        double py = (double)y + 0.5;
        for (x = ix0; x <= ix1; x++) {
            double px = (double)x + 0.5;
            double dist = paper_seg_dist(px, py, x0, y0, x1, y1);
            double proj = (px - x0) * ux + (py - y0) * uy;
            if (proj < 0.0 || proj > seg_len) continue;
            double t = proj / seg_len;
            double hw = hw0 + (hw1 - hw0) * t;
            double pressure = p0 + (p1 - p0) * t;
            if (pressure < 0.0) pressure = 0.0;
            if (pressure > 1.0) pressure = 1.0;
            double eff_hw = hw * (0.8 + 0.4 * pressure);
            double cover = paper_sat(1.0 - (dist - eff_hw) / 1.2, 0.0, 1.0);
            if (cover <= 0.0) continue;
            double alpha = cover * opacity * pressure;
            if (alpha <= 0.0) continue;
            /* Pencil: slightly grainy, lower saturation. */
            if (pencil) {
                double grain = paper_value_noise(x, y, 0x9E3779B9u);
                alpha *= (0.85 + 0.3 * grain);
            }
            size_t off = (size_t)y * stride + (size_t)x * 4;
            if (bleed) {
                paper_blend_pixel(out, off, r, g, b, alpha * 0.18);
            }
            paper_blend_pixel(out, off, r, g, b, alpha);
        }
    }
}

paper_status_t paper_stroke_render_all(paper_engine_t *e,
                                       uint8_t *out, int w, int h, int stride) {
    if (e == NULL || out == NULL || w <= 0 || h <= 0 || stride <= 0)
        return PAPER_ERR_INVALID_ARGUMENT;
    if (e->stroke_count == 0) return PAPER_OK;

    double dpi = e->cfg.dpi;
    double scale = dpi / PAPER_MM_PER_INCH;
    size_t i;
    for (i = 0; i < e->stroke_count; i++) {
        const paper_stroke_t *s = &e->strokes[i];
        if (s->count < 2 || s->points == NULL) continue;
        double opacity = paper_sat(s->opacity, 0.0, 1.0);
        int pencil = (s->ink_type == PAPER_INK_PENCIL);
        size_t j;
        for (j = 1; j < s->count; j++) {
            const paper_stroke_point_t *a = &s->points[j - 1];
            const paper_stroke_point_t *b = &s->points[j];
            double x0 = a->x * scale, y0 = a->y * scale;
            double x1 = b->x * scale, y1 = b->y * scale;
            double w0 = a->width * scale;
            double w1 = b->width * scale;
            paper_render_segment(out, w, h, stride,
                                 x0, y0, w0, a->pressure,
                                 x1, y1, w1, b->pressure,
                                 s->r, s->g, s->b, opacity, pencil);
        }
    }
    return PAPER_OK;
}

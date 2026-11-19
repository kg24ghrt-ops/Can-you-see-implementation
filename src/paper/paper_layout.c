/**
 * @file paper_layout.c
 * @brief Ruled / graph / dot-grid / margin layouts rendered in physical
 * units (millimetres) then converted to pixels at the configured DPI.
 */

#include "paper_internals.h"

typedef struct {
    double spacing_mm;
    double thickness_mm;
    double opacity;
    double r, g, b;
} layout_rule_t;

static void layout_rule_preset(paper_layout_id_t id,
                               layout_rule_t *h, layout_rule_t *v) {
    /* Defaults: light blue-grey ruled lines. */
    double r = 0.45, g = 0.55, b = 0.78;
    h->r = r; h->g = g; h->b = b;
    v->r = r; v->g = g; v->b = b;
    h->opacity = 0.35; v->opacity = 0.35;
    h->thickness_mm = 0.08; v->thickness_mm = 0.08;

    switch (id) {
        case PAPER_LAYOUT_BLANK:
            h->spacing_mm = 0.0; v->spacing_mm = 0.0;
            h->opacity = 0.0; v->opacity = 0.0;
            break;
        case PAPER_LAYOUT_RULED:
            h->spacing_mm = 8.5; v->spacing_mm = 0.0;
            h->opacity = 0.35; break;
        case PAPER_LAYOUT_COLLEGE_RULED:
            h->spacing_mm = 7.144; /* 9/32 inch */
            h->opacity = 0.33; break;
        case PAPER_LAYOUT_WIDE_RULED:
            h->spacing_mm = 8.731; /* 11/32 inch */
            h->opacity = 0.35; break;
        case PAPER_LAYOUT_NARROW_RULED:
            h->spacing_mm = 6.35; /* 1/4 inch */
            h->opacity = 0.30; break;
        case PAPER_LAYOUT_GRAPH:
            h->spacing_mm = 5.0; v->spacing_mm = 5.0;
            h->thickness_mm = 0.06; v->thickness_mm = 0.06;
            h->opacity = 0.25; v->opacity = 0.25;
            break;
        case PAPER_LAYOUT_DOT_GRID:
            h->spacing_mm = 5.0; v->spacing_mm = 5.0;
            h->thickness_mm = 0.0; v->thickness_mm = 0.0;
            h->opacity = 0.30; v->opacity = 0.30;
            break;
        case PAPER_LAYOUT_MARGIN_RULED:
            h->spacing_mm = 8.5; v->spacing_mm = 0.0;
            h->opacity = 0.35; break;
        default:
            h->spacing_mm = 8.5; v->spacing_mm = 0.0;
            h->opacity = 0.35; break;
    }
}

static void paper_hline(uint8_t *out, int w, int h, int stride,
                        double cy, int x0, int x1,
                        double r, double g, double b, double alpha) {
    if (cy < -1.0 || cy > (double)h + 1.0) return;
    if (x0 < 0) x0 = 0; if (x1 > w - 1) x1 = w - 1;
    if (x0 > x1) return;
    int y0 = (int)floor(cy);
    double frac = cy - (double)y0;
    int rows[2] = {y0, y0 + 1};
    double covers[2] = {1.0 - frac, frac};
    uint8_t cr = paper_d2u(r), cg = paper_d2u(g), cb = paper_d2u(b);
    int k;
    for (k = 0; k < 2; k++) {
        int y = rows[k];
        if (y < 0 || y >= h) continue;
        double a = covers[k] * alpha;
        if (a <= 0.0) continue;
        double inv = 1.0 - a;
        size_t base = (size_t)y * stride;
        int x;
        for (x = x0; x <= x1; x++) {
            size_t off = base + (size_t)x * 4;
            out[off + 0] = (uint8_t)(out[off + 0] * inv + cr * a);
            out[off + 1] = (uint8_t)(out[off + 1] * inv + cg * a);
            out[off + 2] = (uint8_t)(out[off + 2] * inv + cb * a);
        }
    }
}

static void paper_vline(uint8_t *out, int w, int h, int stride,
                        double cx, int y0, int y1,
                        double r, double g, double b, double alpha) {
    if (cx < -1.0 || cx > (double)w + 1.0) return;
    if (y0 < 0) y0 = 0; if (y1 > h - 1) y1 = h - 1;
    if (y0 > y1) return;
    int x0 = (int)floor(cx);
    double frac = cx - (double)x0;
    int cols[2] = {x0, x0 + 1};
    double covers[2] = {1.0 - frac, frac};
    uint8_t cr = paper_d2u(r), cg = paper_d2u(g), cb = paper_d2u(b);
    int k;
    for (k = 0; k < 2; k++) {
        int x = cols[k];
        if (x < 0 || x >= w) continue;
        double a = covers[k] * alpha;
        if (a <= 0.0) continue;
        double inv = 1.0 - a;
        size_t base = (size_t)x * 4;
        int y;
        for (y = y0; y <= y1; y++) {
            size_t off = (size_t)y * stride + base;
            out[off + 0] = (uint8_t)(out[off + 0] * inv + cr * a);
            out[off + 1] = (uint8_t)(out[off + 1] * inv + cg * a);
            out[off + 2] = (uint8_t)(out[off + 2] * inv + cb * a);
        }
    }
}

static void paper_dot(uint8_t *out, int w, int h, int stride,
                      double cx, double cy, double radius,
                      double r, double g, double b, double alpha) {
    int minx = (int)floor(cx - radius - 1.0);
    int maxx = (int)ceil(cx + radius + 1.0);
    int miny = (int)floor(cy - radius - 1.0);
    int maxy = (int)ceil(cy + radius + 1.0);
    if (minx < 0) minx = 0; if (miny < 0) miny = 0;
    if (maxx > w - 1) maxx = w - 1; if (maxy > h - 1) maxy = h - 1;
    uint8_t cr = paper_d2u(r), cg = paper_d2u(g), cb = paper_d2u(b);
    double rr = radius * radius;
    int y, x;
    for (y = miny; y <= maxy; y++) {
        for (x = minx; x <= maxx; x++) {
            double dx = (double)x + 0.5 - cx;
            double dy = (double)y + 0.5 - cy;
            double d2 = dx * dx + dy * dy;
            if (d2 > rr) continue;
            double cover = paper_clamp01(1.0 - sqrt(d2) / radius);
            double a = cover * alpha;
            if (a <= 0.0) continue;
            double inv = 1.0 - a;
            size_t off = (size_t)y * stride + (size_t)x * 4;
            out[off + 0] = (uint8_t)(out[off + 0] * inv + cr * a);
            out[off + 1] = (uint8_t)(out[off + 1] * inv + cg * a);
            out[off + 2] = (uint8_t)(out[off + 2] * inv + cb * a);
        }
    }
}

paper_status_t paper_layout_render(paper_engine_t *e,
                                   uint8_t *out, int w, int h, int stride) {
    if (e == NULL || out == NULL || w <= 0 || h <= 0 || stride <= 0)
        return PAPER_ERR_INVALID_ARGUMENT;

    layout_rule_t hr, vr;
    layout_rule_preset(e->cfg.layout, &hr, &vr);
    if (hr.opacity <= 0.0 && vr.opacity <= 0.0) return PAPER_OK;

    double dpi = e->cfg.dpi;
    double lm_px = paper_mm_to_px(e->cfg.layout_params.left_margin_mm, dpi);
    double rm_px = paper_mm_to_px(e->cfg.layout_params.right_margin_mm, dpi);
    double tm_px = paper_mm_to_px(e->cfg.layout_params.top_margin_mm, dpi);
    double bm_px = paper_mm_to_px(e->cfg.layout_params.bottom_margin_mm, dpi);

    int x0 = (int)floor(lm_px);
    int x1 = (int)ceil((double)w - rm_px) - 1;
    int y_start = (int)floor(tm_px);
    int y_end = (int)ceil((double)h - bm_px) - 1;
    if (x0 < 0) x0 = 0;
    if (x1 > w - 1) x1 = w - 1;
    if (y_start < 0) y_start = 0;
    if (y_end > h - 1) y_end = h - 1;

    /* Horizontal rules. */
    if (hr.spacing_mm > 0.0 && hr.opacity > 0.0) {
        double spacing_px = paper_mm_to_px(hr.spacing_mm, dpi);
        double cy = (double)y_start;
        /* First line sits one spacing down from the top margin. */
        cy += spacing_px;
        while (cy <= (double)y_end + 1.0) {
            if (cy >= (double)y_start - 1.0)
                paper_hline(out, w, h, stride, cy, x0, x1,
                            hr.r, hr.g, hr.b, hr.opacity);
            cy += spacing_px;
        }
    }

    /* Vertical rules / dot grid. */
    if (vr.spacing_mm > 0.0 && vr.opacity > 0.0) {
        double spacing_px = paper_mm_to_px(vr.spacing_mm, dpi);
        double cx = (double)x0;
        while (cx <= (double)x1 + 1.0) {
            if (e->cfg.layout == PAPER_LAYOUT_DOT_GRID) {
                /* dots at intersections with horizontal rule positions */
                double cy2 = (double)y_start + paper_mm_to_px(hr.spacing_mm > 0 ? hr.spacing_mm : vr.spacing_mm, dpi);
                while (cy2 <= (double)y_end + 1.0) {
                    if (cy2 >= (double)y_start - 1.0)
                        paper_dot(out, w, h, stride, cx, cy2,
                                  0.9, vr.r, vr.g, vr.b, vr.opacity);
                    cy2 += paper_mm_to_px(hr.spacing_mm > 0 ? hr.spacing_mm : vr.spacing_mm, dpi);
                }
            } else if (vr.thickness_mm > 0.0) {
                if (cx >= (double)x0 - 1.0)
                    paper_vline(out, w, h, stride, cx, y_start, y_end,
                                vr.r, vr.g, vr.b, vr.opacity);
            }
            cx += spacing_px;
        }
    }

    return PAPER_OK;
}

/**
 * @file paper_renderer.c
 * @brief CPU rendering pipeline: paper material → layout rules → handwriting
 * strokes, all into an RGBA8888 working canvas, then converted to the
 * requested pixel format.
 */

#include "paper_internals.h"

static int32_t paper_canvas_stride(int32_t width) {
    return width * 4;
}

paper_status_t paper_renderer_prepare(paper_engine_t *e) {
    if (e == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (!(e->cfg.dims.width_mm > 0.0) || !(e->cfg.dims.height_mm > 0.0))
        return PAPER_ERR_INVALID_STATE;
    if (!(e->cfg.dpi > 0.0)) return PAPER_ERR_INVALID_STATE;

    int32_t w = paper_round(paper_mm_to_px(e->cfg.dims.width_mm, e->cfg.dpi));
    int32_t h = paper_round(paper_mm_to_px(e->cfg.dims.height_mm, e->cfg.dpi));
    if (w <= 0 || h <= 0) return PAPER_ERR_INVALID_STATE;
    int32_t stride = paper_canvas_stride(w);

    int recreated = 0;
    if (e->pixels == NULL || e->width != w || e->height != h || e->stride != stride) {
        if (e->pixels != NULL) free(e->pixels);
        size_t size = (size_t)h * (size_t)stride;
        e->pixels = (uint8_t *)calloc(1, size);
        if (e->pixels == NULL) return PAPER_ERR_OUT_OF_MEMORY;
        e->pixels_size = size;
        recreated = 1;
    }
    e->width = w;
    e->height = h;
    e->stride = stride;
    e->configured = 1;
    if (recreated || e->dirty) e->dirty = 1;
    return PAPER_OK;
}

/** Convert RGBA canvas to the requested output format into `buf`. */
static paper_status_t paper_renderer_emit(paper_engine_t *e,
                                          paper_pixel_buffer_t *buf) {
    if (buf->data == NULL) {
        paper_status_t st = paper_buffer_allocate(buf, buf->format,
                                                  e->width, e->height, buf->stride);
        if (st != PAPER_OK) return st;
    } else {
        if (buf->width < e->width || buf->height < e->height)
            return PAPER_ERR_INVALID_ARGUMENT;
        if (buf->stride == 0) buf->stride = e->width * (int32_t)paper_format_bytes(buf->format);
        if (buf->stride < e->width * (int32_t)paper_format_bytes(buf->format))
            return PAPER_ERR_INVALID_ARGUMENT;
    }
    buf->width = e->width;
    buf->height = e->height;

    size_t bpp = paper_format_bytes(buf->format);
    int y;
    for (y = 0; y < e->height; y++) {
        const uint8_t *src = e->pixels + (size_t)y * e->stride;
        uint8_t *dst = buf->data + (size_t)y * buf->stride;
        int x;
        for (x = 0; x < e->width; x++) {
            size_t so = (size_t)x * 4;
            size_t do_ = (size_t)x * bpp;
            dst[do_ + 0] = src[so + 0];
            dst[do_ + 1] = src[so + 1];
            dst[do_ + 2] = src[so + 2];
            if (bpp == 4) dst[do_ + 3] = src[so + 3];
        }
    }
    return PAPER_OK;
}

paper_status_t paper_renderer_render(paper_engine_t *e,
                                     paper_pixel_buffer_t *buf) {
    if (e == NULL || buf == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    paper_status_t st = paper_renderer_prepare(e);
    if (st != PAPER_OK) return st;

    /* Start each render from a clean canvas. */
    memset(e->pixels, 0, e->pixels_size);

    /* 1. Paper material. */
    st = paper_material_render(e, e->pixels, e->width, e->height, e->stride);
    if (st != PAPER_OK) return st;

    /* 2. Layout rules. */
    st = paper_layout_render(e, e->pixels, e->width, e->height, e->stride);
    if (st != PAPER_OK) return st;

    /* 3. Handwriting strokes. */
    st = paper_stroke_render_all(e, e->pixels, e->width, e->height, e->stride);
    if (st != PAPER_OK) return st;

    /* 4. Emit to caller buffer. */
    st = paper_renderer_emit(e, buf);
    if (st != PAPER_OK) return st;

    e->dirty = 0;
    return PAPER_OK;
}

void paper_renderer_clear_canvas(paper_engine_t *e) {
    if (e == NULL || e->pixels == NULL) return;
    memset(e->pixels, 0, e->pixels_size);
    e->dirty = 1;
}

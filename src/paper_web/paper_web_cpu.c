/**
 * @file paper_web_cpu.c
 * @brief CPU software backend for the web module. Always available; used as
 * the final fallback and as the testable implementation on non-Emscripten
 * targets. Copies the engine's RGBA8888 canvas into an internal buffer.
 */

#include "paper_web_internals.h"

typedef struct {
    uint8_t *pixels;
    size_t pixels_size;
    int32_t width;
    int32_t height;
} paper_web_cpu_t;

static void paper_web_cpu_destroy(paper_gpu_backend_t *b) {
    if (!b) return;
    paper_web_cpu_t *c = (paper_web_cpu_t *)b->priv;
    if (c) {
        free(c->pixels);
        free(c);
    }
}

static paper_status_t paper_web_cpu_init(paper_gpu_backend_t *b,
                                         void *canvas_element) {
    (void)b;
    (void)canvas_element;
    return PAPER_OK;
}

static paper_status_t paper_web_cpu_get_caps(paper_gpu_backend_t *b,
                                             paper_gpu_caps_t *caps) {
    if (!caps) return PAPER_ERR_INVALID_ARGUMENT;
    caps->webgpu_available = 0;
    caps->webgl2_available = 0;
    caps->backend = PAPER_BACKEND_CPU;
    caps->max_texture_size = PAPER_WEB_MAX_TEXTURE_SIZE;
    (void)b;
    return PAPER_OK;
}

static paper_status_t paper_web_cpu_resize(paper_gpu_backend_t *b,
                                           int32_t width, int32_t height) {
    if (!paper_web_size_valid(width, height))
        return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_cpu_t *c = (paper_web_cpu_t *)b->priv;
    size_t size = (size_t)height * (size_t)width * 4;
    uint8_t *pixels = (uint8_t *)realloc(c->pixels, size);
    if (!pixels && size > 0) return PAPER_ERR_OUT_OF_MEMORY;
    c->pixels = pixels;
    c->pixels_size = size;
    c->width = width;
    c->height = height;
    return PAPER_OK;
}

static paper_status_t paper_web_cpu_upload_texture(
    paper_gpu_backend_t *b, const uint8_t *rgba, int32_t width,
    int32_t height, int32_t stride) {
    if (!b || !rgba || width <= 0 || height <= 0 || stride <= 0)
        return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_cpu_t *c = (paper_web_cpu_t *)b->priv;
    if (!paper_web_size_valid(width, height))
        return PAPER_ERR_INVALID_ARGUMENT;
    if ((size_t)height * (size_t)stride > c->pixels_size) {
        /* Grow buffer if needed. */
        size_t need = (size_t)height * (size_t)stride;
        uint8_t *pixels = (uint8_t *)realloc(c->pixels, need);
        if (!pixels) return PAPER_ERR_OUT_OF_MEMORY;
        c->pixels = pixels;
        c->pixels_size = need;
    }
    int y;
    for (y = 0; y < height; y++) {
        memcpy(c->pixels + (size_t)y * (size_t)width * 4,
               rgba + (size_t)y * (size_t)stride,
               (size_t)width * 4);
    }
    c->width = width;
    c->height = height;
    return PAPER_OK;
}

static paper_status_t paper_web_cpu_present(paper_gpu_backend_t *b) {
    (void)b;
    return PAPER_OK;
}

static const paper_gpu_backend_vtable_t k_cpu_vtable = {
    .destroy = paper_web_cpu_destroy,
    .init = paper_web_cpu_init,
    .get_caps = paper_web_cpu_get_caps,
    .resize = paper_web_cpu_resize,
    .upload_texture = paper_web_cpu_upload_texture,
    .present = paper_web_cpu_present,
};

paper_gpu_backend_t *paper_gpu_backend_cpu_create(void) {
    paper_gpu_backend_t *b = (paper_gpu_backend_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    paper_web_cpu_t *c = (paper_web_cpu_t *)calloc(1, sizeof(*c));
    if (!c) {
        free(b);
        return NULL;
    }
    b->vt = &k_cpu_vtable;
    b->priv = c;
    return b;
}

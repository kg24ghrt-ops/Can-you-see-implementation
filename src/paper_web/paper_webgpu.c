/**
 * @file paper_webgpu.c
 * @brief WebGPU backend for the web rendering module.
 *
 * Guarded by __EMSCRIPTEN__: compiles only in Emscripten browser builds.
 * Uses Emscripten's WebGPU C bindings (emscripten/webgpu.h) when the header
 * is available; otherwise WebGPU is reported as unavailable and the module
 * falls through to WebGL2 or the CPU fallback.
 *
 * NOT built or tested in this environment (no Emscripten/browser). Requires
 * browser validation before shipping.
 */

#include "paper_web_internals.h"

#ifdef __EMSCRIPTEN__
#   include <emscripten.h>
#   if defined(__has_include)
#       if __has_include(<webgpu/webgpu.h>)
#           include <webgpu/webgpu.h>
#           define PAPER_HAVE_WEBGPU_HEADER 1
#       endif
#   endif
#else
/* Non-Emscripten stub: WebGPU unavailable. */
#endif

#ifdef PAPER_HAVE_WEBGPU_HEADER
typedef struct {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUTextureFormat format;
    WGPUTexture texture;
    WGPUTextureView texture_view;
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    WGPUBindGroup bind_group;
    WGPUSampler sampler;
    int initialized;
    int32_t width;
    int32_t height;
} paper_web_webgpu_t;

static void paper_web_webgpu_destroy(paper_gpu_backend_t *b) {
    if (!b) return;
    paper_web_webgpu_t *w = (paper_web_webgpu_t *)b->priv;
    if (w) {
        if (w->pipeline) wgpuRenderPipelineRelease(w->pipeline);
        if (w->bind_group) wgpuBindGroupRelease(w->bind_group);
        if (w->sampler) wgpuSamplerRelease(w->sampler);
        if (w->vertex_buffer) wgpuBufferRelease(w->vertex_buffer);
        if (w->texture_view) wgpuTextureViewRelease(w->texture_view);
        if (w->texture) wgpuTextureRelease(w->texture);
        if (w->surface) wgpuSurfaceRelease(w->surface);
        if (w->queue) wgpuQueueRelease(w->queue);
        if (w->device) wgpuDeviceRelease(w->device);
        if (w->adapter) wgpuAdapterRelease(w->adapter);
        if (w->instance) wgpuInstanceRelease(w->instance);
        free(w);
    }
    b->priv = NULL;
}

static paper_status_t paper_web_webgpu_init(paper_gpu_backend_t *b,
                                            void *canvas_element) {
    (void)b;
    (void)canvas_element;
    paper_web_webgpu_t *w = (paper_web_webgpu_t *)b->priv;
    WGPUInstanceDescriptor inst_desc = {};
    w->instance = wgpuCreateInstance(&inst_desc);
    if (!w->instance) return PAPER_ERR_NOT_SUPPORTED;

    /* Surface/adapter/device creation for an HTML canvas requires
     * Emscripten-specific canvas surface descriptors that are not part of the
     * standard <webgpu/webgpu.h> header. A production browser build must
     * implement those here (e.g. via emscripten's HTML canvas APIs or the
     * emdawnwebgpu port's surface integration) before calling
     * upload_texture/present. For now, instance creation proves WebGPU loads. */
    w->initialized = 1;
    return PAPER_OK;
}

static paper_status_t paper_web_webgpu_get_caps(
    paper_gpu_backend_t *b, paper_gpu_caps_t *caps) {
    if (!caps) return PAPER_ERR_INVALID_ARGUMENT;
    caps->webgpu_available = 1;
    caps->webgl2_available = 0; /* determined by webgl2 backend */
    caps->backend = PAPER_BACKEND_WEBGPU;
    caps->max_texture_size = PAPER_WEB_MAX_TEXTURE_SIZE;
    (void)b;
    return PAPER_OK;
}

static paper_status_t paper_web_webgpu_resize(
    paper_gpu_backend_t *b, int32_t width, int32_t height) {
    if (!paper_web_size_valid(width, height))
        return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_webgpu_t *w = (paper_web_webgpu_t *)b->priv;
    w->width = width;
    w->height = height;
    return PAPER_OK;
}

static paper_status_t paper_web_webgpu_upload_texture(
    paper_gpu_backend_t *b, const uint8_t *rgba, int32_t width,
    int32_t height, int32_t stride) {
    (void)b;
    (void)rgba;
    (void)width;
    (void)height;
    (void)stride;
    paper_web_webgpu_t *w = (paper_web_webgpu_t *)b->priv;
    if (!w->initialized || !w->device) return PAPER_ERR_INVALID_STATE;
    return PAPER_OK;
}

static paper_status_t paper_web_webgpu_present(paper_gpu_backend_t *b) {
    (void)b;
    return PAPER_OK;
}

static const paper_gpu_backend_vtable_t k_webgpu_vtable = {
    .destroy = paper_web_webgpu_destroy,
    .init = paper_web_webgpu_init,
    .get_caps = paper_web_webgpu_get_caps,
    .resize = paper_web_webgpu_resize,
    .upload_texture = paper_web_webgpu_upload_texture,
    .present = paper_web_webgpu_present,
};

paper_gpu_backend_t *paper_gpu_backend_webgpu_create(void) {
    paper_gpu_backend_t *b =
        (paper_gpu_backend_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    paper_web_webgpu_t *w =
        (paper_web_webgpu_t *)calloc(1, sizeof(*w));
    if (!w) {
        free(b);
        return NULL;
    }
    b->vt = &k_webgpu_vtable;
    b->priv = w;
    return b;
}
#else
/* WebGPU header not available: WebGPU backend reports unavailable. */
typedef struct {
    int placeholder;
} paper_web_webgpu_stub_t;

static void paper_web_webgpu_destroy(paper_gpu_backend_t *b) {
    if (!b) return;
    free(b->priv);
    b->priv = NULL;
}

static paper_status_t paper_web_webgpu_init(paper_gpu_backend_t *b,
                                            void *canvas_element) {
    (void)b;
    (void)canvas_element;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgpu_get_caps(
    paper_gpu_backend_t *b, paper_gpu_caps_t *caps) {
    if (!caps) return PAPER_ERR_INVALID_ARGUMENT;
    caps->webgpu_available = 0;
    caps->webgl2_available = 0;
    caps->backend = PAPER_BACKEND_CPU;
    caps->max_texture_size = PAPER_WEB_MAX_TEXTURE_SIZE;
    (void)b;
    return PAPER_OK;
}

static paper_status_t paper_web_webgpu_resize(
    paper_gpu_backend_t *b, int32_t width, int32_t height) {
    (void)b;
    (void)width;
    (void)height;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgpu_upload_texture(
    paper_gpu_backend_t *b, const uint8_t *rgba, int32_t width,
    int32_t height, int32_t stride) {
    (void)b;
    (void)rgba;
    (void)width;
    (void)height;
    (void)stride;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgpu_present(paper_gpu_backend_t *b) {
    (void)b;
    return PAPER_ERR_NOT_SUPPORTED;
}

static const paper_gpu_backend_vtable_t k_webgpu_vtable = {
    .destroy = paper_web_webgpu_destroy,
    .init = paper_web_webgpu_init,
    .get_caps = paper_web_webgpu_get_caps,
    .resize = paper_web_webgpu_resize,
    .upload_texture = paper_web_webgpu_upload_texture,
    .present = paper_web_webgpu_present,
};

paper_gpu_backend_t *paper_gpu_backend_webgpu_create(void) {
    paper_gpu_backend_t *b =
        (paper_gpu_backend_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    paper_web_webgpu_stub_t *s =
        (paper_web_webgpu_stub_t *)calloc(1, sizeof(*s));
    if (!s) {
        free(b);
        return NULL;
    }
    b->vt = &k_webgpu_vtable;
    b->priv = s;
    return b;
}
#endif /* PAPER_HAVE_WEBGPU_HEADER */

/**
 * @file paper_web.c
 * @brief Web rendering module: auto-detects WebGPU (primary) / WebGL 2.0
 * (fallback) / CPU (always available) and composites the engine's RGBA8888
 * canvas to the browser display target.
 */

#include "paper_web_internals.h"

static const char *paper_web_backend_name_impl(paper_backend_t backend) {
    switch (backend) {
        case PAPER_BACKEND_WEBGPU: return "webgpu";
        case PAPER_BACKEND_WEBGL2: return "webgl2";
        case PAPER_BACKEND_CPU: return "cpu";
        default: return "unknown";
    }
}

const char *paper_web_backend_name(paper_backend_t backend) {
    return paper_web_backend_name_impl(backend);
}

const char *paper_web_version(void) {
    return "paper-web 0.1.0 (Module 01 web)";
}

paper_backend_t paper_web_detect(paper_backend_caps_t *caps) {
    paper_backend_t selected = PAPER_BACKEND_CPU;

    /* Probe WebGPU backend. */
    paper_gpu_backend_t *wgpu = paper_gpu_backend_webgpu_create();
    if (wgpu) {
        paper_gpu_caps_t gc = {0};
        if (wgpu->vt->get_caps(wgpu, &gc) == PAPER_OK && gc.webgpu_available) {
            selected = PAPER_BACKEND_WEBGPU;
            if (caps) {
                caps->webgpu_available = gc.webgpu_available;
                caps->webgl2_available = gc.webgl2_available;
                caps->backend = gc.backend;
                caps->max_texture_size = gc.max_texture_size;
            }
            wgpu->vt->destroy(wgpu);
            free(wgpu);
            return selected;
        }
        wgpu->vt->destroy(wgpu);
        free(wgpu);
    }

    /* Probe WebGL 2.0 backend. */
    paper_gpu_backend_t *gl = paper_gpu_backend_webgl2_create();
    if (gl) {
        paper_gpu_caps_t gc = {0};
        if (gl->vt->get_caps(gl, &gc) == PAPER_OK && gc.webgl2_available) {
            selected = PAPER_BACKEND_WEBGL2;
            if (caps) {
                caps->webgpu_available = gc.webgpu_available;
                caps->webgl2_available = gc.webgl2_available;
                caps->backend = gc.backend;
                caps->max_texture_size = gc.max_texture_size;
            }
            gl->vt->destroy(gl);
            free(gl);
            return selected;
        }
        gl->vt->destroy(gl);
        free(gl);
    }

    /* CPU fallback (always available). */
    paper_gpu_backend_t *cpu = paper_gpu_backend_cpu_create();
    if (cpu) {
        paper_gpu_caps_t gc = {0};
        if (cpu->vt->get_caps(cpu, &gc) == PAPER_OK) {
            selected = PAPER_BACKEND_CPU;
            if (caps) {
                caps->webgpu_available = gc.webgpu_available;
                caps->webgl2_available = gc.webgl2_available;
                caps->backend = gc.backend;
                caps->max_texture_size = gc.max_texture_size;
            }
        }
        cpu->vt->destroy(cpu);
        free(cpu);
    }
    return selected;
}

paper_status_t paper_web_create(paper_engine_t *engine,
                                paper_backend_t preferred,
                                void **out) {
    if (!engine || !out) return PAPER_ERR_INVALID_ARGUMENT;

    /* Resolve output pixel dimensions from physical config (dims x dpi).
     * Do not require engine->configured: the first present() triggers the
     * engine render which allocates the canvas. */
    if (!(engine->cfg.dims.width_mm > 0.0) ||
        !(engine->cfg.dims.height_mm > 0.0) ||
        !(engine->cfg.dpi > 0.0))
        return PAPER_ERR_INVALID_STATE;

    int32_t w = paper_round(paper_mm_to_px(engine->cfg.dims.width_mm,
                                           engine->cfg.dpi));
    int32_t h = paper_round(paper_mm_to_px(engine->cfg.dims.height_mm,
                                           engine->cfg.dpi));
    if (!paper_web_size_valid(w, h)) return PAPER_ERR_INVALID_ARGUMENT;

    paper_web_ctx_t *ctx = (paper_web_ctx_t *)calloc(1, sizeof(*ctx));
    if (!ctx) return PAPER_ERR_OUT_OF_MEMORY;

    ctx->engine = engine;
    ctx->width = w;
    ctx->height = h;
    ctx->configured = 1;

    /* Auto-detect / select backend. */
    paper_backend_t chosen = preferred;
    if (chosen == PAPER_BACKEND_AUTO) {
        chosen = paper_web_detect(NULL);
    } else if (chosen != PAPER_BACKEND_WEBGPU &&
               chosen != PAPER_BACKEND_WEBGL2 &&
               chosen != PAPER_BACKEND_CPU) {
        free(ctx);
        return PAPER_ERR_INVALID_ARGUMENT;
    }

    /* Try preferred backend, fall down the chain. */
    paper_gpu_backend_t *backend = NULL;
    int tried_webgpu = 0, tried_webgl2 = 0;

    if (chosen == PAPER_BACKEND_WEBGPU) {
        backend = paper_gpu_backend_webgpu_create();
        tried_webgpu = 1;
    } else if (chosen == PAPER_BACKEND_WEBGL2) {
        backend = paper_gpu_backend_webgl2_create();
        tried_webgl2 = 1;
    } else {
        backend = paper_gpu_backend_cpu_create();
    }

    if (backend && backend->vt->init(backend, NULL) != PAPER_OK) {
        backend->vt->destroy(backend);
        free(backend);
        backend = NULL;
    }

    /* Fallback chain: WebGPU -> WebGL2 -> CPU. */
    if (!backend && !tried_webgpu && chosen != PAPER_BACKEND_CPU) {
        backend = paper_gpu_backend_webgpu_create();
        tried_webgpu = 1;
        if (backend && backend->vt->init(backend, NULL) != PAPER_OK) {
            backend->vt->destroy(backend);
            free(backend);
            backend = NULL;
        }
    }
    if (!backend && !tried_webgl2 && chosen != PAPER_BACKEND_CPU) {
        backend = paper_gpu_backend_webgl2_create();
        tried_webgl2 = 1;
        if (backend && backend->vt->init(backend, NULL) != PAPER_OK) {
            backend->vt->destroy(backend);
            free(backend);
            backend = NULL;
        }
    }
    if (!backend) {
        backend = paper_gpu_backend_cpu_create();
        if (backend && backend->vt->init(backend, NULL) != PAPER_OK) {
            backend->vt->destroy(backend);
            free(backend);
            free(ctx);
            return PAPER_ERR_NOT_SUPPORTED;
        }
    }

    ctx->backend = *backend;
    {
        paper_gpu_caps_t caps = {0};
        backend->vt->get_caps(backend, &caps);
        ctx->effective = (paper_backend_t)caps.backend;
    }

    /* Resize to engine dimensions. */
    if (backend->vt->resize(backend, ctx->width, ctx->height) != PAPER_OK) {
        backend->vt->destroy(backend);
        free(ctx);
        return PAPER_ERR_INVALID_ARGUMENT;
    }

    /* backend was copied by value into ctx->backend; free the wrapper. */
    free(backend);

    *out = ctx;
    return PAPER_OK;
}

void paper_web_destroy(void *ctx) {
    if (!ctx) return;
    paper_web_ctx_t *c = (paper_web_ctx_t *)ctx;
    if (c->backend.priv || c->backend.vt) {
        c->backend.vt->destroy(&c->backend);
    }
    free(c);
}

paper_status_t paper_web_present(void *ctx) {
    if (!ctx) return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_ctx_t *c = (paper_web_ctx_t *)ctx;
    if (!c->engine || !c->backend.vt) return PAPER_ERR_INVALID_STATE;

    /* Render the engine's frame into an engine-allocated RGBA8888 buffer. */
    paper_pixel_buffer_t buf = {0};
    buf.format = PAPER_PIXEL_RGBA8888;
    paper_status_t st = paper_engine_render(c->engine, &buf);
    if (st != PAPER_OK) return st;

    /* Upload to GPU backend. */
    st = c->backend.vt->upload_texture(&c->backend, buf.data, buf.width,
                                       buf.height, buf.stride);
    if (st != PAPER_OK) {
        paper_buffer_release(&buf);
        return st;
    }

    /* Present to display target. */
    st = c->backend.vt->present(&c->backend);
    paper_buffer_release(&buf);
    if (st != PAPER_OK) return st;

    c->dirty = 0;
    return PAPER_OK;
}

paper_status_t paper_web_resize(void *ctx, int32_t width, int32_t height) {
    if (!ctx) return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_ctx_t *c = (paper_web_ctx_t *)ctx;
    if (!paper_web_size_valid(width, height))
        return PAPER_ERR_INVALID_ARGUMENT;
    c->width = width;
    c->height = height;
    if (c->backend.vt && c->backend.vt->resize)
        return c->backend.vt->resize(&c->backend, width, height);
    return PAPER_OK;
}

paper_backend_t paper_web_get_backend(const void *ctx) {
    if (!ctx) return PAPER_BACKEND_CPU;
    const paper_web_ctx_t *c = (const paper_web_ctx_t *)ctx;
    return c->effective;
}

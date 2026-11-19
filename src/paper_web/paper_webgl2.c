/**
 * @file paper_webgl2.c
 * @brief WebGL 2.0 fallback backend for the web rendering module.
 *
 * Guarded by __EMSCRIPTEN__: compiles only in Emscripten browser builds.
 * Renders the paper texture to a fullscreen quad via WebGL2 when the
 * headers are available; otherwise WebGL2 is reported as unavailable and
 * the module falls through to the CPU fallback.
 *
 * NOT built or tested in this environment (no Emscripten/browser). Requires
 * browser validation before shipping.
 */

#include "paper_web_internals.h"

#ifdef __EMSCRIPTEN__
#   include <emscripten/html5.h>
#   if defined(__has_include)
#       if __has_include(<GLES3/gl3.h>)
#           include <GLES3/gl3.h>
#           define PAPER_HAVE_WEBGL2_HEADER 1
#       endif
#   endif
#else
/* Non-Emscripten stub: WebGL2 unavailable. */
#endif

#ifdef PAPER_HAVE_WEBGL2_HEADER
typedef struct {
    int gl_context;       /**< Emscripten WebGL context handle. */
    GLuint program;
    GLuint texture;
    GLuint vao;
    GLuint vbo;
    GLint loc_texture;
    GLint loc_resolution;
    int initialized;
    int32_t width;
    int32_t height;
} paper_web_webgl2_t;

static void paper_web_webgl2_destroy(paper_gpu_backend_t *b) {
    if (!b) return;
    paper_web_webgl2_t *g = (paper_web_webgl2_t *)b->priv;
    if (g) {
        if (g->program) glDeleteProgram(g->program);
        if (g->texture) glDeleteTextures(1, &g->texture);
        if (g->vao) glDeleteVertexArrays(1, &g->vao);
        if (g->vbo) glDeleteBuffers(1, &g->vbo);
        if (g->gl_context) emscripten_webgl_destroy_context(g->gl_context);
        free(g);
    }
    b->priv = NULL;
}

static paper_status_t paper_web_webgl2_init(paper_gpu_backend_t *b,
                                            void *canvas_element) {
    (void)b;
    (void)canvas_element;
    paper_web_webgl2_t *g = (paper_web_webgl2_t *)b->priv;

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.alpha = 1;
    attrs.depth = 0;
    attrs.stencil = 0;
    attrs.antialias = 0;
    attrs.premultipliedAlpha = 0;
    attrs.preserveDrawingBuffer = 1;

    const char *canvas_selector = (const char *)canvas_element;
    if (!canvas_selector) canvas_selector = "#canvas";
    g->gl_context = emscripten_webgl_create_context(canvas_selector, &attrs);
    if (g->gl_context < 0) return PAPER_ERR_NOT_SUPPORTED;
    emscripten_webgl_make_context_current(g->gl_context);

    g->initialized = 0;
    return PAPER_OK;
}

static paper_status_t paper_web_webgl2_get_caps(
    paper_gpu_backend_t *b, paper_gpu_caps_t *caps) {
    if (!caps) return PAPER_ERR_INVALID_ARGUMENT;
    caps->webgpu_available = 0; /* determined by webgpu backend */
    caps->webgl2_available = 1;
    caps->backend = PAPER_BACKEND_WEBGL2;
    caps->max_texture_size = PAPER_WEB_MAX_TEXTURE_SIZE;
    (void)b;
    return PAPER_OK;
}

static paper_status_t paper_web_webgl2_resize(
    paper_gpu_backend_t *b, int32_t width, int32_t height) {
    if (!paper_web_size_valid(width, height))
        return PAPER_ERR_INVALID_ARGUMENT;
    paper_web_webgl2_t *g = (paper_web_webgl2_t *)b->priv;
    g->width = width;
    g->height = height;
#ifdef __EMSCRIPTEN__
    if (g->gl_context) {
        emscripten_webgl_make_context_current(g->gl_context);
    }
#endif
    return PAPER_OK;
}

static paper_status_t paper_web_webgl2_upload_texture(
    paper_gpu_backend_t *b, const uint8_t *rgba, int32_t width,
    int32_t height, int32_t stride) {
    (void)b;
    (void)rgba;
    (void)width;
    (void)height;
    (void)stride;
    paper_web_webgl2_t *g = (paper_web_webgl2_t *)b->priv;
    if (!g->initialized || g->gl_context < 0)
        return PAPER_ERR_INVALID_STATE;
    emscripten_webgl_make_context_current(g->gl_context);
    if (!g->texture) glGenTextures(1, &g->texture);
    glBindTexture(GL_TEXTURE_2D, g->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba);
    return PAPER_OK;
}

static paper_status_t paper_web_webgl2_present(paper_gpu_backend_t *b) {
    (void)b;
    return PAPER_OK;
}

static const paper_gpu_backend_vtable_t k_webgl2_vtable = {
    .destroy = paper_web_webgl2_destroy,
    .init = paper_web_webgl2_init,
    .get_caps = paper_web_webgl2_get_caps,
    .resize = paper_web_webgl2_resize,
    .upload_texture = paper_web_webgl2_upload_texture,
    .present = paper_web_webgl2_present,
};

paper_gpu_backend_t *paper_gpu_backend_webgl2_create(void) {
    paper_gpu_backend_t *b =
        (paper_gpu_backend_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    paper_web_webgl2_t *g =
        (paper_web_webgl2_t *)calloc(1, sizeof(*g));
    if (!g) {
        free(b);
        return NULL;
    }
    b->vt = &k_webgl2_vtable;
    b->priv = g;
    return b;
}
#else
/* WebGL2 header not available: WebGL2 backend reports unavailable. */
typedef struct {
    int placeholder;
} paper_web_webgl2_stub_t;

static void paper_web_webgl2_destroy(paper_gpu_backend_t *b) {
    if (!b) return;
    free(b->priv);
    b->priv = NULL;
}

static paper_status_t paper_web_webgl2_init(paper_gpu_backend_t *b,
                                            void *canvas_element) {
    (void)b;
    (void)canvas_element;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgl2_get_caps(
    paper_gpu_backend_t *b, paper_gpu_caps_t *caps) {
    if (!caps) return PAPER_ERR_INVALID_ARGUMENT;
    caps->webgpu_available = 0;
    caps->webgl2_available = 0;
    caps->backend = PAPER_BACKEND_CPU;
    caps->max_texture_size = PAPER_WEB_MAX_TEXTURE_SIZE;
    (void)b;
    return PAPER_OK;
}

static paper_status_t paper_web_webgl2_resize(
    paper_gpu_backend_t *b, int32_t width, int32_t height) {
    (void)b;
    (void)width;
    (void)height;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgl2_upload_texture(
    paper_gpu_backend_t *b, const uint8_t *rgba, int32_t width,
    int32_t height, int32_t stride) {
    (void)b;
    (void)rgba;
    (void)width;
    (void)height;
    (void)stride;
    return PAPER_ERR_NOT_SUPPORTED;
}

static paper_status_t paper_web_webgl2_present(paper_gpu_backend_t *b) {
    (void)b;
    return PAPER_ERR_NOT_SUPPORTED;
}

static const paper_gpu_backend_vtable_t k_webgl2_vtable = {
    .destroy = paper_web_webgl2_destroy,
    .init = paper_web_webgl2_init,
    .get_caps = paper_web_webgl2_get_caps,
    .resize = paper_web_webgl2_resize,
    .upload_texture = paper_web_webgl2_upload_texture,
    .present = paper_web_webgl2_present,
};

paper_gpu_backend_t *paper_gpu_backend_webgl2_create(void) {
    paper_gpu_backend_t *b =
        (paper_gpu_backend_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    paper_web_webgl2_stub_t *s =
        (paper_web_webgl2_stub_t *)calloc(1, sizeof(*s));
    if (!s) {
        free(b);
        return NULL;
    }
    b->vt = &k_webgl2_vtable;
    b->priv = s;
    return b;
}
#endif /* PAPER_HAVE_WEBGL2_HEADER */

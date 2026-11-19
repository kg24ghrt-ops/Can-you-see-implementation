#ifndef PAPER_WEB_H
#define PAPER_WEB_H

/**
 * @file paper_web.h
 * @brief Web rendering module for the Paper Rendering Engine.
 *
 * Provides GPU-accelerated rendering for WebAssembly builds with automatic
 * backend selection: WebGPU (primary) -> WebGL 2.0 (fallback) -> CPU
 * software path (always available). The public ABI is plain C and is
 * ABI-compatible across native, Android .so, and WASM builds.
 *
 * All WebGPU/WebGL 2.0 code is guarded by __EMSCRIPTEN__ and only compiles
 * in Emscripten browser builds. On non-Emscripten targets the module falls
 * back to the CPU renderer, so it compiles and tests cleanly everywhere.
 */

#include "paper/paper.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * GPU backend selection
 * =========================================================================== */

/**
 * @brief Available rendering backends.
 */
typedef enum {
    PAPER_BACKEND_AUTO = 0,     /**< Auto-detect best available backend. */
    PAPER_BACKEND_WEBGPU = 1,   /**< WebGPU (primary). */
    PAPER_BACKEND_WEBGL2 = 2,   /**< WebGL 2.0 (fallback). */
    PAPER_BACKEND_CPU = 3,      /**< CPU software path (always available). */
} paper_backend_t;

/**
 * @brief Detected backend capabilities.
 */
typedef struct {
    int webgpu_available;       /**< 1 if WebGPU is available. */
    int webgl2_available;       /**< 1 if WebGL 2.0 is available. */
    int backend;                /**< Selected backend (one of PAPER_BACKEND_*). */
    int max_texture_size;       /**< Max GPU texture size (pixels, 0 = unknown). */
} paper_backend_caps_t;

/* ===========================================================================
 * Web module lifecycle
 * =========================================================================== */

/**
 * @brief Create a web rendering context tied to an existing paper_engine_t.
 * @param engine The paper engine (must already be configured).
 * @param preferred Backend preference (PAPER_BACKEND_AUTO picks best).
 * @param out Receives the opaque web context handle.
 * @return PAPER_OK on success, PAPER_ERR_NOT_SUPPORTED if no backend works,
 *         PAPER_ERR_OUT_OF_MEMORY on allocation failure.
 * Ownership: caller owns the returned handle; destroy with paper_web_destroy.
 */
paper_status_t paper_web_create(paper_engine_t *engine,
                                paper_backend_t preferred,
                                void **out);

/**
 * @brief Destroy a web context and release GPU resources.
 * Safe to call with NULL.
 */
void paper_web_destroy(void *ctx);

/**
 * @brief Detect available backends and select one.
 * @param caps Receives capability info. May be NULL.
 * @return Selected backend id.
 */
paper_backend_t paper_web_detect(paper_backend_caps_t *caps);

/* ===========================================================================
 * Rendering
 * =========================================================================== */

/**
 * @brief Render the engine's current frame to the web canvas / GPU target.
 * The engine must have been rendered via paper_engine_render() first; this
 * call presents/composites the resulting pixel buffer to the display target.
 *
 * @param ctx Web context from paper_web_create().
 * @return PAPER_OK on success.
 */
paper_status_t paper_web_present(void *ctx);

/**
 * @brief Resize the web rendering target (canvas / offscreen buffer).
 * @param width,height New size in pixels. Must be > 0 and within
 *                     paper_backend_caps_t.max_texture_size when known.
 * @return PAPER_OK on success, PAPER_ERR_INVALID_ARGUMENT for bad size.
 */
paper_status_t paper_web_resize(void *ctx, int32_t width, int32_t height);

/**
 * @brief Get the selected backend for this context.
 */
paper_backend_t paper_web_get_backend(const void *ctx);

/**
 * @brief Return a static string naming the backend, e.g. "webgpu", "webgl2",
 * or "cpu".
 */
const char *paper_web_backend_name(paper_backend_t backend);

/* ===========================================================================
 * Version
 * =========================================================================== */

const char *paper_web_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PAPER_WEB_H */

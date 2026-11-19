#ifndef PAPER_GPU_BACKEND_H
#define PAPER_GPU_BACKEND_H

/**
 * @file paper_gpu_backend.h
 * @brief GPU backend interface for the web rendering module.
 *
 * Backends (WebGPU, WebGL2, CPU) implement this interface. The web module
 * holds a pointer to a backend vtable and dispatches through it, keeping
 * the core engine free of platform/GPU code.
 */

#include "paper/paper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct paper_gpu_backend paper_gpu_backend_t;

/**
 * @brief Backend capability query.
 */
typedef struct {
    int webgpu_available;
    int webgl2_available;
    int backend;
    int max_texture_size;
} paper_gpu_caps_t;

/**
 * @brief Backend vtable. All functions receive the backend's private state.
 */
typedef struct {
    void (*destroy)(paper_gpu_backend_t *b);
    paper_status_t (*init)(paper_gpu_backend_t *b, void *canvas_element);
    paper_status_t (*get_caps)(paper_gpu_backend_t *b, paper_gpu_caps_t *caps);
    paper_status_t (*resize)(paper_gpu_backend_t *b, int32_t width,
                             int32_t height);
    /** Upload an RGBA8888 pixel buffer to a GPU texture. */
    paper_status_t (*upload_texture)(paper_gpu_backend_t *b,
                                     const uint8_t *rgba,
                                     int32_t width, int32_t height,
                                     int32_t stride);
    /** Present the uploaded texture to the display target. */
    paper_status_t (*present)(paper_gpu_backend_t *b);
} paper_gpu_backend_vtable_t;

struct paper_gpu_backend {
    const paper_gpu_backend_vtable_t *vt;
    void *priv;
};

/**
 * @brief Backend constructor signatures (implemented per backend).
 */
paper_gpu_backend_t *paper_gpu_backend_webgpu_create(void);
paper_gpu_backend_t *paper_gpu_backend_webgl2_create(void);
paper_gpu_backend_t *paper_gpu_backend_cpu_create(void);

#ifdef __cplusplus
}
#endif

#endif /* PAPER_GPU_BACKEND_H */

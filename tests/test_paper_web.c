#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "paper_web/paper_web.h"

int main(void) {
    /* Backend name strings. */
    assert(strcmp(paper_web_backend_name(PAPER_BACKEND_WEBGPU), "webgpu") == 0);
    assert(strcmp(paper_web_backend_name(PAPER_BACKEND_WEBGL2), "webgl2") == 0);
    assert(strcmp(paper_web_backend_name(PAPER_BACKEND_CPU), "cpu") == 0);
    assert(strcmp(paper_web_backend_name(PAPER_BACKEND_AUTO), "unknown") == 0);
    assert(strcmp(paper_web_backend_name((paper_backend_t)99), "unknown") == 0);

    /* Version string non-empty. */
    assert(paper_web_version() != NULL && paper_web_version()[0] != '\0');

    /* Create a paper engine, small canvas (size-limited test). */
    paper_engine_t *engine = NULL;
    assert(paper_engine_create(&engine) == PAPER_OK);
    assert(paper_engine_set_dpi(engine, 72.0) == PAPER_OK);
    assert(paper_engine_set_seed(engine, 7u) == PAPER_OK);
    assert(paper_engine_set_layout(engine, PAPER_LAYOUT_BLANK, NULL) == PAPER_OK);
    /* 256x256 px canvas -> well within size limits. */
    assert(paper_engine_set_dimensions(engine, PAPER_SIZE_CUSTOM,
                                       &(paper_dimensions_t){90.0, 90.0},
                                       PAPER_PORTRAIT) == PAPER_OK);

    /* Auto-detect: on non-Emscripten this resolves to CPU. */
    paper_backend_caps_t caps = {0};
    paper_backend_t b = paper_web_detect(&caps);
    assert(b == PAPER_BACKEND_CPU || b == PAPER_BACKEND_WEBGPU ||
           b == PAPER_BACKEND_WEBGL2);
    assert(caps.max_texture_size > 0);

    /* Create web context with explicit CPU backend (always available). */
    void *ctx = NULL;
    assert(paper_web_create(engine, PAPER_BACKEND_CPU, &ctx) == PAPER_OK);
    assert(ctx != NULL);
    assert(paper_web_get_backend(ctx) == PAPER_BACKEND_CPU);

    /* Present: renders + uploads + presents. */
    assert(paper_web_present(ctx) == PAPER_OK);

    /* Resize to valid small size. */
    assert(paper_web_resize(ctx, 128, 128) == PAPER_OK);
    assert(paper_web_present(ctx) == PAPER_OK);

    /* Invalid sizes rejected. */
    assert(paper_web_resize(ctx, 0, 128) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_resize(ctx, 128, 0) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_resize(ctx, -1, 128) == PAPER_ERR_INVALID_ARGUMENT);
    /* Exceeds max texture size. */
    assert(paper_web_resize(ctx, 9000, 9000) == PAPER_ERR_INVALID_ARGUMENT);

    /* Auto backend also resolves to something usable. */
    void *ctx2 = NULL;
    assert(paper_web_create(engine, PAPER_BACKEND_AUTO, &ctx2) == PAPER_OK);
    assert(ctx2 != NULL);
    assert(paper_web_present(ctx2) == PAPER_OK);
    paper_web_destroy(ctx2);

    /* Explicit WebGPU/WebGL2 create returns NULL on non-Emscripten
     * (backends unavailable), but web_create with them should still
     * fall back to CPU rather than hard-failing. */
    void *ctx3 = NULL;
    assert(paper_web_create(engine, PAPER_BACKEND_WEBGPU, &ctx3) == PAPER_OK);
    assert(ctx3 != NULL);
    assert(paper_web_get_backend(ctx3) == PAPER_BACKEND_CPU);
    paper_web_destroy(ctx3);

    void *ctx4 = NULL;
    assert(paper_web_create(engine, PAPER_BACKEND_WEBGL2, &ctx4) == PAPER_OK);
    assert(ctx4 != NULL);
    assert(paper_web_get_backend(ctx4) == PAPER_BACKEND_CPU);
    paper_web_destroy(ctx4);

    /* Invalid args. */
    assert(paper_web_create(NULL, PAPER_BACKEND_CPU, &ctx) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_create(engine, PAPER_BACKEND_CPU, NULL) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_create(engine, (paper_backend_t)99, &ctx) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_present(NULL) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_web_resize(NULL, 128, 128) == PAPER_ERR_INVALID_ARGUMENT);

    paper_web_destroy(ctx);
    paper_web_destroy(NULL); /* safe */
    paper_engine_destroy(engine);

    printf("paper_web OK (backend=%s)\n", paper_web_backend_name(b));
    return 0;
}

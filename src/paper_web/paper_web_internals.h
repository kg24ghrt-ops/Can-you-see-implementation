#ifndef PAPER_WEB_INTERNALS_H
#define PAPER_WEB_INTERNALS_H

#include "paper_web.h"
#include "paper_gpu_backend.h"
#include "paper/paper_internals.h"

/* ------------------------------------------------------------------ */
/* Web context                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    paper_engine_t *engine;        /**< Not owned; borrowed from caller. */
    paper_gpu_backend_t backend;   /**< Active GPU backend (or NULL => CPU). */
    paper_backend_t effective;     /**< Resolved backend id. */
    int32_t width;
    int32_t height;
    int configured;
    int dirty;                     /**< Re-present required. */
} paper_web_ctx_t;

/* ------------------------------------------------------------------ */
/* Size limits (keep test/render sizes sane on mobile/web)           */
/* ------------------------------------------------------------------ */

#define PAPER_WEB_MAX_TEXTURE_SIZE 4096   /**< Conservative upper bound. */
#define PAPER_WEB_TEST_MAX_SIZE    512    /**< Test canvas size cap. */

/** Validate a canvas size against limits. */
static inline int paper_web_size_valid(int32_t w, int32_t h) {
    return w > 0 && h > 0 &&
           w <= PAPER_WEB_MAX_TEXTURE_SIZE &&
           h <= PAPER_WEB_MAX_TEXTURE_SIZE;
}

#endif /* PAPER_WEB_INTERNALS_H */

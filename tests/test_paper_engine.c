#include <assert.h>
#include <stdio.h>
#include "paper/paper.h"

int main(void) {
    paper_engine_t *e = NULL;
    assert(paper_engine_create(&e) == PAPER_OK);
    assert(e != NULL);
    assert(paper_engine_create(NULL) == PAPER_ERR_INVALID_ARGUMENT);

    /* Defaults: A4 portrait, 150 DPI. */
    int w = -1, h = -1;
    /* Not configured for query until a render prepares the canvas. */
    assert(paper_engine_get_output_dimensions(e, &w, &h) == PAPER_ERR_INVALID_STATE);

    assert(paper_engine_set_dpi(e, 150.0) == PAPER_OK);
    assert(paper_engine_set_dpi(e, 0.0) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_engine_set_dpi(e, -10.0) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_engine_set_seed(e, 42u) == PAPER_OK);

    paper_pixel_buffer_t buf = {0};
    buf.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e, &buf) == PAPER_OK);
    assert(buf.data != NULL);
    assert(buf.width > 0 && buf.height > 0);
    assert(buf.stride == buf.width * 4);
    assert(buf.data_size == (size_t)buf.height * (size_t)buf.stride);

    /* A4 @ 150 dpi portrait = 1240 x 1754 px. */
    assert(paper_engine_get_output_dimensions(e, &w, &h) == PAPER_OK);
    assert(w == 1240 && h == 1754);
    assert(w == buf.width && h == buf.height);

    /* Landscape swaps dimensions. */
    assert(paper_engine_set_dimensions(e, PAPER_SIZE_A4, NULL, PAPER_LANDSCAPE) == PAPER_OK);
    paper_buffer_release(&buf);
    buf = (paper_pixel_buffer_t){0};
    buf.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e, &buf) == PAPER_OK);
    assert(paper_engine_get_output_dimensions(e, &w, &h) == PAPER_OK);
    assert(w == 1754 && h == 1240);

    /* Invalid pixel format. */
    buf.format = (paper_pixel_format_t)99;
    assert(paper_engine_render(e, &buf) == PAPER_ERR_INVALID_ARGUMENT);
    buf.format = PAPER_PIXEL_RGBA8888;

    paper_buffer_release(&buf);
    assert(buf.data == NULL && buf.data_size == 0);
    paper_buffer_release(NULL); /* safe */

    paper_engine_destroy(e);
    paper_engine_destroy(NULL); /* safe */
    printf("paper_engine OK\n");
    return 0;
}

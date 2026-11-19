#include <assert.h>
#include <stdio.h>
#include "paper/paper.h"

int main(void) {
    paper_engine_t *e = NULL;
    assert(paper_engine_create(&e) == PAPER_OK);
    assert(paper_engine_set_dpi(e, 72.0) == PAPER_OK);
    assert(paper_engine_set_seed(e, 2024u) == PAPER_OK);

    paper_pixel_buffer_t a = {0}, b = {0};
    a.format = PAPER_PIXEL_RGBA8888;
    b.format = PAPER_PIXEL_RGBA8888;

    /* Same engine, same settings, two renders => identical bytes (no RNG drift). */
    assert(paper_engine_render(e, &a) == PAPER_OK);
    assert(paper_engine_render(e, &b) == PAPER_OK);
    assert(a.data_size == b.data_size);
    assert(memcmp(a.data, b.data, a.data_size) == 0);

    /* Resetting to the same seed reproduces the same output. */
    assert(paper_engine_set_seed(e, 2024u) == PAPER_OK);
    assert(paper_engine_render(e, &b) == PAPER_OK);
    assert(memcmp(a.data, b.data, a.data_size) == 0);

    paper_buffer_release(&a);
    paper_buffer_release(&b);
    paper_engine_destroy(e);
    printf("paper_rng OK\n");
    return 0;
}

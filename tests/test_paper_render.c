#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "paper/paper.h"

int main(void) {
    paper_engine_t *e1 = NULL, *e2 = NULL;
    assert(paper_engine_create(&e1) == PAPER_OK);
    assert(paper_engine_create(&e2) == PAPER_OK);

    paper_engine_set_dpi(e1, 150.0);
    paper_engine_set_dpi(e2, 150.0);
    paper_engine_set_seed(e1, 1234u);
    paper_engine_set_seed(e2, 1234u);

    /* Same settings + same seed => byte-identical output (determinism). */
    paper_pixel_buffer_t a = {0}, b = {0};
    a.format = PAPER_PIXEL_RGBA8888;
    b.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e1, &a) == PAPER_OK);
    assert(paper_engine_render(e2, &b) == PAPER_OK);
    assert(a.width == b.width && a.height == b.height);
    assert(a.data_size == b.data_size);
    assert(memcmp(a.data, b.data, a.data_size) == 0);

    /* Different seed => different output. */
    paper_engine_set_seed(e2, 999u);
    assert(paper_engine_render(e2, &b) == PAPER_OK);
    assert(memcmp(a.data, b.data, a.data_size) != 0);

    /* RGB888 output is smaller (no alpha) and matches RGB channels. */
    paper_pixel_buffer_t rgb = {0};
    rgb.format = PAPER_PIXEL_RGB888;
    assert(paper_engine_render(e1, &rgb) == PAPER_OK);
    assert(rgb.stride == rgb.width * 3);
    assert(rgb.data_size == (size_t)rgb.height * rgb.stride);
    int x, y;
    for (y = 0; y < a.height; y++) {
        for (x = 0; x < a.width; x++) {
            size_t sa = (size_t)y * a.stride + (size_t)x * 4;
            size_t sr = (size_t)y * rgb.stride + (size_t)x * 3;
            assert(a.data[sa + 0] == rgb.data[sr + 0]);
            assert(a.data[sa + 1] == rgb.data[sr + 1]);
            assert(a.data[sa + 2] == rgb.data[sr + 2]);
        }
    }

    /* Adding a stroke changes the rendered output. */
    paper_engine_clear_strokes(e1);
    paper_stroke_point_t pts[2] = {
        {100.0, 100.0, 0.5, 1.0, 0.0, 0.0, 0.0},
        {200.0, 200.0, 0.5, 1.0, 0.0, 0.0, 0.0},
    };
    paper_stroke_t s = {pts, 2, PAPER_INK_BALLPOINT, 0.05, 0.05, 0.05, 0.9};
    assert(paper_engine_add_stroke(e1, &s) == PAPER_OK);
    paper_pixel_buffer_t c = {0};
    c.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e1, &c) == PAPER_OK);
    assert(memcmp(a.data, c.data, a.data_size) != 0);

    paper_buffer_release(&a);
    paper_buffer_release(&b);
    paper_buffer_release(&c);
    paper_buffer_release(&rgb);
    paper_engine_destroy(e1);
    paper_engine_destroy(e2);
    printf("paper_render OK\n");
    return 0;
}

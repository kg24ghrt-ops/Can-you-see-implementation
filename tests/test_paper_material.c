#include <assert.h>
#include <stdio.h>
#include "paper/paper.h"

int main(void) {
    paper_engine_t *e = NULL;
    assert(paper_engine_create(&e) == PAPER_OK);
    assert(paper_engine_set_dpi(e, 150.0) == PAPER_OK);
    assert(paper_engine_set_seed(e, 5u) == PAPER_OK);

    paper_pixel_buffer_t a = {0}, b = {0};
    a.format = PAPER_PIXEL_RGBA8888;
    b.format = PAPER_PIXEL_RGBA8888;

    /* Natural white material. */
    assert(paper_engine_set_material(e, PAPER_MATERIAL_NATURAL_WHITE, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &a) == PAPER_OK);

    /* Aged material should differ. */
    assert(paper_engine_set_material(e, PAPER_MATERIAL_AGED, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &b) == PAPER_OK);
    assert(a.data_size == b.data_size);
    assert(memcmp(a.data, b.data, a.data_size) != 0);

    /* Pixels should not be flat: some variation expected. */
    int varied = 0;
    uint8_t first_r = a.data[0], first_g = a.data[1], first_b = a.data[2];
    size_t i;
    for (i = 4; i < a.data_size; i += 4) {
        if (a.data[i + 0] != first_r || a.data[i + 1] != first_g || a.data[i + 2] != first_b) {
            varied = 1;
            break;
        }
    }
    assert(varied);

    /* Alpha channel fully opaque. */
    for (i = 3; i < a.data_size; i += 4) assert(a.data[i] == 255);

    /* Custom material validation. */
    paper_material_params_t cp = {0.9, 0.8, 0.7, 0.1, 0.1, 0.1, 0.1};
    assert(paper_engine_set_material(e, PAPER_MATERIAL_CUSTOM, &cp) == PAPER_OK);
    assert(paper_engine_set_material(e, PAPER_MATERIAL_CUSTOM, NULL) == PAPER_ERR_INVALID_ARGUMENT);

    paper_buffer_release(&a);
    paper_buffer_release(&b);
    paper_engine_destroy(e);
    printf("paper_material OK\n");
    return 0;
}

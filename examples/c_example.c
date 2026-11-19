/* examples/c_example.c
 *
 * C usage example for the Paper Rendering Engine (Module 01).
 *
 * Build: compile this file with every source in src/paper/ and link the math
 * library. Then run ./c_example. It renders an A4 warm-white, college-ruled
 * page at 150 DPI with a single ballpoint stroke and writes the RGBA8888
 * pixels to stdout as raw binary.
 */

#include "paper/paper.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    paper_engine_t *engine = NULL;
    if (paper_engine_create(&engine) != PAPER_OK) {
        fprintf(stderr, "failed to create engine\n");
        return 1;
    }

    /* Configure: A4 portrait, 150 dpi, warm white, college-ruled, seed 42. */
    paper_engine_set_dpi(engine, 150.0);
    paper_engine_set_seed(engine, 42u);
    paper_engine_set_material(engine, PAPER_MATERIAL_WARM_WHITE, NULL);
    paper_engine_set_layout(engine, PAPER_LAYOUT_COLLEGE_RULED, NULL);

    /* Add one handwriting stroke (ballpoint, mm on the page). */
    paper_stroke_point_t pts[] = {
        {60.0, 120.0, 0.5, 0.9, 0.0, 0.0, 0.0},
        {110.0, 150.0, 0.6, 1.0, 0.0, 0.0, 0.0},
        {170.0, 130.0, 0.5, 0.8, 0.0, 0.0, 0.0},
    };
    paper_stroke_t stroke = {
        pts, 3, PAPER_INK_BALLPOINT, 0.06, 0.06, 0.06, 0.9,
    };
    paper_engine_add_stroke(engine, &stroke);

    /* Render into an engine-allocated RGBA8888 buffer. */
    paper_pixel_buffer_t buf = {0};
    buf.format = PAPER_PIXEL_RGBA8888;
    if (paper_engine_render(engine, &buf) != PAPER_OK) {
        fprintf(stderr, "render failed\n");
        paper_engine_destroy(engine);
        return 1;
    }

    /* Query dimensions. */
    int w = 0, h = 0;
    paper_engine_get_output_dimensions(engine, &w, &h);
    fprintf(stderr, "rendered %d x %d px, stride %d, %zu bytes\n",
            w, h, buf.stride, buf.data_size);

    /* Write raw RGBA8888 to stdout. */
    fwrite(buf.data, 1, buf.data_size, stdout);

    /* Cleanup. buf was allocated by the engine. */
    paper_buffer_release(&buf);
    paper_engine_destroy(engine);
    return 0;
}

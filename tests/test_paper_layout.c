#include <assert.h>
#include <stdio.h>
#include "paper/paper.h"

int main(void) {
    paper_engine_t *e = NULL;
    assert(paper_engine_create(&e) == PAPER_OK);
    assert(paper_engine_set_dpi(e, 150.0) == PAPER_OK);
    assert(paper_engine_set_seed(e, 3u) == PAPER_OK);

    paper_pixel_buffer_t blank = {0}, ruled = {0}, college = {0}, wide = {0};
    blank.format = ruled.format = college.format = wide.format = PAPER_PIXEL_RGBA8888;

    assert(paper_engine_set_layout(e, PAPER_LAYOUT_BLANK, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &blank) == PAPER_OK);

    assert(paper_engine_set_layout(e, PAPER_LAYOUT_RULED, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &ruled) == PAPER_OK);
    assert(blank.data_size == ruled.data_size);
    assert(memcmp(blank.data, ruled.data, blank.data_size) != 0);

    assert(paper_engine_set_layout(e, PAPER_LAYOUT_COLLEGE_RULED, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &college) == PAPER_OK);
    assert(memcmp(ruled.data, college.data, ruled.data_size) != 0); /* different spacing */

    assert(paper_engine_set_layout(e, PAPER_LAYOUT_WIDE_RULED, NULL) == PAPER_OK);
    assert(paper_engine_render(e, &wide) == PAPER_OK);
    assert(memcmp(college.data, wide.data, college.data_size) != 0);

    /* Graph and dot-grid both produce 2D patterns; they differ from ruled. */
    assert(paper_engine_set_layout(e, PAPER_LAYOUT_GRAPH, NULL) == PAPER_OK);
    paper_pixel_buffer_t graph = {0};
    graph.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e, &graph) == PAPER_OK);
    assert(memcmp(ruled.data, graph.data, ruled.data_size) != 0);

    assert(paper_engine_set_layout(e, PAPER_LAYOUT_DOT_GRID, NULL) == PAPER_OK);
    paper_pixel_buffer_t dots = {0};
    dots.format = PAPER_PIXEL_RGBA8888;
    assert(paper_engine_render(e, &dots) == PAPER_OK);
    assert(memcmp(graph.data, dots.data, graph.data_size) != 0);

    paper_buffer_release(&blank);
    paper_buffer_release(&ruled);
    paper_buffer_release(&college);
    paper_buffer_release(&wide);
    paper_buffer_release(&graph);
    paper_buffer_release(&dots);
    paper_engine_destroy(e);
    printf("paper_layout OK\n");
    return 0;
}

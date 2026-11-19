#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "paper/paper.h"

int main(void) {
    paper_pixel_buffer_t buf = {0};
    assert(paper_buffer_allocate(&buf, PAPER_PIXEL_RGBA8888, 10, 10, 0) == PAPER_OK);
    assert(buf.width == 10 && buf.height == 10);
    assert(buf.stride == 10 * 4);
    assert(buf.data != NULL);
    assert(buf.data_size == (size_t)10 * buf.stride);
    /* Zero-initialised. */
    size_t i;
    for (i = 0; i < buf.data_size; i++) assert(buf.data[i] == 0);

    /* Explicit stride (must be >= width*bpp). */
    paper_pixel_buffer_t buf2 = {0};
    assert(paper_buffer_allocate(&buf2, PAPER_PIXEL_RGBA8888, 10, 10, 64) == PAPER_OK);
    assert(buf2.stride == 64);

    paper_pixel_buffer_t bad = {0};
    assert(paper_buffer_allocate(&bad, PAPER_PIXEL_RGBA8888, 10, 10, 10) == PAPER_ERR_INVALID_ARGUMENT); /* stride too small */
    assert(paper_buffer_allocate(&bad, (paper_pixel_format_t)7, 10, 10, 0) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_buffer_allocate(&bad, PAPER_PIXEL_RGBA8888, 0, 10, 0) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_buffer_allocate(NULL, PAPER_PIXEL_RGBA8888, 10, 10, 0) == PAPER_ERR_INVALID_ARGUMENT);

    /* Write / read a pixel. */
    buf.data[0] = 10; buf.data[1] = 20; buf.data[2] = 30; buf.data[3] = 255;
    assert(buf.data[0] == 10 && buf.data[3] == 255);

    /* Copy roundtrip. */
    paper_pixel_buffer_t dst = {0};
    assert(paper_buffer_allocate(&dst, PAPER_PIXEL_RGBA8888, 10, 10, 0) == PAPER_OK);
    assert(paper_buffer_copy(&dst, &buf, 10, 10) == PAPER_OK);
    assert(memcmp(dst.data, buf.data, dst.data_size) == 0);
    assert(paper_buffer_copy(&dst, &buf, 11, 10) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_buffer_copy(NULL, &buf, 10, 10) == PAPER_ERR_INVALID_ARGUMENT);

    paper_buffer_release(&buf);
    assert(buf.data == NULL && buf.data_size == 0);
    paper_buffer_release(&buf2);
    paper_buffer_release(&dst);
    paper_buffer_release(NULL);
    printf("paper_buffer OK\n");
    return 0;
}

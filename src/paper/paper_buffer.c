/**
 * @file paper_buffer.c
 * @brief Pixel buffer abstraction: RGBA8888 / RGB888, explicit stride,
 * safe allocation and destruction.
 */

#include "paper_internals.h"

size_t paper_format_bytes(paper_pixel_format_t fmt) {
    switch (fmt) {
        case PAPER_PIXEL_RGB888: return 3;
        case PAPER_PIXEL_RGBA8888: return 4;
        default: return 0;
    }
}

paper_status_t paper_buffer_allocate(paper_pixel_buffer_t *buf,
                                     paper_pixel_format_t fmt,
                                     int32_t width, int32_t height,
                                     int32_t stride) {
    if (buf == NULL || width <= 0 || height <= 0) return PAPER_ERR_INVALID_ARGUMENT;
    size_t bpp = paper_format_bytes(fmt);
    if (bpp == 0) return PAPER_ERR_INVALID_ARGUMENT;
    if (stride == 0) stride = width * (int32_t)bpp;
    if (stride < width * (int32_t)bpp) return PAPER_ERR_INVALID_ARGUMENT;
    /* Overflow guard. */
    if ((size_t)height > (size_t)(-1) / (size_t)stride) return PAPER_ERR_OUT_OF_MEMORY;
    size_t size = (size_t)height * (size_t)stride;
    uint8_t *data = (uint8_t *)calloc(1, size);
    if (data == NULL) return PAPER_ERR_OUT_OF_MEMORY;
    buf->format = fmt;
    buf->width = width;
    buf->height = height;
    buf->stride = stride;
    buf->data = data;
    buf->data_size = size;
    return PAPER_OK;
}

void paper_buffer_release(paper_pixel_buffer_t *buf) {
    if (buf == NULL) return;
    if (buf->data != NULL) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->format = PAPER_PIXEL_RGBA8888;
    buf->width = 0;
    buf->height = 0;
    buf->stride = 0;
    buf->data_size = 0;
}

/** Copy rectangle [0,w)x[0,h) from src to dst respecting each stride. */
paper_status_t paper_buffer_copy(const paper_pixel_buffer_t *dst,
                                 const paper_pixel_buffer_t *src,
                                 int32_t w, int32_t h) {
    if (dst == NULL || src == NULL || dst->data == NULL || src->data == NULL)
        return PAPER_ERR_INVALID_ARGUMENT;
    if (w > dst->width || w > src->width || h > dst->height || h > src->height)
        return PAPER_ERR_INVALID_ARGUMENT;
    size_t bpp = paper_format_bytes(dst->format);
    size_t sbpp = paper_format_bytes(src->format);
    if (bpp == 0 || sbpp == 0 || bpp != sbpp) return PAPER_ERR_INVALID_ARGUMENT;
    int32_t row_bytes = w * (int32_t)bpp;
    int y;
    for (y = 0; y < h; y++) {
        memcpy(dst->data + (size_t)y * dst->stride,
               src->data + (size_t)y * src->stride,
               (size_t)row_bytes);
    }
    return PAPER_OK;
}

/**
 * @file paper_engine.c
 * @brief Engine state management and public C API entry points.
 */

#include "paper_internals.h"

paper_status_t paper_engine_create(paper_engine_t **out) {
    if (out == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    paper_engine_t *e = (paper_engine_t *)calloc(1, sizeof(*e));
    if (e == NULL) return PAPER_ERR_OUT_OF_MEMORY;

    /* Sensible defaults: A4 portrait, 150 DPI, natural white, blank, RGBA. */
    e->cfg.size_id = PAPER_SIZE_A4;
    e->cfg.dims.width_mm = 210.0;
    e->cfg.dims.height_mm = 297.0;
    e->cfg.orientation = PAPER_PORTRAIT;
    e->cfg.dpi = 150.0;
    e->cfg.seed = 0x12345678u;
    e->cfg.material = PAPER_MATERIAL_NATURAL_WHITE;
    memset(&e->cfg.material_params, 0, sizeof(e->cfg.material_params));
    e->cfg.layout = PAPER_LAYOUT_BLANK;
    memset(&e->cfg.layout_params, 0, sizeof(e->cfg.layout_params));
    e->cfg.pixel_format = PAPER_PIXEL_RGBA8888;
    e->dirty = 1;

    *out = e;
    return PAPER_OK;
}

void paper_engine_destroy(paper_engine_t *engine) {
    if (engine == NULL) return;
    if (engine->strokes != NULL) {
        size_t i;
        for (i = 0; i < engine->stroke_count; i++) {
            free(engine->strokes[i].points);
        }
        free(engine->strokes);
    }
    if (engine->pixels != NULL) free(engine->pixels);
    free(engine);
}

paper_status_t paper_engine_set_material(paper_engine_t *engine,
                                         paper_material_id_t material,
                                         const paper_material_params_t *params) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (material < 0 || material >= PAPER_MATERIAL_CUSTOM + 1) return PAPER_ERR_INVALID_ARGUMENT;
    if (material == PAPER_MATERIAL_CUSTOM && params == NULL)
        return PAPER_ERR_INVALID_ARGUMENT;
    engine->cfg.material = material;
    if (params != NULL) engine->cfg.material_params = *params;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_set_layout(paper_engine_t *engine,
                                       paper_layout_id_t layout,
                                       const paper_layout_params_t *params) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (layout < 0 || layout >= PAPER_LAYOUT_CUSTOM + 1) return PAPER_ERR_INVALID_ARGUMENT;
    if (layout == PAPER_LAYOUT_CUSTOM && params == NULL)
        return PAPER_ERR_INVALID_ARGUMENT;
    engine->cfg.layout = layout;
    if (params != NULL) engine->cfg.layout_params = *params;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_configure(paper_engine_t *engine,
                                      const paper_render_config_t *config) {
    if (engine == NULL || config == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    paper_status_t st;
    st = paper_engine_set_dimensions(engine, config->size_id,
                                     config->size_id == PAPER_SIZE_CUSTOM
                                         ? &config->custom_dims : NULL,
                                     config->orientation);
    if (st != PAPER_OK) return st;
    st = paper_engine_set_dpi(engine, config->dpi);
    if (st != PAPER_OK) return st;
    st = paper_engine_set_seed(engine, config->seed);
    if (st != PAPER_OK) return st;
    st = paper_engine_set_material(engine, config->material,
                                   &config->material_params);
    if (st != PAPER_OK) return st;
    st = paper_engine_set_layout(engine, config->layout,
                                 &config->layout_params);
    if (st != PAPER_OK) return st;
    engine->cfg.pixel_format = config->pixel_format;
    return PAPER_OK;
}

paper_status_t paper_engine_add_stroke(paper_engine_t *engine,
                                       const paper_stroke_t *stroke) {
    if (engine == NULL || stroke == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (stroke->count > 0 && stroke->points == NULL)
        return PAPER_ERR_INVALID_ARGUMENT;
    if (engine->stroke_count == engine->stroke_capacity) {
        size_t newcap = engine->stroke_capacity ? engine->stroke_capacity * 2 : 8;
        paper_stroke_t *tmp = (paper_stroke_t *)realloc(
            engine->strokes, newcap * sizeof(*tmp));
        if (tmp == NULL) return PAPER_ERR_OUT_OF_MEMORY;
        engine->strokes = tmp;
        engine->stroke_capacity = newcap;
    }
    paper_stroke_t *dst = &engine->strokes[engine->stroke_count];
    dst->ink_type = stroke->ink_type;
    dst->r = stroke->r; dst->g = stroke->g; dst->b = stroke->b;
    dst->opacity = stroke->opacity;
    dst->count = stroke->count;
    if (stroke->count > 0) {
        dst->points = (paper_stroke_point_t *)malloc(
            stroke->count * sizeof(*dst->points));
        if (dst->points == NULL) return PAPER_ERR_OUT_OF_MEMORY;
        memcpy(dst->points, (const void *)stroke->points,
               stroke->count * sizeof(*dst->points));
    } else {
        dst->points = NULL;
    }
    engine->stroke_count++;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_clear_strokes(paper_engine_t *engine) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    size_t i;
    for (i = 0; i < engine->stroke_count; i++) {
        if (engine->strokes[i].points != NULL) {
            free((void *)engine->strokes[i].points);
            engine->strokes[i].points = NULL;
        }
    }
    engine->stroke_count = 0;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_render(paper_engine_t *engine,
                                   paper_pixel_buffer_t *buffer) {
    if (engine == NULL || buffer == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (buffer->format != PAPER_PIXEL_RGBA8888 &&
        buffer->format != PAPER_PIXEL_RGB888)
        return PAPER_ERR_INVALID_ARGUMENT;
    return paper_renderer_render(engine, buffer);
}

paper_status_t paper_engine_get_pixel_buffer(const paper_engine_t *engine,
                                             paper_pixel_buffer_t *out) {
    if (engine == NULL || out == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (!engine->configured) return PAPER_ERR_INVALID_STATE;
    out->format = PAPER_PIXEL_RGBA8888;
    out->width = engine->width;
    out->height = engine->height;
    out->stride = engine->stride;
    out->data = engine->pixels;
    out->data_size = engine->pixels_size;
    return PAPER_OK;
}

paper_status_t paper_engine_clear(paper_engine_t *engine) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    paper_renderer_clear_canvas(engine);
    return PAPER_OK;
}

const char *paper_version(void) {
    return "paper-engine 0.1.0 (Module 01)";
}

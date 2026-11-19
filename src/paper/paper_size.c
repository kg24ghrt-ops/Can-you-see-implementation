/**
 * @file paper_size.c
 * @brief Physical paper dimension database and DPI-to-pixel conversion.
 *
 * All dimensions in millimetres. ISO B5 (176x250 mm) and JIS B5
 * (182x257 mm) are intentionally distinct.
 */

#include "paper_internals.h"

static const struct {
    paper_size_id_t id;
    const char *name;
    double w_mm, h_mm;
} k_sizes[PAPER_SIZE_COUNT] = {
    {PAPER_SIZE_A0,        "A0",        841.0, 1189.0},
    {PAPER_SIZE_A1,        "A1",        594.0,  841.0},
    {PAPER_SIZE_A2,        "A2",        420.0,  594.0},
    {PAPER_SIZE_A3,        "A3",        297.0,  420.0},
    {PAPER_SIZE_A4,        "A4",        210.0,  297.0},
    {PAPER_SIZE_A5,        "A5",        148.0,  210.0},
    {PAPER_SIZE_A6,        "A6",        105.0,  148.0},
    {PAPER_SIZE_B4,        "B4 (ISO)",  250.0,  353.0},
    {PAPER_SIZE_B5,        "B5 (ISO)",  176.0,  250.0},
    {PAPER_SIZE_B6,        "B6 (ISO)",  125.0,  176.0},
    {PAPER_SIZE_US_LETTER, "US Letter", 215.9,  279.4},
    {PAPER_SIZE_US_LEGAL,  "US Legal",  215.9,  355.6},
    {PAPER_SIZE_US_EXECUTIVE,"US Executive",184.15, 266.7},
    {PAPER_SIZE_JIS_B5,    "B5 (JIS)",  182.0,  257.0},
    {PAPER_SIZE_CUSTOM,    "Custom",     0.0,    0.0},
};

paper_status_t paper_get_standard_size(paper_size_id_t size_id,
                                       paper_dimensions_t *out_dims) {
    if (out_dims == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (size_id < 0 || size_id >= PAPER_SIZE_COUNT) return PAPER_ERR_INVALID_ARGUMENT;
    if (size_id == PAPER_SIZE_CUSTOM) return PAPER_ERR_INVALID_ARGUMENT;
    out_dims->width_mm = k_sizes[size_id].w_mm;
    out_dims->height_mm = k_sizes[size_id].h_mm;
    return PAPER_OK;
}

const char *paper_get_size_name(paper_size_id_t size_id) {
    if (size_id < 0 || size_id >= PAPER_SIZE_COUNT) return NULL;
    return k_sizes[size_id].name;
}

static void paper_apply_orientation(paper_dimensions_t *dims,
                                    paper_orientation_t orient) {
    if (orient == PAPER_LANDSCAPE) {
        double tmp = dims->width_mm;
        dims->width_mm = dims->height_mm;
        dims->height_mm = tmp;
    }
}

paper_status_t paper_engine_set_dimensions(paper_engine_t *engine,
                                           paper_size_id_t size_id,
                                           const paper_dimensions_t *custom_dims,
                                           paper_orientation_t orientation) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (size_id < 0 || size_id >= PAPER_SIZE_COUNT) return PAPER_ERR_INVALID_ARGUMENT;
    if (size_id == PAPER_SIZE_CUSTOM) {
        if (custom_dims == NULL) return PAPER_ERR_INVALID_ARGUMENT;
        if (!(custom_dims->width_mm > 0.0 && custom_dims->width_mm < 10000.0)) return PAPER_ERR_INVALID_ARGUMENT;
        if (!(custom_dims->height_mm > 0.0 && custom_dims->height_mm < 10000.0)) return PAPER_ERR_INVALID_ARGUMENT;
        engine->cfg.dims = *custom_dims;
    } else {
        paper_dimensions_t d;
        paper_status_t st = paper_get_standard_size(size_id, &d);
        if (st != PAPER_OK) return st;
        engine->cfg.dims = d;
    }
    paper_apply_orientation(&engine->cfg.dims, orientation);
    engine->cfg.size_id = size_id;
    engine->cfg.orientation = orientation;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_set_dpi(paper_engine_t *engine, double dpi) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    if (!(dpi > 0.0) || dpi > 10000.0) return PAPER_ERR_INVALID_ARGUMENT;
    engine->cfg.dpi = dpi;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_set_seed(paper_engine_t *engine, uint32_t seed) {
    if (engine == NULL) return PAPER_ERR_INVALID_ARGUMENT;
    engine->cfg.seed = seed;
    engine->dirty = 1;
    return PAPER_OK;
}

paper_status_t paper_engine_get_output_dimensions(const paper_engine_t *engine,
                                                  int32_t *out_width,
                                                  int32_t *out_height) {
    if (engine == NULL || out_width == NULL || out_height == NULL)
        return PAPER_ERR_INVALID_ARGUMENT;
    if (!engine->configured) return PAPER_ERR_INVALID_STATE;
    *out_width = engine->width;
    *out_height = engine->height;
    return PAPER_OK;
}

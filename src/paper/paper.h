#ifndef PAPER_H
#define PAPER_H

/**
 * @file paper.h
 * @brief Public C API for the Hyper-Realistic Paper Rendering Engine (Module 01).
 *
 * All types and functions are C-compatible and expose a stable ABI. No C++
 * types, exceptions, or STL containers appear here. Opaque handles are used
 * for engine state; memory ownership rules are documented per function.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * Status codes
 * =========================================================================== */

/**
 * @brief Every public function returns one of these status codes.
 */
typedef enum {
    PAPER_OK = 0,                 /**< Success. */
    PAPER_ERR_INVALID_ARGUMENT = -1, /**< Invalid argument / out-of-range value. */
    PAPER_ERR_OUT_OF_MEMORY = -2,    /**< Allocation failure. */
    PAPER_ERR_INVALID_STATE = -3,    /**< Engine not configured for requested op. */
    PAPER_ERR_NOT_SUPPORTED = -4,    /**< Feature not supported in this build. */
    PAPER_ERR_IO = -5,           /**< Buffer write / read failure. */
} paper_status_t;

/* ===========================================================================
 * Paper sizes (physical dimensions in millimetres)
 * =========================================================================== */

/**
 * @brief Standard paper size identifiers.
 *
 * ISO B5 (176x250 mm) and JIS B5 (182x257 mm) are distinct and both provided.
 */
typedef enum {
    PAPER_SIZE_A0 = 0,
    PAPER_SIZE_A1,
    PAPER_SIZE_A2,
    PAPER_SIZE_A3,
    PAPER_SIZE_A4,
    PAPER_SIZE_A5,
    PAPER_SIZE_A6,
    PAPER_SIZE_B4,
    PAPER_SIZE_B5,        /**< ISO B5: 176 x 250 mm */
    PAPER_SIZE_B6,
    PAPER_SIZE_US_LETTER,   /**< 215.9 x 279.4 mm */
    PAPER_SIZE_US_LEGAL,    /**< 215.9 x 355.6 mm */
    PAPER_SIZE_US_EXECUTIVE,/**< 184.15 x 266.7 mm */
    PAPER_SIZE_JIS_B5,      /**< JIS B5: 182 x 257 mm (differs from ISO B5) */
    PAPER_SIZE_CUSTOM,
    PAPER_SIZE_COUNT,
} paper_size_id_t;

/**
 * @brief Page orientation.
 */
typedef enum {
    PAPER_PORTRAIT = 0,
    PAPER_LANDSCAPE = 1,
} paper_orientation_t;

/**
 * @brief Physical dimensions in millimetres.
 */
typedef struct {
    double width_mm;
    double height_mm;
} paper_dimensions_t;

/**
 * @brief Full specification for a standard size.
 */
typedef struct {
    paper_size_id_t id;
    const char *name;
    paper_dimensions_t dims;
} paper_size_spec_t;

/* ===========================================================================
 * Materials
 * =========================================================================== */

typedef enum {
    PAPER_MATERIAL_NATURAL_WHITE = 0,
    PAPER_MATERIAL_WARM_WHITE,
    PAPER_MATERIAL_CREAM,
    PAPER_MATERIAL_IVORY,
    PAPER_MATERIAL_AGED,
    PAPER_MATERIAL_RECYCLED,
    PAPER_MATERIAL_CUSTOM,
} paper_material_id_t;

/**
 * @brief Tunable parameters for PAPER_MATERIAL_CUSTOM (also used as hints
 * for procedural generation of named materials). All colour channels 0..1.
 */
typedef struct {
    double r;            /**< Base red albedo   (0..1) */
    double g;            /**< Base green albedo (0..1) */
    double b;            /**< Base blue albedo  (0..1) */
    double grain;        /**< Fine grain strength  (0..1) */
    double fiber;        /**< Fibre density      (0..1) */
    double luminance_var;/**< Low-freq luminance variation (0..1) */
    double edge_var;     /**< Edge irregularity  (0..1) */
} paper_material_params_t;

/* ===========================================================================
 * Layouts
 * =========================================================================== */

typedef enum {
    PAPER_LAYOUT_BLANK = 0,
    PAPER_LAYOUT_RULED,
    PAPER_LAYOUT_COLLEGE_RULED,
    PAPER_LAYOUT_WIDE_RULED,
    PAPER_LAYOUT_NARROW_RULED,
    PAPER_LAYOUT_GRAPH,
    PAPER_LAYOUT_DOT_GRID,
    PAPER_LAYOUT_MARGIN_RULED,
    PAPER_LAYOUT_CUSTOM,
} paper_layout_id_t;

/**
 * @brief A single rule specification in physical units (millimetres).
 */
typedef struct {
    double spacing_mm;   /**< Distance between adjacent rules */
    double thickness_mm; /**< Rule stroke thickness */
    double opacity;      /**< Rule opacity (0..1) */
    double r, g, b;      /**< Rule colour (0..1) */
} paper_rule_spec_t;

/**
 * @brief Layout geometry in physical units.
 */
typedef struct {
    double left_margin_mm;
    double right_margin_mm;
    double top_margin_mm;
    double bottom_margin_mm;
    double header_region_mm;
    double footer_region_mm;
    paper_rule_spec_t horizontal; /**< Horizontal rules */
    paper_rule_spec_t vertical;   /**< Vertical rules (graph/dot-grid) */
} paper_layout_params_t;

/* ===========================================================================
 * Handwriting strokes
 * =========================================================================== */

typedef enum {
    PAPER_INK_BALLPOINT = 0,
    PAPER_INK_GEL,
    PAPER_INK_FOUNTAIN,
    PAPER_INK_PENCIL,
    PAPER_INK_MARKER,
} paper_ink_type_t;

/**
 * @brief One sampled point of a vector handwriting stroke (in millimetres on
 * the page).
 */
typedef struct {
    double x;        /**< X position (mm) */
    double y;        /**< Y position (mm) */
    double width;    /**< Stroke width (mm) */
    double pressure; /**< Pen pressure (0..1) */
    double tilt;     /**< Pen tilt (0..1, optional) */
    double velocity; /**< Stroke velocity (optional) */
    double timestamp;/**< Sample time (seconds, optional) */
} paper_stroke_point_t;

/**
 * @brief A complete handwriting stroke.
 *
 * The engine does not copy `points`; the caller must keep the array alive
 * until paper_engine_add_stroke returns (the engine copies point data
 * internally during the call).
 */
typedef struct {
    paper_stroke_point_t *points;
    size_t count;
    paper_ink_type_t ink_type;
    double r, g, b;    /**< Ink colour (0..1) */
    double opacity;    /**< Ink opacity (0..1) */
} paper_stroke_t;

/* ===========================================================================
 * Pixel buffer
 * =========================================================================== */

typedef enum {
    PAPER_PIXEL_RGBA8888 = 0,
    PAPER_PIXEL_RGB888 = 1,
} paper_pixel_format_t;

/**
 * @brief Pixel buffer abstraction.
 *
 * If `data` is NULL and `data_size` is 0 on input to paper_engine_render(),
 * the engine allocates the buffer and the caller must release it with
 * paper_buffer_release(). If `data` is non-NULL, the engine writes into the
 * caller-provided memory and does not take ownership.
 */
typedef struct {
    paper_pixel_format_t format;
    int32_t width;
    int32_t height;
    int32_t stride;        /**< Bytes per row (0 => auto-computed) */
    uint8_t *data;
    size_t data_size;      /**< Total allocated bytes */
} paper_pixel_buffer_t;

/**
 * @brief Release a buffer previously allocated by the engine.
 * Safe to call with NULL.
 */
void paper_buffer_release(paper_pixel_buffer_t *buffer);

/**
 * @brief Allocate a pixel buffer. `stride` of 0 means auto-compute
 * (width * bytes-per-pixel). The buffer must be released with
 * paper_buffer_release().
 */
paper_status_t paper_buffer_allocate(paper_pixel_buffer_t *buf,
                                     paper_pixel_format_t fmt,
                                     int32_t width, int32_t height,
                                     int32_t stride);

/**
 * @brief Copy a w x h rectangle from src to dst, respecting each buffer's
 * stride. Both buffers must use the same pixel format.
 */
paper_status_t paper_buffer_copy(const paper_pixel_buffer_t *dst,
                                 const paper_pixel_buffer_t *src,
                                 int32_t w, int32_t h);

/* ===========================================================================
 * Rendering configuration
 * =========================================================================== */

typedef struct {
    paper_size_id_t size_id;
    paper_dimensions_t custom_dims;   /**< Used when size_id == PAPER_SIZE_CUSTOM */
    paper_orientation_t orientation;
    double dpi;                       /**< Output resolution (72, 96, 150, 300, 600, custom) */
    uint32_t seed;                    /**< Deterministic procedural seed */
    paper_material_id_t material;
    paper_material_params_t material_params;
    paper_layout_id_t layout;
    paper_layout_params_t layout_params;
    paper_pixel_format_t pixel_format;
} paper_render_config_t;

/* ===========================================================================
 * Opaque engine handle
 * =========================================================================== */

typedef struct paper_engine paper_engine_t;

/* ===========================================================================
 * Engine lifecycle
 * =========================================================================== */

/**
 * @brief Create a new rendering engine instance.
 * @param out Receives the opaque handle. Caller must later call
 *            paper_engine_destroy().
 * @return PAPER_OK on success, PAPER_ERR_OUT_OF_MEMORY on allocation failure.
 * Ownership: caller owns the returned handle.
 */
paper_status_t paper_engine_create(paper_engine_t **out);

/**
 * @brief Destroy an engine and release all associated memory.
 * Safe to call with NULL. After this, `engine` must not be used.
 */
void paper_engine_destroy(paper_engine_t *engine);

/* ===========================================================================
 * Configuration
 * =========================================================================== */

/**
 * @brief Set physical page dimensions and orientation.
 */
paper_status_t paper_engine_set_dimensions(paper_engine_t *engine,
                                           paper_size_id_t size_id,
                                           const paper_dimensions_t *custom_dims,
                                           paper_orientation_t orientation);

/**
 * @brief Set output resolution in dots per inch. Must be > 0.
 */
paper_status_t paper_engine_set_dpi(paper_engine_t *engine, double dpi);

/**
 * @brief Set the deterministic procedural seed. Same seed + settings => same
 * output pixels.
 */
paper_status_t paper_engine_set_seed(paper_engine_t *engine, uint32_t seed);

/**
 * @brief Set the paper material. `params` may be NULL for named materials;
 * required for PAPER_MATERIAL_CUSTOM.
 */
paper_status_t paper_engine_set_material(paper_engine_t *engine,
                                         paper_material_id_t material,
                                         const paper_material_params_t *params);

/**
 * @brief Set the page layout. `params` may be NULL for preset layouts;
 * required for PAPER_LAYOUT_CUSTOM.
 */
paper_status_t paper_engine_set_layout(paper_engine_t *engine,
                                       paper_layout_id_t layout,
                                       const paper_layout_params_t *params);

/**
 * @brief Set the full render configuration at once (convenience).
 */
paper_status_t paper_engine_configure(paper_engine_t *engine,
                                      const paper_render_config_t *config);

/* ===========================================================================
 * Queries
 * =========================================================================== */

/**
 * @brief Look up the physical dimensions (mm) of a standard size.
 * @return PAPER_OK on success, PAPER_ERR_INVALID_ARGUMENT for unknown id.
 */
paper_status_t paper_get_standard_size(paper_size_id_t size_id,
                                       paper_dimensions_t *out_dims);

/**
 * @brief Get the name of a standard size. Returns NULL for unknown ids.
 */
const char *paper_get_size_name(paper_size_id_t size_id);

/**
 * @brief Compute the rendered output pixel dimensions for the current
 * configuration (width x height in pixels).
 */
paper_status_t paper_engine_get_output_dimensions(const paper_engine_t *engine,
                                                  int32_t *out_width,
                                                  int32_t *out_height);

/**
 * @brief Get a pointer to the engine's internal pixel buffer description
 * after a successful render. The buffer remains owned by the engine.
 */
paper_status_t paper_engine_get_pixel_buffer(const paper_engine_t *engine,
                                             paper_pixel_buffer_t *out);

/* ===========================================================================
 * Strokes
 * =========================================================================== */

/**
 * @brief Add a handwriting stroke to the render queue. Strokes are rendered
 * on top of the paper material during paper_engine_render().
 */
paper_status_t paper_engine_add_stroke(paper_engine_t *engine,
                                       const paper_stroke_t *stroke);

/**
 * @brief Remove all queued strokes without clearing the paper texture.
 */
paper_status_t paper_engine_clear_strokes(paper_engine_t *engine);

/* ===========================================================================
 * Rendering
 * =========================================================================== */

/**
 * @brief Render the configured page (material + layout + queued strokes)
 * into `buffer`.
 *
 * If buffer->data is NULL, the engine allocates a buffer of the required
 * size and fills buffer->data/data_size/stride; the caller releases it with
 * paper_buffer_release(). If buffer->data is non-NULL, the engine writes
 * into it; buffer->width/height/stride must be large enough (query via
 * paper_engine_get_output_dimensions) and format must match pixel_format.
 *
 * @return PAPER_OK on success.
 */
paper_status_t paper_engine_render(paper_engine_t *engine,
                                   paper_pixel_buffer_t *buffer);

/**
 * @brief Clear the engine's internal pixel buffer (fill with transparent
 * black) without re-rendering.
 */
paper_status_t paper_engine_clear(paper_engine_t *engine);

/* ===========================================================================
 * Version
 * =========================================================================== */

/**
 * @brief Return a static version string, e.g. "0.1.0".
 */
const char *paper_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PAPER_H */

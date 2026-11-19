# Public API reference

Full documentation (every parameter, return value and ownership rule) is in
`src/paper/paper.h`. This page is a compact index.

## Status codes

`paper_status_t`: `PAPER_OK`, `PAPER_ERR_INVALID_ARGUMENT`,
`PAPER_ERR_OUT_OF_MEMORY`, `PAPER_ERR_INVALID_STATE`,
`PAPER_ERR_NOT_SUPPORTED`, `PAPER_ERR_IO`. All public functions return one.

## Lifecycle

- `paper_engine_create(paper_engine_t **out)` — create; caller owns handle.
- `paper_engine_destroy(paper_engine_t *engine)` — release everything; NULL-safe.
- `paper_version(void)` — static version string.

## Configuration

- `paper_engine_set_dimensions(engine, size_id, custom_dims, orientation)`
- `paper_engine_set_dpi(engine, dpi)` — dpi > 0.
- `paper_engine_set_seed(engine, seed)` — deterministic procedural seed.
- `paper_engine_set_material(engine, material, params)` — params required for CUSTOM.
- `paper_engine_set_layout(engine, layout, params)` — params required for CUSTOM.
- `paper_engine_configure(engine, config)` — set everything at once.

## Queries

- `paper_get_standard_size(size_id, &dims)` — physical mm for a standard size.
- `paper_get_size_name(size_id)` — name string, NULL for unknown.
- `paper_engine_get_output_dimensions(engine, &w, &h)` — rendered pixel size.
- `paper_engine_get_pixel_buffer(engine, &buf)` — engine-owned RGBA8888 canvas (do not release).

## Strokes

- `paper_engine_add_stroke(engine, &stroke)` — copies points internally.
- `paper_engine_clear_strokes(engine)` — drop queued strokes.

## Rendering

- `paper_engine_render(engine, &buf)` — render material + layout + strokes.
  - If `buf->data == NULL`: engine allocates; caller releases via `paper_buffer_release`.
  - If `buf->data != NULL`: engine writes into caller memory; caller owns it.
- `paper_engine_clear(engine)` — clear internal canvas.

## Pixel buffer

- `paper_buffer_allocate(&buf, fmt, w, h, stride)` — allocate (stride 0 = auto).
- `paper_buffer_release(&buf)` — free; NULL-safe.
- `paper_buffer_copy(&dst, &src, w, h)` — copy rectangle respecting strides.

## Types (key)

- `paper_size_id_t` — A0..A6, B4, B5 (ISO), B6, US Letter/Legal/Executive, JIS B5, CUSTOM.
- `paper_orientation_t` — PORTRAIT / LANDSCAPE.
- `paper_material_id_t` — natural/warm white, cream, ivory, aged, recycled, custom.
- `paper_layout_id_t` — blank, ruled, college/wide/narrow-ruled, graph, dot-grid, margin-ruled, custom.
- `paper_ink_type_t` — ballpoint, gel, fountain, pencil, marker.
- `paper_pixel_format_t` — RGBA8888 / RGB888.
- `paper_engine_t` — opaque handle.

## ABI

The API is plain C: no classes, exceptions, STL, or C++-specific types cross
the boundary. Symbols are exported as `paper_*` and are identical across native,
Android `.so` and WebAssembly builds.

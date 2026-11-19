# Architecture

## Module boundaries

The engine is layered so rendering logic never touches platform APIs.

```
application (C / C++ / JNI / JS)
        |
        v
src/paper/paper.h        public C API (stable ABI)
        |
        v
src/paper/paper_engine.c engine state, configuration, public entry points
        |
        v
src/paper/paper_renderer.c CPU pipeline: material -> layout -> strokes -> emit
        |          |          |
        v          v          v
paper_material.c  paper_layout.c  paper_stroke.c
        |
        v
paper_rng.c       deterministic PCG32 + value noise / fBM
paper_buffer.c    RGBA8888 / RGB888 pixel buffer, stride, alloc/free
paper_size.c      physical dimensions (mm), DPI conversion, orientation
```

- **Core rendering algorithms:** C (`src/paper/*.c`).
- **Public API:** C-compatible header (`paper.h`); opaque `paper_engine_t` handle.
- **C++:** none required; the header compiles as C++ and examples show C++ usage.
- **Platform integrations:** isolated from rendering logic; none exist inside `src/paper` (no Android NDK, no GPU, no Vulkan/OpenGL).
- **No global mutable rendering state:** all state lives in `paper_engine_t`.

## Rendering pipeline (`paper_engine_render`)

1. `paper_renderer_prepare` — resolve output pixel dimensions from physical
   size × DPI (`mm * dpi / 25.4`), portrait/landscape swap, allocate/reuse the
   RGBA8888 working canvas.
2. Canvas cleared.
3. `paper_material_render` — procedural paper texture: base colour, low-frequency
   luminance variation, fine grain, scattered fibres, subtle edge darkening. All
   deterministic from the seed.
4. `paper_layout_render` — ruled/graph/dot-grid lines and margins, specified in
   millimetres and converted to pixels at render time; anti-aliased lines.
5. `paper_stroke_render_all` — queued handwriting strokes blended into the
   paper with variable width, pressure, opacity and optional pencil grain.
6. `paper_renderer_emit` — convert the RGBA canvas to the requested format
   (RGBA8888 or RGB888), respecting explicit stride; allocate the caller buffer
   if `buffer->data` is NULL.

No intermediate full-image copies beyond the single working canvas and the
final output buffer.

## Physical units

All dimensions and rules are stored in millimetres. Pixel dimensions are
computed at render time from DPI, so the same configuration renders correctly
at 72, 150, 300 or 600 dpi without a separate implementation. ISO B5
(176×250 mm) and JIS B5 (182×257 mm) are distinct entries.

## Memory ownership

| Function | Ownership rule |
|----------|----------------|
| `paper_engine_create` | Caller owns handle; release with `paper_engine_destroy`. |
| `paper_engine_destroy` | Releases engine, canvas and queued strokes. Safe with NULL. |
| `paper_engine_render` (buf->data == NULL) | Engine allocates `buf->data`; caller releases with `paper_buffer_release`. |
| `paper_engine_render` (buf->data != NULL) | Engine writes into caller memory; no ownership taken. Caller must ensure size/stride are adequate. |
| `paper_engine_get_pixel_buffer` | Returns engine-owned canvas pointer; caller must NOT release it. |
| `paper_buffer_allocate` | Caller owns result; release with `paper_buffer_release`. |
| `paper_buffer_copy` | Copies into caller-provided `dst`. |
| `paper_engine_add_stroke` | Engine copies the stroke's points internally; caller's array may be freed after the call returns. |

## Determinism

All procedural generation uses a PCG32 PRNG seeded via `paper_engine_set_seed`.
Identical configuration + seed ⇒ identical output pixels, across runs and
platforms.

## Precision

Physical measurements use `double`. Pixel dimensions are `int32_t`. The
working canvas is RGBA8888 (8 bits/channel).

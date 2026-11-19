# Paper Rendering Engine — Module 01 (Paper Rendering Core)

A C-first, CPU-based, physically convincing paper rendering library. It renders
a sheet of paper that looks photographed or scanned rather than a flat digital
document: natural colouration, fibre/grain texture, ruled layouts, and
pressure-sensitive handwriting strokes that blend into the paper material.

The public API is a stable C ABI, usable from C, C++, Android (JNI) and
WebAssembly. Core rendering has no platform dependencies — no Android NDK, no
GPU, no Vulkan/OpenGL inside `src/paper`.

## Features

- Physical paper dimensions in millimetres: ISO A0–A6, B4–B6, US Letter/Legal/Executive, JIS B5 (distinct from ISO B5), custom sizes.
- Portrait / landscape orientation.
- DPI → pixel conversion (72, 96, 150, 300, 600, custom) preserving proportions.
- Procedural paper materials: natural white, warm white, cream, ivory, aged, recycled, custom.
- Deterministic procedural texture from a configurable seed (identical settings + seed ⇒ identical pixels).
- Layouts: blank, ruled, college/wide/narrow-ruled, graph, dot-grid, margin-ruled, custom; rules in physical units.
- Vector handwriting strokes: ballpoint, gel, fountain, pencil, marker; anti-aliased, variable width, pressure-sensitive, ink blending into paper.
- CPU backend with RGBA8888 / RGB888 pixel buffers, explicit stride, safe alloc/free.
- CMake build: Linux shared + static library. Android `.so` and WebAssembly `.wasm` via external toolchains (see `docs/build.md`).

## Repository layout

```
src/paper/        implementation (C)
src/paper/paper.h public C API (stable ABI)
src/paper/paper_internals.h internal shared declarations
tests/test_paper_*.c  automated tests (asserts + ASan/UBSan)
examples/         C and C++ usage examples
CMakeLists.txt    primary build system
docs/             build, architecture and API documentation
```

The pre-existing `src/Vec3.*` and `tests/test_vec3.cpp` (old C++ core) are left
untouched; this module is additive.

## Build (native Linux/Termux)

CMake is the primary build system. This environment does not ship `cmake`, so
the commands below compile directly with `clang`; use CMake on systems that have
it.

```bash
# Build and run all tests with AddressSanitizer + UBSan
mkdir -p build
for t in tests/test_paper_*.c; do
  n=$(basename "$t" .c)
  clang -std=c11 -O1 -g -Wall -Wextra -Werror -Isrc "$t" src/paper/*.c \
        -o "build/$n" -lm -fsanitize=address,undefined
  "build/$n"
done

# Shared library
clang -std=c11 -O2 -fPIC -Isrc -shared src/paper/*.c -o build/libpaper.so -lm

# Static library
clang -std=c11 -O2 -Isrc -c src/paper/*.c && ar rcs build/libpaper.a *.o && rm -f *.o
```

All 7 tests pass under ASan+UBSan. The `.so` exports the full public C API
(`nm -D --defined-only build/libpaper.so | grep paper`).

## C usage example

See `examples/c_example.c` and `examples/cpp_example.cpp`. Quick summary:

```c
#include "paper/paper.h"
#include <stdio.h>

int main(void) {
    paper_engine_t *e = NULL;
    paper_engine_create(&e);
    paper_engine_set_dpi(e, 150.0);
    paper_engine_set_seed(e, 42u);
    paper_engine_set_material(e, PAPER_MATERIAL_WARM_WHITE, NULL);
    paper_engine_set_layout(e, PAPER_LAYOUT_COLLEGE_RULED, NULL);

    paper_pixel_buffer_t buf = {0};
    buf.format = PAPER_PIXEL_RGBA8888;
    paper_engine_render(e, &buf);   // engine allocates buf->data

    // buf.data now holds RGBA8888 pixels (buf.width x buf.height, buf.stride)
    // ... write to PNG / upload to GPU / pass to JS ...

    paper_buffer_release(&buf);     // release engine-allocated buffer
    paper_engine_destroy(e);
    return 0;
}
```

Memory ownership: `paper_engine_create` returns a handle the caller owns and
must release with `paper_engine_destroy`. `paper_engine_render` allocates the
output buffer when `buffer->data` is NULL; release it with
`paper_buffer_release`. If you provide `buffer->data`, the engine writes into it
and does not take ownership.

## Build targets

| Target | How |
|--------|-----|
| Linux shared (`libpaper.so`) | CMake `paper` or `clang -shared -fPIC` |
| Linux static (`libpaper.a`)  | CMake `paper_static` or `ar` |
| Android `.so` (arm64-v8a, armeabi-v7a, x86_64, x86) | CMake + Android NDK toolchain (`docs/build.md`) |
| WebAssembly `.wasm` + JS glue | Emscripten `emcmake cmake` / `emcc` (`docs/build.md`) |

## Documentation

- `docs/build.md` — native, Android NDK and WebAssembly build instructions.
- `docs/architecture.md` — module boundaries, rendering pipeline, memory rules.
- `src/paper/paper.h` — self-documenting public API (every function, parameter,
  return value and ownership rule is commented).
- `examples/c_example.c`, `examples/cpp_example.cpp` — runnable examples.

## Testing

Run the suite (ASan+UBSan):

```bash
ctest --test-dir build   # when using CMake
# or directly:
for t in build/test_paper_*; do "$t"; done
```

Tests cover: ISO/JIS/US dimensions, B5-vs-JIS-B5 distinction, DPI→pixel,
orientation, texture determinism, layouts, stroke rendering, buffer stride,
allocation/free, invalid input handling and render consistency.

## License / stability

Module 01 foundation. The C ABI in `paper.h` is intended to remain stable
across WASM and Android builds; GPU acceleration can be added later without
changing the public API.

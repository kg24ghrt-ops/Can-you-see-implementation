# Build instructions

The paper engine uses **CMake** as the primary build system. The implementation
is C11; the public API is a plain C header (`src/paper/paper.h`) with a stable
ABI. No C++, exceptions, STL, or platform rendering APIs appear in core.

Source files: `src/paper/*.c`. Public header: `src/paper/paper.h`.

## 1. Native (Linux / Termux / any POSIX)

CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build -V
```

This produces `build/libpaper.so` (shared), `build/libpaper.a` (static) and the
test binaries.

Without `cmake` (e.g. this Termux environment which only has `clang`+`make`):

```bash
# tests (ASan+UBSan)
for t in tests/test_paper_*.c; do
  n=$(basename "$t" .c)
  clang -std=c11 -O1 -g -Wall -Wextra -Werror -Isrc "$t" src/paper/*.c \
        -o "build/$n" -lm -fsanitize=address,undefined
  "build/$n"
done
# shared library
clang -std=c11 -O2 -fPIC -Isrc -shared src/paper/*.c -o build/libpaper.so -lm
# static library
clang -std=c11 -O2 -Isrc -c src/paper/*.c && ar rcs build/libpaper.a *.o && rm -f *.o
```

## 2. Android native library (`.so`)

Use the Android NDK CMake toolchain. Core code is platform-free (no NDK calls
inside `src/paper`); platform integration (JNI, asset loading) belongs in a
thin layer outside `src/paper`.

Example (adjust `ANDROID_NDK` and `ANDROID_ABI`):

```bash
export ANDROID_NDK=/path/to/android-ndk
export ANDROID_ABI=arm64-v8a   # armeabi-v7a | x86_64 | x86

cmake -S . -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=$ANDROID_ABI \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -DPAPER_BUILD_TESTS=OFF
cmake --build build-android -j
# -> build-android/libpaper.so
```

Verify exported symbols:

```bash
aarch64-linux-android-nm -D --defined-only build-android/libpaper.so | grep paper
```

The `.so` exports `paper_engine_create`, `paper_engine_render`,
`paper_engine_get_pixel_buffer`, `paper_buffer_release`, etc. — suitable for
consumption from Java/Kotlin via JNI or from native Android code.

## 3. WebAssembly (Emscripten)

Use the Emscripten SDK compiler wrapper. The public C API is exported and can
be called from JavaScript.

```bash
export EMSDK=/path/to/emsdk
source $EMSDK/emsdk_env.sh

emcmake cmake -S . -B build-wasm \
  -DCMAKE_BUILD_TYPE=Release \
  -DPAPER_BUILD_TESTS=OFF
cmake --build build-wasm -j
# -> build-wasm/paper.wasm  (+ paper.js glue if EMCC settings emit it)
```

Or compile directly:

```bash
emcc -O2 -s WASM=1 -s EXPORTED_FUNCTIONS='["_paper_engine_create","_paper_engine_render",...]' \
     -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
     -Isrc src/paper/*.c -o build-wasm/paper.js
```

Browser-compatible memory management: the engine allocates with `malloc`/`free`
(emscripten's `malloc` is `emmalloc` by default) and exposes
`paper_buffer_release` so the JS side can free engine-allocated buffers through
the C ABI.

## 4. CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `PAPER_BUILD_SHARED` | ON | Build `libpaper.so` |
| `PAPER_BUILD_STATIC` | ON | Build `libpaper.a` |
| `PAPER_BUILD_TESTS` | ON | Build and register the test suite |

## 5. Verification checklist

- [ ] `cmake --build` succeeds; `ctest` passes.
- [ ] `libpaper.so` exports all `paper_*` public symbols.
- [ ] A consumer links against the `.so`/`.a` and renders a page.
- [ ] Android: `.so` builds for each target ABI and `nm -D` shows the symbols.
- [ ] WASM: `.wasm` loads and exported C functions are callable from JS.

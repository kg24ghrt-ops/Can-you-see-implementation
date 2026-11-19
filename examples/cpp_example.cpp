// examples/cpp_example.cpp
//
// C++ usage example for the Paper Rendering Engine (Module 01). The engine is
// a C library; this file shows idiomatic C++ consumption (RAII wrapper over
// the opaque handle and buffer) without exposing any C++ types across the ABI.
//
// Build:
//   clang++ -std=c++20 -O2 -Isrc examples/cpp_example.cpp src/paper/*.c -o cpp_example -lm

#include "paper/paper.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

namespace paper {

// RAII wrapper: engine handle owned by C++ scope, released on destruction.
class Engine {
  public:
    Engine() {
        if (paper_engine_create(&handle_) != PAPER_OK) handle_ = nullptr;
    }
    ~Engine() { if (handle_) paper_engine_destroy(handle_); }
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    operator bool() const { return handle_ != nullptr; }
    paper_engine_t* get() { return handle_; }

  private:
    paper_engine_t* handle_{nullptr};
};

// RAII wrapper: engine-allocated pixel buffer released on destruction.
class Buffer {
  public:
    explicit Buffer(paper_pixel_format_t fmt)
        : buf_{} {
        buf_.format = fmt;
    }
    ~Buffer() { paper_buffer_release(&buf_); }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    paper_pixel_buffer_t* get() { return &buf_; }

  private:
    paper_pixel_buffer_t buf_;
};

}  // namespace paper

int main() {
    paper::Engine engine;
    if (!engine) {
        std::fprintf(stderr, "failed to create engine\n");
        return 1;
    }

    paper_render_config_t cfg{};
    cfg.size_id = PAPER_SIZE_A4;
    cfg.orientation = PAPER_PORTRAIT;
    cfg.dpi = 150.0;
    cfg.seed = 99u;
    cfg.material = PAPER_MATERIAL_NATURAL_WHITE;
    cfg.layout = PAPER_LAYOUT_BLANK;
    cfg.pixel_format = PAPER_PIXEL_RGBA8888;
    if (paper_engine_configure(engine.get(), &cfg) != PAPER_OK) {
        std::fprintf(stderr, "configure failed\n");
        return 1;
    }

    paper::Buffer buf(PAPER_PIXEL_RGBA8888);
    if (paper_engine_render(engine.get(), buf.get()) != PAPER_OK) {
        std::fprintf(stderr, "render failed\n");
        return 1;
    }

    int w = 0, h = 0;
    paper_engine_get_output_dimensions(engine.get(), &w, &h);
    std::fprintf(stderr, "C++ rendered %d x %d px\n", w, h);

    // buf.get()->data holds buf.get()->data_size bytes of RGBA8888.
    std::vector<unsigned char> copy(buf.get()->data,
                                    buf.get()->data + buf.get()->data_size);
    (void)copy;

    return 0;
}

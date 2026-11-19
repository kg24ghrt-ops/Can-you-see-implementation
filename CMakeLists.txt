cmake_minimum_required(VERSION 3.10)
project(paper_engine C VERSION 0.1.0)

# Hyper-Realistic Paper Rendering Engine - Module 01
#
# Primary build system: C.
# Targets: native Linux shared/static, Android .so (via NDK toolchain),
# WebAssembly .wasm (via Emscripten). Platform toolchains are supplied
# externally (see BUILD.md); this file stays platform-agnostic.

set(PAPER_SRC
    src/paper/paper_engine.c
    src/paper/paper_size.c
    src/paper/paper_rng.c
    src/paper/paper_material.c
    src/paper/paper_layout.c
    src/paper/paper_stroke.c
    src/paper/paper_buffer.c
    src/paper/paper_renderer.c
)

set(PAPER_TEST_SRC
    tests/test_paper_size.c
    tests/test_paper_rng.c
    tests/test_paper_material.c
    tests/test_paper_layout.c
    tests/test_paper_buffer.c
    tests/test_paper_engine.c
    tests/test_paper_render.c
)

option(PAPER_BUILD_TESTS "Build automated tests" ON)
option(PAPER_BUILD_SHARED "Build shared library" ON)
option(PAPER_BUILD_STATIC "Build static library" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

set(PAPER_COMPILE_FLAGS -Wall -Wextra -Werror -O2 -std=c11)

add_library(paper ${PAPER_SRC})
target_include_directories(paper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(paper PRIVATE ${PAPER_COMPILE_FLAGS})
set_target_properties(paper PROPERTIES
    VERSION ${PAPER_VERSION}
    SOVERSION 0
)

if(PAPER_BUILD_STATIC)
    add_library(paper_static STATIC ${PAPER_SRC})
    target_include_directories(paper_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(paper_static PRIVATE ${PAPER_COMPILE_FLAGS})
endif()

if(PAPER_BUILD_TESTS)
    enable_testing()
    set(_sanitize -fsanitize=address,undefined)
    foreach(_f ${PAPER_TEST_SRC})
        get_filename_component(_name ${_f} NAME_WE)
        add_executable(${_name} ${_f} ${PAPER_SRC})
        target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_compile_options(${_name} PRIVATE ${PAPER_COMPILE_FLAGS} -g ${_sanitize})
        target_link_libraries(${_name} PRIVATE m)
        add_test(NAME ${_name} COMMAND ${_name})
    endforeach()
endif()

# Install rules
install(TARGETS paper paper_static
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES src/paper/paper.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/paper)

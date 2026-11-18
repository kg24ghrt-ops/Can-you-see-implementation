CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Werror
SRC := $(wildcard src/*.cpp)
TESTS := $(wildcard tests/*.cpp)
.RECIPEPREFIX = >
.PHONY: build test clean
build:
> mkdir -p build
> $(CXX) $(CXXFLAGS) $(SRC) -c -o build/core.o
> echo "Build OK"
test: build
> for f in $(TESTS); do name=$$(basename $$f .cpp); $(CXX) $(CXXFLAGS) -Isrc $$f build/core.o -o build/$$name -lm; ./build/$$name; done
clean:
> rm -rf build

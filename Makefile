# Local helper - CI is the source of truth, you do not need this locally
.RECIPEPREFIX = >
FSTAR ?= fstar.exe
KRML ?= krml
CLANG ?= clang
SRC := $(wildcard src/*.fst)
OUT_DIR := out
BUILD_DIR := build

.PHONY: verify extract build test clean

verify:
> $(FSTAR) --include src $(SRC)

extract: verify
> mkdir -p $(OUT_DIR)
> $(KRML) -o $(OUT_DIR) $(SRC)

build: extract
> mkdir -p $(BUILD_DIR)
> $(CLANG) -std=c11 -O2 -Wall -Wextra -I$(OUT_DIR) -c $(OUT_DIR)/*.c -o $(BUILD_DIR)/core.o

test: build
> for f in tests/c/*.c; do name=$$(basename $$f .c); $(CLANG) -std=c11 -O2 -Wall -Wextra -I$(OUT_DIR) -Itests/c $$f $(BUILD_DIR)/core.o -o $(BUILD_DIR)/$$name -lm; ./$(BUILD_DIR)/$$name; done

clean:
> rm -rf $(OUT_DIR) $(BUILD_DIR)

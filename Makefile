SRC_DIR = src
INC = $(wildcard $(SRC_DIR)/*.h)
SRC = $(wildcard $(SRC_DIR)/*.c)

DIST_DIR = dist
WASM = $(DIST_DIR)/durfeechess.wasm

all: $(WASM)

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

$(WASM): $(DIST_DIR) $(SRC) $(INC)
	clang \
	  -Werror \
	  -Wall \
	  -O3 \
	  -flto \
	  --target=wasm32 \
	  -nostdlib \
	  -Wl,--no-entry \
	  -Wl,--export=init_game \
	  -Wl,--export=attempt_move \
	  -Wl,--export=BOARD \
	  -Wl,--allow-undefined \
	  -Wl,--lto-O3 \
	  -o $(WASM) \
	  $(SRC)

NAME = waveforest
CC = gcc
PACKAGES = $(shell pkg-config --libs jack) -lm
INCLUDE = -Iinclude -Iexternal/include
SANITIZE = -fsanitize=address
ADDITIONAL_CFLAGS =
CFLAGS = $(PACKAGES) $(INCLUDE) -Wall -Wextra -Wshadow -pedantic -Wstrict-prototypes -march=native $(ADDITIONAL_CFLAGS)
CFLAGS_STANDALONE = $(CFLAGS) $(shell pkg-config --libs raylib)
CFLAGS_SHARED = $(CFLAGS) -fpic
CFLAGS_DEBUG = $(CFLAGS) -DDEBUG -ggdb
CFLAGS_ASAN = $(CFLAGS_DEBUG) $(SANITIZE)

SRC = $(wildcard src/*.c) $(wildcard external/src/*.c)
BUILD_DIR = build
SRC_DIR = src
SRC_DIR_NODES = nodes

standalone: $(BUILD_DIR)/standalone
standalone_asan: $(BUILD_DIR)/standalone_asan
standalone_debug: $(BUILD_DIR)/standalone_debug

ARGS=

run: $(BUILD_DIR)/standalone_debug
	@echo "WARNING: no address sanitation enabled, consider running with 'make run_asan' when developing."
	$(BUILD_DIR)/standalone_debug $(ARGS)

run_asan: $(BUILD_DIR)/standalone_asan
	$(BUILD_DIR)/standalone_asan $(ARGS)

STANDALONE_SRC = $(SRC) $(wildcard $(SRC_DIR)/standalone/*.c) $(wildcard $(SRC_DIR)/gui/*.c)

$(BUILD_DIR)/standalone: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) $^ -o $@
$(BUILD_DIR)/standalone_asan: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) $(SANITIZE) -ggdb -DDEBUG $^ -o $@
$(BUILD_DIR)/standalone_debug: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) -ggdb -DDEBUG $^ -o $@

# LV2 plugin build
.PHONY: lv2
lv2: $(BUILD_DIR) $(BUILD_DIR)/lv2 $(BUILD_DIR)/lv2/$(NAME).so

LV2_SRC = $(SRC) $(wildcard $(SRC_DIR)/lv2/*.c)
$(BUILD_DIR)/lv2/$(NAME).so: $(LV2_SRC)
	$(CC) $(CFLAGS_SHARED) -shared $^ -o $@

lv2_install: lv2
	install -d /usr/lib/lv2/waveforest.lv2
	install -m 644 build/lv2/*.so  /usr/lib/lv2/waveforest.lv2/
	install -m 644 build/lv2/*.ttl /usr/lib/lv2/waveforest.lv2/

# Node build

OBJS_NODES = $(patsubst $(SRC_DIR_NODES)/%.c, $(BUILD_DIR)/nodes/%.so, $(wildcard $(SRC_DIR_NODES)/*.c))

nodes: $(BUILD_DIR)/nodes $(OBJS_NODES)

$(BUILD_DIR)/nodes/%.o: $(SRC_DIR_NODES)/%.c
	@echo $(OBJS_NODES)
	$(CC) $(CFLAGS_SHARED) -c $< -o $@

$(BUILD_DIR)/nodes/%.so: $(BUILD_DIR)/nodes/%.o
	$(CC) -shared $< -o $@

.PHONY: clean
clean:
	rm $(BUILD_DIR)/lv2/*.so -f


$(BUILD_DIR):
	mkdir -p $@
$(BUILD_DIR)/lv2:
	mkdir -p $@
$(BUILD_DIR)/nodes:
	mkdir -p $@

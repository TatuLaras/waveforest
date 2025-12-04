NAME = waveforest
CC = gcc
PACKAGES = $(shell pkg-config --libs jack) -lm
INCLUDE = -Iinclude -Iexternal/include
SANITIZE = -fsanitize=address
CFLAGS = $(PACKAGES) $(INCLUDE) -Wall -Wextra -Wshadow -pedantic -Wstrict-prototypes -march=native
CFLAGS_STANDALONE = $(CFLAGS) $(shell pkg-config --libs raylib)
CFLAGS_SHARED = $(CFLAGS) -fpic
NODES_INCLUDE = -Inodes/include -Iinclude -Iexternal/include
CFLAGS_NODES = $(NODES_INCLUDE) -Wall -Wextra -Wshadow -pedantic -Wstrict-prototypes -march=native -fpic
CFLAGS_DEBUG = $(CFLAGS) -DDEBUG -ggdb
CFLAGS_ASAN = $(CFLAGS_DEBUG) $(SANITIZE)

SRC = $(wildcard src/*.c) $(wildcard external/src/*.c)
BUILD_DIR = build
SRC_DIR = src
SRC_DIR_NODES = nodes

standalone: $(BUILD_DIR) nodes $(BUILD_DIR)/standalone
for_installation: $(BUILD_DIR) nodes $(BUILD_DIR)/for_installation
standalone_asan: $(BUILD_DIR) nodes $(BUILD_DIR)/standalone_asan
standalone_debug: $(BUILD_DIR) nodes $(BUILD_DIR)/standalone_debug

ARGS=

run: standalone_debug
	@echo "WARNING: no address sanitation enabled, consider running with 'make run_asan' when developing."
	$(BUILD_DIR)/standalone_debug $(ARGS)

run_asan: standalone_asan
	$(BUILD_DIR)/standalone_asan $(ARGS)

STANDALONE_SRC = $(SRC) $(wildcard $(SRC_DIR)/standalone/*.c) $(wildcard $(SRC_DIR)/gui/*.c)

$(BUILD_DIR)/for_installation: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) -DRESOURCE_PATH='"/usr/share/waveforest/"' -DNODE_DIR='"/usr/share/waveforest/nodes/"' -DPATCH_DIR='"~/.waveforest/"' $^ -o $@

$(BUILD_DIR)/standalone: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) -DRESOURCE_PATH='"res/"' -DNODE_DIR='"build/nodes"' -DPATCH_DIR='"$(HOME)/.waveforest/"' $^ -o $@
$(BUILD_DIR)/standalone_asan: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) $(SANITIZE) -ggdb -DDEBUG -DRESOURCE_PATH='"res/"' -DNODE_DIR='"build/nodes"' -DPATCH_DIR='"$(HOME)/.waveforest/"' $^ -o $@
$(BUILD_DIR)/standalone_debug: $(STANDALONE_SRC) 
	$(CC) $(CFLAGS_STANDALONE) -ggdb -DDEBUG -DRESOURCE_PATH='"res/"' -DNODE_DIR='"build/nodes"' -DPATCH_DIR='"$(HOME)/.waveforest/"' $^ -o $@

# LV2 plugin build

OBJS_LV2 = $(patsubst $(SRC_DIR)/lv2/%.c, $(BUILD_DIR)/lv2/%.so, $(wildcard $(SRC_DIR)/lv2/*.c))

.PHONY: lv2
lv2: $(BUILD_DIR) $(BUILD_DIR)/lv2 $(OBJS_LV2)

LV2_SRC = $(SRC) $(wildcard external/submodules/libxputty/xputty/*.c) $(wildcard external/submodules/libxputty/xputty/xdgmime/*.c)
LV2_PACKAGES = $(shell pkg-config --libs cairo x11)
LV2_INCLUDE = -Iexternal/submodules/libxputty/libxputty/include/ -I/usr/include/cairo
CFLAGS_LV2 = $(CFLAGS_SHARED) $(LV2_PACKAGES) $(LV2_INCLUDE) -shared -pipe -DPIC 

$(BUILD_DIR)/lv2/%.so: $(SRC_DIR)/lv2/%.c $(LV2_SRC)
	$(CC) $(CFLAGS_LV2) $^ -o $@

lv2_install: lv2
	install -d /usr/lib/lv2/waveforest.lv2
	install -m 644 $(BUILD_DIR)/lv2/*.so  /usr/lib/lv2/waveforest.lv2/
	install -m 644 $(SRC_DIR)/lv2/*.ttl /usr/lib/lv2/waveforest.lv2/

install: for_installation
	cp res /usr/share/waveforest -r
	cp $(BUILD_DIR)/nodes /usr/share/waveforest/nodes -r
	cp $(BUILD_DIR)/for_installation /usr/bin/$(NAME)

# Node build

OBJS_NODES = $(patsubst $(SRC_DIR_NODES)/%.c, $(BUILD_DIR)/nodes/%.so, $(wildcard $(SRC_DIR_NODES)/*.c))

# External sources for nodes
NODES_SRC = external/src/string_vector.c

nodes: $(BUILD_DIR)/nodes $(OBJS_NODES)

$(BUILD_DIR)/nodes/%.o: $(SRC_DIR_NODES)/%.c
	@echo $(OBJS_NODES)
	$(CC) $(CFLAGS_NODES) -c $< -o $@

$(BUILD_DIR)/nodes/%.so: $(BUILD_DIR)/nodes/%.o
	$(CC) -shared $(NODES_SRC) $(NODES_INCLUDE) $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)


$(BUILD_DIR):
	mkdir -p $@
$(BUILD_DIR)/lv2:
	mkdir -p $@
$(BUILD_DIR)/nodes:
	mkdir -p $@

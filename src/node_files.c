#include "node_files.h"
#include "common.h"

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#define FUNCTION_NAME_INSTANTIATE "node_instantiate"
#define FUNCTION_NAME_PROCESS "node_process"
#define FUNCTION_NAME_FREE "node_free"
#define FUNCTION_NAME_GUI "node_gui"
#define FUNCTION_NAME_CUSTOM_DATA "node_custom_data"

static char node_directory[PATH_MAX + 2] = {0};

static inline void *get_symbol(void *handle, const char *name) {
    void *symbol_func = dlsym(handle, name);

    char *error = dlerror();
    if (error) {
        ERROR("cannot fetch shared object symbol '%s'", name);
        return 0;
    }
    return symbol_func;
}

StringVector node_files_enumerate(void) {
    StringVector vec = stringvec_init();
    get_basenames_with_suffix(node_directory, &vec, ".so");
    return vec;
}

int node_files_load(char *node_name, Node *out_node) {
    assert(out_node);
    assert(node_name);

    Node node = {0};
    strncpy(node.name, node_name, MAX_NAME - 1);

    char path[PATH_MAX] = {0};
    strcpy(path, node_directory);
    strcat(path, node_name);
    strcat(path, ".so");

    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        ERROR("cannot open shared object file %s", dlerror());
        return 1;
    }
    dlerror();

    // Get symbols from shared object
    node.functions.instantiate =
        (NodeInstantiateFunction)get_symbol(handle, FUNCTION_NAME_INSTANTIATE);
    node.functions.process =
        (NodeProcessFunction)get_symbol(handle, FUNCTION_NAME_PROCESS);

    if (!node.functions.instantiate || !node.functions.process) {
        ERROR("Could not load node '%s' symbols", node_name);
        HINT("Have you implemented all the required functions in "
             "nodes/common_header.h?");
        return 1;
    }

    node.functions.free =
        (NodeFreeFunction)get_symbol(handle, FUNCTION_NAME_FREE);
    node.functions.gui = (NodeGuiFunction)get_symbol(handle, FUNCTION_NAME_GUI);
    node.functions.custom_data =
        (NodeCustomDataFunction)get_symbol(handle, FUNCTION_NAME_CUSTOM_DATA);

    memcpy(out_node, &node, sizeof(Node));
    return 0;
}

void node_files_set_directory(const char *directory) {
    strncpy(node_directory, directory, PATH_MAX);
    uint64_t length = strlen(node_directory);
    if (node_directory[length - 1] != '/') {
        node_directory[length] = '/';
        node_directory[length + 1] = 0;
    } else {
        node_directory[length] = 0;
    }
}

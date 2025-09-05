#ifndef _NODE_FILES
#define _NODE_FILES

#include "node_manager.h"
#include "string_vector.h"

// Returns a StringVector containing all names of the .so files in
// `directory`.
StringVector node_files_enumerate(void);
// Loads a node file (.so) and returns it via `out_node`.
// Returns non-zero value on error.
int node_files_load(char *node_name, Node *out_node);
// Set the directory from which node files will be searched.
void node_files_set_directory(const char *directory);

#endif

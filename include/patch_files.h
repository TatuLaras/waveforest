#ifndef _PATCH_FILES
#define _PATCH_FILES

#include "string_vector.h"
int patch_write(const char *file);
int patch_read(const char *file);

void patch_set_directory(const char *directory);
int patch_save(const char *name);
int patch_load(const char *name);

// Returns a StringVector containing all names of the .waveforest files in
// directory configured with patch_set_directory().
StringVector patch_files_enumerate(void);

#endif

#ifndef _COMMON
#define _COMMON

#define LOG_SRC "waveforest: "
#include "log.h"

#ifdef LSP_EDITOR
#define RESOURCE_PATH ""
#endif

#include "string_vector.h"

#define max(a, b) (((a) > b) ? a : b)
#define min(a, b) (((a) < b) ? a : b)
#define clamp(val, a, b) min(max(val, a), b)

// Inserts a list of filenames (without extension) of all files in `directory`
// that end with `suffix` into the StringVector `destination`.
void get_basenames_with_suffix(const char *directory, StringVector *destination,
                               const char *suffix);

#endif

#include "common.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

static inline int has_suffix(const char *string, const char *suffix) {
    size_t string_len = strlen(string);
    size_t suffix_len = strlen(suffix);
    if (string_len < suffix_len) {
        return 0;
    }
    return memcmp(string + (string_len - suffix_len), suffix, suffix_len) == 0;
}

void get_basenames_with_suffix(const char *directory, StringVector *destination,
                               const char *suffix) {
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (!d) {
        perror("Could not open directory");
        return;
    }

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG && has_suffix(dir->d_name, suffix))
            stringvec_append(destination, dir->d_name,
                             strlen(dir->d_name) - strlen(suffix));
    }

    closedir(d);

    return;
}

#ifndef _GUI_COMMON
#define _GUI_COMMON

#include "clay.h"

int is_mouse_inside_box(Clay_BoundingBox box, float *out_percentage_x,
                        float *out_percentage_y);
Clay_ElementId unique_id(char *prefix, uint64_t uid);

// Allows the user to type into a text buffer. Returns 1 if buffer was modified.
int type_into_buffer(char *buf_chars, uint32_t *buf_length, uint32_t buf_max);

#endif

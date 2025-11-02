#ifndef _DROPDOWN
#define _DROPDOWN

#include "string_vector.h"

void component_dropdown(StringVector *options, uint16_t *selected_index);

// Call this once at the end of every frame.
void component_dropdown_end_of_frame(void);

#endif

#ifndef _FUZZY_PICKER
#define _FUZZY_PICKER

#include "string_vector.h"

typedef void (*OptionSelectedCallback)(char *name);

#define MAX_RESULTS 10

// Ownership of `options` will be transfered into this translation unit.
void fuzzy_picker_start_picking(const char *text, StringVector options,
                                OptionSelectedCallback selected_callback);
// Returns 1 if the picker is currently active on screen.
int fuzzy_picker_render(void);

// Handle keyboard inputs for the picker. Returns 1 if it should capture all
// inputs, that is no shortcuts should be processed.
int fuzzy_picker_handle_inputs(void);

void fuzzy_picker_deinit(void);

#endif

#ifndef _COMMON_HEADER
#define _COMMON_HEADER

#include "common_node_types.h"

// It is advised to get input buffer values through this macro.
// This is in order to support slider-provided manual values without needlessly
// filling a whole buffer with the same value.
#define INPUT_GET_VALUE(buffer, i) (buffer.data ? buffer.data[i] : buffer.value)

// ----- Mandatory -----

// Instantiate the node instance here. The value returned here will be given to
// other functions as the arg parameter. The desired height in grid units should
// be written into `out_height_in_grid_units`. Functions for registering input
// and output ports are provided, as well as a pointer to a custom data block
// written into by node_custom_data() of size MAX_CUSTOM_DATA.
void *node_instantiate(NodeInstanceHandle handle,
                       uint8_t *out_height_in_grid_units,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output,
                       uint8_t *custom_data);

// Your process function called for each cycle of DSP processing.
void node_process(void *arg, Info *info, InputBuffer *input_buffers,
                  float **output_buffers, uint32_t frame_count);

// ----- Optional -----

// Uninitialize node instance here.
void node_free(void *arg);

// Callback for custom GUI. The `draw` struct contains function pointers for
// drawing different types of UI elements. See the Draw struct for more details.
void node_gui(void *arg, Draw draw);

// Called usually on patch save, allowing you to write custom data into
// `out_custom_data` that you can later access through node_instantiate().
// Size of the custom data block is defined by the constant MAX_CUSTOM_DATA.
void node_custom_data(void *arg, uint8_t *out_custom_data);

#endif

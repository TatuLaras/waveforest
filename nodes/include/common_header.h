#ifndef _COMMON_HEADER
#define _COMMON_HEADER

#include "common_node_types.h"

#define INPUT_GET_VALUE(buffer, i) (buffer.data ? buffer.data[i] : buffer.value)

void *node_instantiate(NodeInstanceHandle handle,
                       uint8_t *out_height_in_grid_units,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output,
                       uint8_t *custom_data);

void node_process(void *arg, Info *info, InputBuffer *input_buffers,
                  float **output_buffers, uint32_t frame_count);

// Optional
void node_free(void *arg);
void node_gui(void *arg, Draw draw);
void node_custom_data(void *arg, uint8_t *out_custom_data);

#endif

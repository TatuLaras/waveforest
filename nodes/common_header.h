#ifndef _COMMON_HEADER
#define _COMMON_HEADER

#include "common_node_types.h"

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output);
void *node_free(NodeInstanceHandle handle);

void node_process(void *arg, Info *info, float **input_buffers,
                  float **output_buffers, uint32_t frame_count);

#endif

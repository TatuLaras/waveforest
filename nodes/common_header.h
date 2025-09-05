#ifndef _COMMON_HEADER
#define _COMMON_HEADER

#include "common_node_types.h"

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterPortFunction register_port);
void *node_free(NodeInstanceHandle handle);

void node_process(float **input_buffers, float **output_buffers,
                  uint32_t buffer_size, uint32_t input_buffers_count,
                  uint32_t output_buffers_count, void *arg);

#endif

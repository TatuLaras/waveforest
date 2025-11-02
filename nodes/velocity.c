#include "common_header.h"

#include <stdlib.h>

static OutputPortHandle out;

void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    out = register_output(handle, "output");

    *out_height = 1;
    return 0;
    (void)register_input;
}

void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    for (uint32_t i = 0; i < frame_count; i++)
        output_bufs[out][i] = (float)info->note->on_velocity / 128;

    (void)arg;
    (void)input_bufs;
}

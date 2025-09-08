#include "common_header.h"
#include <stdlib.h>

typedef struct {
    float output_value;
} State;

static PortHandle output = 0;

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    output = (*register_output)(handle, "output");

    return calloc(1, sizeof(State));
    (void)register_input;
}

void node_process(void *arg, Info *info, float **input_buffers,
                  float **output_buffers, uint32_t frame_count) {

    State *state = (State *)arg;

    for (uint32_t frame_i = 0; frame_i < frame_count; frame_i++) {
        output_buffers[output][frame_i] = state->output_value;
    }

    (void)input_buffers;
    (void)info;
}

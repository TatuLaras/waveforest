#include "common_header.h"
#include <stdio.h>

static OutputPortHandle out;
static InputPortHandle in[INPUT_COUNT] = {0};

void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output,
                       uint8_t *custom_data) {

    out = register_output(handle, "output");

    for (uint16_t i = 0; i < INPUT_COUNT; i++) {
        InputPort port = {.manual = {.default_value = 0, .min = 0, .max = 10}};
        snprintf(port.name, sizeof(port.name) - 1, "input %u", i);
        in[i] = register_input(handle, port);
    }

    *out_height = NODE_HEIGHT;
    return 0;
    (void)custom_data;
}

void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    for (uint32_t frame_i = 0; frame_i < frame_count; frame_i++) {
        float value = 1;
        for (uint32_t i = 0; i < INPUT_COUNT; i++)
            value *= INPUT_GET_VALUE(input_bufs[i], frame_i);

        output_bufs[out][frame_i] = value;
    }

    (void)arg;
    (void)info;
}

#include "common_header.h"

static OutputPortHandle out;
static InputPortHandle in;

void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    out = register_output(handle, "output");

    in = register_input(
        handle,
        (InputPort){.name = "input",
                    .manual = {.default_value = 0, .min = 0, .max = 1}});

    *out_height = 1;
    return 0;
}

void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    for (uint32_t i = 0; i < frame_count; i++)
        output_bufs[out][i] = INPUT_GET_VALUE(input_bufs[in], i);

    (void)arg;
    (void)info;
}

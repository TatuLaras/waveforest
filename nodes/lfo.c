#include "common_header.h"

#include <math.h>
#include <stdlib.h>

static OutputPortHandle out;
static InputPortHandle in_frequency;
static InputPortHandle in_phase;
static InputPortHandle in_volume;

void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output,
                       uint8_t *custom_data) {

    out = (*register_output)(handle, "output");

    in_frequency = (*register_input)(
        handle,
        (InputPort){.name = "frequency",
                    .manual = {.default_value = 1, .min = 0, .max = 50}});
    in_phase = (*register_input)(
        handle,
        (InputPort){.name = "phase",
                    .manual = {.default_value = 0, .min = 0, .max = 8}});
    in_volume = (*register_input)(
        handle,
        (InputPort){.name = "volume",
                    .manual = {.default_value = 1, .min = 0, .max = 4}});

    *out_height = 1;
    return 0;
    (void)custom_data;
}

void node_free(void *arg) {
    free(arg);
}

void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    for (uint32_t i = 0; i < frame_count; i++) {
        double time =
            (double)(info->coarse_time + i) / (double)info->sample_rate;

        output_bufs[out][i] =
            sin(time * M_PI * 2 * INPUT_GET_VALUE(input_bufs[in_frequency], i) +
                INPUT_GET_VALUE(input_bufs[in_phase], i)) *
            INPUT_GET_VALUE(input_bufs[in_volume], i);
    }

    (void)arg;
}

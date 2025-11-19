#include "common_header.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum : uint16_t {
    MODE_SINE,
    MODE_TRIANGLE,
    MODE_SAW,
    MODE_SQUARE,
} Mode;

typedef struct {
    StringVector modes;
    Mode selected_mode;
} State;

static OutputPortHandle out;
static InputPortHandle in_frequency;
static InputPortHandle in_phase;
static InputPortHandle in_volume;

void *node_instantiate(NodeInstanceHandle handle,
                       uint8_t *out_height_in_grid_units,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    out = (*register_output)(handle, "output");

    in_frequency = register_input(
        handle,
        (InputPort){.name = "frequency",
                    .manual = {.default_value = 1, .min = 0, .max = 20}});
    in_phase = register_input(
        handle,
        (InputPort){.name = "phase",
                    .manual = {.default_value = 0, .min = 0, .max = 8}});
    in_volume = register_input(
        handle,
        (InputPort){.name = "volume",
                    .manual = {.default_value = 1, .min = 0, .max = 4}});

    void *arg = malloc(sizeof(State));

    State state = {.modes = stringvec_init()};
    stringvec_append(&state.modes, "sine", strlen("sine"));
    stringvec_append(&state.modes, "triangle", strlen("triangle"));
    stringvec_append(&state.modes, "saw", strlen("saw"));
    stringvec_append(&state.modes, "square", strlen("square"));

    memcpy(arg, &state, sizeof(State));

    *out_height_in_grid_units = 2;

    return arg;
}

void node_free(void *arg) {
    State *state = (State *)arg;

    if (state->modes.data)
        stringvec_free(&state->modes);

    free(arg);
}

void node_gui(void *arg, Draw draw) {
    State *state = (State *)arg;

    draw.dropdown(&state->modes, &state->selected_mode);
}

static inline float wave_function(Mode mode, double x) {
    switch (mode) {
    case MODE_SINE:
        return sin(x * M_PI * 2);
    case MODE_TRIANGLE:
        return fabs(fmod(x, 1.0) * 2 - 1) * 2 - 1;
    case MODE_SAW:
        return fmod(x, 1.0) * 2 - 1;
    case MODE_SQUARE:
        return ((uint64_t)(x * 2) % 2 == 0) * 2 - 1;
    }

    return 0;
}

void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    State *state = (State *)arg;

    for (uint32_t i = 0; i < frame_count; i++) {

        double time =
            (double)(info->coarse_time + i) / (double)info->sample_rate;

        output_bufs[out][i] =
            wave_function(state->selected_mode,
                          time * info->note->frequency *
                                  INPUT_GET_VALUE(input_bufs[in_frequency], i) +
                              INPUT_GET_VALUE(input_bufs[in_phase], i)) *
            INPUT_GET_VALUE(input_bufs[in_volume], i);
    }
}

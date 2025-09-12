#include "common_header.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t use_exact_frequency;
} State;

// Can be shared across instances because registered port handles are always
// deterministic if registered in the same order.
static OutputPortHandle out;
static InputPortHandle in_frequency;
static InputPortHandle in_phase;
static InputPortHandle in_volume;
static float note_frequencies[MIDI_NOTE_COUNT] = {0};

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    out = (*register_output)(handle, "output");

    in_frequency = (*register_input)(
        handle,
        (InputPort){.name = "frequency",
                    .manual = {.default_value = 1, .min = 0, .max = 20}});
    in_phase = (*register_input)(
        handle,
        (InputPort){.name = "phase",
                    .manual = {.default_value = 0, .min = 0, .max = 8}});
    in_volume = (*register_input)(
        handle,
        (InputPort){.name = "volume",
                    .manual = {.default_value = 1, .min = 0, .max = 4}});

    for (uint32_t i = 0; i < 128; i++) {
        note_frequencies[i] = 440 * pow(2, (i - 69.0) / 12.0);
    }

    return calloc(1, sizeof(State));
}

void node_process(void *arg, Info *info, float **input_buffers,
                  float **output_buffers, uint32_t frame_count) {

    State *state = (State *)arg;

    for (uint32_t i = 0; i < frame_count; i++) {
        double time =
            (double)(info->coarse_time + i) / (double)info->sample_rate;

        // if (info->note.is_on)
        output_buffers[out][i] =
            sin(time * M_PI * 2 * note_frequencies[info->note.note] *
                    input_buffers[in_frequency][i] +
                input_buffers[in_phase][i]) *
            input_buffers[in_volume][i];
        // else
        //     output_buffers[out][i] = 0;
        //
        // float value = 0;
        // for (uint32_t note_i = 0; note_i < MIDI_NOTE_COUNT; note_i++) {
        //
        //     if (!info->midi_note_on[note_i])
        //         continue;
        //
        //     value += sin(state->time * M_PI * 2 * note_frequencies[note_i] *
        //                      input_buffers[in_frequency][i] +
        //                  input_buffers[in_phase][i]) *
        //              input_buffers[in_volume][i];
        // }
        // output_buffers[out][i] = value;
        // if (on_note_count)
        //     output_buffers[out][i] = value / on_note_count;
        // else
        //     output_buffers[out][i] = 0;
    }
}

#include "common_header.h"

#include <math.h>
#include <stdlib.h>

typedef struct {
    uint8_t use_exact_frequency;
    double time;
    uint8_t note_on[MIDI_NOTE_COUNT];
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
    in_frequency =
        (*register_input)(handle, (InputPort){.name = "frequency",
                                              .manual = {
                                                  .default_value = 1.0,
                                                  .min = 0,
                                                  .max = 20,
                                              }});
    in_phase = (*register_input)(handle, (InputPort){.name = "phase",
                                                     .manual = {
                                                         .default_value = 0.0,
                                                         .min = 0,
                                                         .max = 8,
                                                     }});
    in_volume = (*register_input)(handle, (InputPort){.name = "volume",
                                                      .manual = {
                                                          .default_value = 1,
                                                          .min = 0,
                                                          .max = 4,
                                                      }});

    for (uint32_t i = 0; i < 128; i++)
        note_frequencies[i] = 440 * pow(2, (i - 69) / 12.0);

    return calloc(1, sizeof(State));
}

void node_process(void *arg, Info *info, float **input_buffers,
                  float **output_buffers, uint32_t frame_count) {

    State *state = (State *)arg;

    uint32_t midi_event_i = 0;

    for (uint32_t i = 0; i < frame_count; i++) {

        if (midi_event_i < info->midi_event_count &&
            info->midi_events[midi_event_i].time == i) {

            MidiEvent *event = info->midi_events + midi_event_i;

            if (event->type == MIDI_NOTE_ON) {
                state->note_on[event->note] = 1;
                // printf("NOTEON %u\n", event->note);
            } else if (event->type == MIDI_NOTE_OFF)
                state->note_on[event->note] = 0;

            midi_event_i++;
        }

        state->time += 1.0 / (double)info->sample_rate;

        // memset(output_buffers[out], 0, frame_count * sizeof(float));
        output_buffers[out][i] =
            sin(state->time * M_PI * 2 * 220 * input_buffers[in_frequency][i] +
                input_buffers[in_phase][i]) *
            input_buffers[in_volume][i];

        // for (uint32_t note_i = 0; note_i < MIDI_NOTE_COUNT; note_i++) {
        //
        //     if (!state->note_on[note_i])
        //         continue;
        //
        //     output_buffers[out][i] += sin(state->time * M_PI * 2 * 220 *
        //                                       input_buffers[in_frequency][i]
        //                                       +
        //                                   input_buffers[in_phase][i]) *
        //                               input_buffers[in_volume][i];
        // }
    }
}

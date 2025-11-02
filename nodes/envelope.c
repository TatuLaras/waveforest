#include "common.h"
#include "common_header.h"

#include <stdlib.h>

typedef enum : uint8_t {
    STAGE_RELEASE = 0,
    STAGE_ATTACK,
    STAGE_DECAY,
    STAGE_SUSTAIN,
} Stage;

typedef struct {
    Stage stage;
    float value;
    float slope;
} Envelope;

typedef struct {
    Envelope envelopes[MIDI_NOTES];
} State;

static OutputPortHandle out;
static InputPortHandle in_attack;
static InputPortHandle in_decay;
static InputPortHandle in_sustain;
static InputPortHandle in_release;

static inline float calculate_slope(float from, float to, float time,
                                    float time_step) {
    float zero_to_one = time_step / max(time, 0.0001);
    float distance = to - from;
    return zero_to_one * distance;
}

void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output) {

    out = register_output(handle, "output");

    in_attack = register_input(
        handle,
        (InputPort){.name = "attack",
                    .manual = {.default_value = 0.001, .min = 0, .max = 15}});
    in_decay = register_input(
        handle,
        (InputPort){.name = "decay",
                    .manual = {.default_value = 0.001, .min = 0, .max = 15}});
    in_sustain = register_input(
        handle,
        (InputPort){.name = "sustain",
                    .manual = {.default_value = 1, .min = 0, .max = 1}});
    in_release = register_input(
        handle, (InputPort){.name = "release",
                            .manual = {.default_value = 0.001,
                                       .min = 0,
                                       .max = MAX_RELEASE_TIME}});

    *out_height = 2;
    return calloc(1, sizeof(State));
}

void node_free(void *arg) {
    free(arg);
}

void node_process(void *arg, Info *info, InputBuffer *input_buffers,
                  float **output_buffers, uint32_t frame_count) {

    State *state = (State *)arg;
    Envelope *envelope = state->envelopes + info->note->number;
    float time_step = 1.0 / info->sample_rate;

    for (uint32_t i = 0; i < frame_count; i++) {

        uint8_t is_on = info->note->is_on;
        is_on = is_on && info->coarse_time + i >= info->note->enable_time;
        is_on = is_on && (info->note->release_time < info->note->enable_time ||
                          info->coarse_time + i <= info->note->release_time);

        float attack = INPUT_GET_VALUE(input_buffers[in_attack], i);
        float decay = INPUT_GET_VALUE(input_buffers[in_decay], i);
        float sustain = INPUT_GET_VALUE(input_buffers[in_sustain], i);
        float release = INPUT_GET_VALUE(input_buffers[in_release], i);

        if (!is_on && envelope->stage != STAGE_RELEASE) {
            envelope->slope =
                calculate_slope(envelope->value, 0, release, time_step);
            envelope->stage = STAGE_RELEASE;
        }

        switch (envelope->stage) {

        case STAGE_RELEASE: {
            if (is_on) {
                envelope->slope =
                    calculate_slope(envelope->value, 1, attack, time_step);
                envelope->stage = STAGE_ATTACK;
            } else {
                envelope->value += envelope->slope;
            }
        } break;

        case STAGE_ATTACK: {
            envelope->value += envelope->slope;
            if (envelope->value >= 1.0) {
                envelope->slope = calculate_slope(1, sustain, decay, time_step);
                envelope->stage = STAGE_DECAY;
            }
        } break;

        case STAGE_DECAY: {
            envelope->value += envelope->slope;
            if (envelope->value <= sustain)
                envelope->stage = STAGE_SUSTAIN;

        } break;

        case STAGE_SUSTAIN: {
            envelope->value = sustain;
        } break;
        }

        envelope->value = clamp(envelope->value, 0, 1);
        output_buffers[out][i] = envelope->value;
    }
}

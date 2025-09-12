#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include <lv2/core/lv2.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define WAVEFOREST_URI "http://www.laras.cc/plugins/waveforest"

typedef enum {
    WAVEFOREST_CONTROL = 0,
    WAVEFOREST_INPUT = 1,
    WAVEFOREST_OUTPUT = 2,
    WAVEFOREST_MACRO_1 = 3,
} PortIndex;

typedef struct {
    // Port buffers
    const LV2_Atom_Sequence *control;
    const float *in;
    float *out;

    // Features
    LV2_URID_Map *map;
    LV2_Log_Logger logger;

    struct {
        LV2_URID midi_MidiEvent;
    } uris;
} Midigate;

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {
    Midigate *self = (Midigate *)calloc(1, sizeof(Midigate));
    if (!self) {
        return NULL;
    }

    // Scan host features for URID map
    // clang-format off
  const char* missing = lv2_features_query(
    features,
    LV2_LOG__log,  &self->logger.log, false,
    LV2_URID__map, &self->map,        true,
    NULL);
    // clang-format on

    lv2_log_logger_set_map(&self->logger, self->map);
    if (missing) {
        lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
        free(self);
        return NULL;
    }

    self->uris.midi_MidiEvent =
        self->map->map(self->map->handle, LV2_MIDI__MidiEvent);

    core_init(rate);
    lv2_log_note(&self->logger, "Initialized WaveForest with sample rate %f\n",
                 rate);

    return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Midigate *self = (Midigate *)instance;

    switch ((PortIndex)port) {
    case WAVEFOREST_CONTROL:
        self->control = (const LV2_Atom_Sequence *)data;
        break;
    case WAVEFOREST_INPUT:
        self->in = (const float *)data;
        break;
    case WAVEFOREST_OUTPUT:
        self->out = (float *)data;
        break;
    case WAVEFOREST_MACRO_1:
        break;
    }
}

static void activate(LV2_Handle instance) {
    core_send_midi_event((MidiEvent){.type = MIDI_ALL_NOTES_OFF});
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    Midigate *self = (Midigate *)instance;
    uint32_t offset = 0;

    for (LV2_Atom_Event *ev = lv2_atom_sequence_begin(&(self->control)->body);
         !lv2_atom_sequence_is_end(&(self->control)->body,
                                   (self->control)->atom.size, (ev));
         (ev) = lv2_atom_sequence_next(ev)) {

        if (ev->body.type != self->uris.midi_MidiEvent)
            continue;

        const uint8_t *const msg = (const uint8_t *)(ev + 1);
        switch (lv2_midi_message_type(msg)) {
        case LV2_MIDI_MSG_NOTE_ON:
            core_send_midi_event((MidiEvent){
                .type = MIDI_NOTE_ON, .note = msg[1] & 0x7f, .velocity = 128});
            break;
        case LV2_MIDI_MSG_NOTE_OFF:
            core_send_midi_event(
                (MidiEvent){.type = MIDI_NOTE_OFF, .note = msg[1] & 0x7f});
            break;
        case LV2_MIDI_MSG_CONTROLLER:
            if (msg[1] == LV2_MIDI_CTL_ALL_NOTES_OFF) {
                core_send_midi_event((MidiEvent){.type = MIDI_ALL_NOTES_OFF});
            }
            break;
        default:
            break;
        }

        core_get_frames(self->out + offset, ev->time.frames - offset);
        offset = (uint32_t)ev->time.frames;
    }

    core_get_frames(self->out + offset, sample_count - offset);
}

static void deactivate(LV2_Handle instance) {}

static void cleanup(LV2_Handle instance) {
    free(instance);
}
static const void *extension_data(const char *uri) {
    return NULL;
}

static const LV2_Descriptor descriptor = {
    WAVEFOREST_URI, instantiate, connect_port,  activate, run,
    deactivate,     cleanup,     extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}

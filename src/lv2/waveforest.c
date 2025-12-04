#include <linux/limits.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>

#include "patch_files.h"
#include "uris.h"

#include <stdbool.h>
#include <stdlib.h>

#include "midi.h"
#include "node_files.h"
#include "node_manager.h"

#define WAVEFOREST_VERSION 8

typedef enum {
    PORT_CONTROL = 0,
    PORT_MIDI = 1,
    PORT_OUT = 2,
} PortIndex;

typedef struct {
    // Port buffers
    const LV2_Atom_Sequence *midi_in;
    const LV2_Atom_Sequence *control_in;
    float *out;

    // Features
    LV2_URID_Map *map;
    LV2_Log_Logger logger;
    LV2_Worker_Schedule *schedule;

    char path[PATH_MAX + 1];

    URIs uris;
} State;

static inline const LV2_Atom *
parse_set_file_message(const URIs *uris, const LV2_Atom_Object *obj) {
    if (obj->body.otype != uris->patch_Set) {
        fprintf(stderr, "Ignoring unknown message type %d\n", obj->body.otype);
        return NULL;
    }

    /* Get property URI. */
    const LV2_Atom *property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property) {
        fprintf(stderr, "Malformed set message has no body.\n");
        return NULL;
    } else if (property->type != uris->atom_URID) {
        fprintf(stderr, "Malformed set message has non-URID property.\n");
        return NULL;
    } else if (((LV2_Atom_URID *)property)->body != uris->wf_patch) {
        fprintf(stderr, "Set message for unknown property.\n");
        return NULL;
    }

    /* Get value. */
    const LV2_Atom *file_path = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &file_path, 0);
    if (!file_path) {
        fprintf(stderr, "Malformed set message has no value.\n");
        return NULL;
    } else if (file_path->type != uris->atom_Path) {
        fprintf(stderr, "Set message value is not a Path.\n");
        return NULL;
    }

    return file_path;
}

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {

    State *state = (State *)calloc(1, sizeof(State));

    // clang-format off
    const char *missing =
        lv2_features_query(features,
                           LV2_LOG__log, &state->logger.log, 0,
                           LV2_URID__map, &state->map, 1, 
                           LV2_WORKER__schedule, &state->schedule, 1,
                           0);
    // clang-format on

    lv2_log_logger_set_map(&state->logger, state->map);
    if (missing) {
        lv2_log_error(&state->logger, "Missing feature <%s>\n", missing);
        free(state);
        return NULL;
    }

    map_uris(state->map, &state->uris);

    //  TODO: Proper way to handle node plugins, like a directory in /usr/lib/
    //  or something.
    node_files_set_directory("/home/tatu/_repos/waveforest/build/nodes/");
    node_init();

    lv2_log_note(&state->logger, "Initialized WaveForest version %u",
                 WAVEFOREST_VERSION);
    return (LV2_Handle)state;
}

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    State *self = (State *)instance;

    switch ((PortIndex)port) {
    case PORT_CONTROL:
        self->control_in = (LV2_Atom_Sequence *)data;
        break;
    case PORT_OUT:
        self->out = (float *)data;
        break;
    case PORT_MIDI:
        self->midi_in = (LV2_Atom_Sequence *)data;
        break;
    }
}

static void activate(LV2_Handle instance) {}

static void run(LV2_Handle instance, uint32_t n_samples) {
    const State *self = (State *)instance;

    // Patch file load message handling

    LV2_ATOM_SEQUENCE_FOREACH(self->control_in, ev) {

        LV2_Atom_Object *obj = (LV2_Atom_Object *)&ev->body;
        if (obj->body.otype != self->uris.patch_Set)
            continue;

        // Parse "new file path" message and load file in a worker thread
        self->schedule->schedule_work(
            self->schedule->handle, lv2_atom_total_size(&ev->body), &ev->body);
    }

    // Read MIDI events

    LV2_ATOM_SEQUENCE_FOREACH(self->midi_in, ev) {
        if (ev->body.type != self->uris.midi_MidiEvent)
            continue;

        const uint8_t *const msg = (const uint8_t *)(ev + 1);
        switch (lv2_midi_message_type(msg)) {
        case LV2_MIDI_MSG_NOTE_ON:
            midi_set_note_on(msg[1], msg[2], ev->time.frames);
            break;
        case LV2_MIDI_MSG_NOTE_OFF:
            midi_set_note_off(msg[1], msg[2], ev->time.frames);
            break;
        case LV2_MIDI_MSG_CONTROLLER:
            if (msg[1] == LV2_MIDI_CTL_ALL_NOTES_OFF)
                midi_reset();

            break;
        default:
            break;
        }
    }

    // Produce audio output
    node_consume_network(n_samples, self->out);
}

//  NOTE: Will crash if patch file is swapped when any MIDI note is held down
//  due to a new node network being loaded mid-processing. Maybe this is
//  something we want to do in run() altough it will produce a harsh sound due
//  to it not being RT-safe.
static LV2_Worker_Status work(LV2_Handle instance,
                              LV2_Worker_Respond_Function respond,
                              LV2_Worker_Respond_Handle handle, uint32_t size,
                              const void *data) {

    State *self = (State *)instance;

    const LV2_Atom_Object *obj = (const LV2_Atom_Object *)data;

    const LV2_Atom *file_path = parse_set_file_message(&self->uris, obj);
    if (!file_path || file_path->size == 0 || file_path->size >= PATH_MAX)
        return LV2_WORKER_ERR_UNKNOWN;

    strncpy(self->path, (char *)(file_path + 1), file_path->size);
    self->path[file_path->size] = '\0';

    patch_read(self->path);
    lv2_log_note(&self->logger, "Loaded WaveForest patch %s", self->path);

    return LV2_WORKER_SUCCESS;
}

static LV2_State_Status save(LV2_Handle instance,
                             LV2_State_Store_Function store,
                             LV2_State_Handle handle, uint32_t flags,
                             const LV2_Feature *const *features) {

    State *self = (State *)instance;

    store(handle, self->uris.wf_patch, self->path, strlen(self->path) + 1,
          self->uris.atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    return LV2_STATE_SUCCESS;
}

static LV2_State_Status restore(LV2_Handle instance,
                                LV2_State_Retrieve_Function retrieve,
                                LV2_State_Handle handle, uint32_t flags,
                                const LV2_Feature *const *features) {

    State *self = (State *)instance;

    const char *path = retrieve(handle, self->uris.wf_patch, 0, 0, 0);
    if (!path)
        return LV2_STATE_ERR_UNKNOWN;

    strncpy(self->path, path, PATH_MAX);
    patch_read(self->path);

    lv2_log_note(&self->logger, "Loaded WaveForest patch %s", self->path);

    return LV2_STATE_SUCCESS;
}

static void deactivate(LV2_Handle instance) {}

static void cleanup(LV2_Handle instance) {

    free(instance);
}

static const void *extension_data(const char *uri) {

    static const LV2_Worker_Interface worker = {work, 0, 0};
    static const LV2_State_Interface state = {save, restore};

    if (!strcmp(uri, LV2_WORKER__interface))
        return &worker;
    else if (!strcmp(uri, LV2_STATE__interface))
        return &state;

    return NULL;
}

static const LV2_Descriptor descriptor = {
    WAVEFOREST_URI, instantiate, connect_port,  activate, run,
    deactivate,     cleanup,     extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    switch (index) {
    case 0:
        return &descriptor;
    default:
        return NULL;
    }
}

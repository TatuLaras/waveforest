/*
 * Collection of node-related types so they are also available for node source
 * files via including this file in nodes/common_header.h.
 */
#ifndef _COMMON_NODE_TYPES
#define _COMMON_NODE_TYPES

#include "string_vector.h"
#include <stdint.h>

#define MAX_NAME 128
#define MAX_CUSTOM_DATA 1024

// Nodes will for a MIDI key be processed for this amount of time after the MIDI
// key release. This is to allow for stuff like a release envelope fade out.
#define MAX_RELEASE_TIME 15.0

typedef uint32_t NodeHandle;
typedef uint32_t NodeInstanceHandle;
typedef uint16_t PortHandle;
typedef PortHandle OutputPortHandle;
typedef PortHandle InputPortHandle;

typedef struct {
    char name[MAX_NAME];
    struct {
        NodeInstanceHandle output_instance;
        PortHandle output_port;
    } connection;
    // Used when nothing is connected to this port
    struct {
        float value;
        float default_value;
        float min;
        float max;
    } manual;
} InputPort;

typedef PortHandle (*RegisterInputPortFunction)(NodeInstanceHandle handle,
                                                InputPort port);
typedef PortHandle (*RegisterOutputPortFunction)(NodeInstanceHandle handle,
                                                 char *name);

typedef enum : uint8_t {
    MIDI_NOTE_ON,
    MIDI_NOTE_OFF,
    MIDI_ALL_NOTES_OFF,
} MidiEventType;

typedef struct {
    MidiEventType type;
    uint8_t note;
    uint8_t velocity;
    uint32_t time;
} MidiEvent;

#define MIDI_NOTES 128

typedef struct {
    uint8_t is_on;
    uint8_t number;
    uint8_t on_velocity;
    uint8_t off_velocity;
    float frequency;
    uint64_t release_time;
    uint64_t enable_time;
} NoteInfo;

typedef struct {
    float sample_rate;
    uint64_t coarse_time;
    NoteInfo *note;
    float bpm;
} Info;

typedef struct {
    float *data;
    float value;
} InputBuffer;

typedef void (*DrawDropdownFunction)(StringVector *options,
                                     uint16_t *selected_index);

typedef struct {
    DrawDropdownFunction dropdown;
} Draw;

#endif

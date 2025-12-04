#ifndef _MIDI
#define _MIDI

#include "common_node_types.h"
#include <stdint.h>

// Turn off all notes
void midi_reset(void);
void midi_event_send(uint8_t *msg, uint32_t time);
NoteInfo *midi_get_note_info(uint8_t note);

#endif

#ifndef _MIDI
#define _MIDI

#include "common_node_types.h"
#include <stdint.h>

// Turn off all notes
void midi_reset(void);
void midi_set_note_on(uint8_t note, uint8_t velocity, uint32_t time);
void midi_set_note_off(uint8_t note, uint8_t velocity, uint32_t time);
NoteInfo *midi_get_note_info(uint8_t note);

#endif

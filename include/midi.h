#ifndef _MIDI
#define _MIDI

#include <stdint.h>

#define MIDI_NOTES 128

extern uint8_t midi_note_on[MIDI_NOTES];
extern const float midi_note_frequencies[MIDI_NOTES];

#endif

#include "midi.h"
#include "common.h"
#include "node_manager.h"

#include <math.h>
#include <string.h>

VEC_DECLARE(uint8_t, NoteOffQueue, noteoffqueue)
VEC_IMPLEMENT(uint8_t, NoteOffQueue, noteoffqueue)

static NoteInfo note_data[MIDI_NOTES] = {0};
static NoteOffQueue note_off_queue = {0};
static uint8_t is_frequencies_calculated = 0;
static int is_sostenuto = 0;

static inline void calculate_frequencies(void) {

    for (uint32_t i = 0; i < MIDI_NOTES; i++) {
        note_data[i].frequency = 440 * pow(2, (i - 69.0) / 12.0);
        note_data[i].number = i;
    }
    is_frequencies_calculated = 1;
}

void midi_reset(void) {

    memset(note_data, 0, sizeof(note_data));
}

static void set_note_on(uint8_t note, uint8_t velocity, uint32_t time) {

    if (note >= MIDI_NOTES)
        return;
    if (!is_frequencies_calculated)
        calculate_frequencies();

    note_data[note].is_on = 1;
    note_data[note].on_velocity = velocity;
    note_data[note].enable_time = time + node_get_coarse_time();
}

static void set_note_off(uint8_t note, uint8_t velocity, uint32_t time) {

    if (note >= MIDI_NOTES)
        return;

    if (is_sostenuto) {
        noteoffqueue_append(&note_off_queue, note);
        return;
    }

    note_data[note].is_on = 0;
    note_data[note].off_velocity = velocity;
    note_data[note].release_time = time + node_get_coarse_time();
}

static void set_sostenuto(int is_on, uint32_t time) {

    if (!note_off_queue.data)
        note_off_queue = noteoffqueue_init();

    if (is_on) {
        is_sostenuto = 1;
        return;
    }

    is_sostenuto = 0;

    for (uint32_t i = 0; i < note_off_queue.data_used; i++)
        set_note_off(note_off_queue.data[i], 0x3f, 0);

    note_off_queue.data_used = 0;
}

void midi_event_send(uint8_t *msg, uint32_t time) {

    //  NOTE: Channel is ignored

    // Note on
    if ((msg[0] & 0xf0) == 0x90) {
        set_note_on(msg[1], msg[2], time);
        return;
    }

    // Note off
    if ((msg[0] & 0xf0) == 0x80) {
        set_note_off(msg[1], msg[2], time);
        return;
    }

    // Control Change
    if ((msg[0] & 0xf0) == 0xb0) {

        // Sustain / sostenuto pedal
        if (msg[1] == 64) {
            set_sostenuto(msg[2] > 0x3f, time);
            return;
        }

        return;
    }
}

NoteInfo *midi_get_note_info(uint8_t note) {
    if (note >= MIDI_NOTES)
        return 0;
    return note_data + note;
}

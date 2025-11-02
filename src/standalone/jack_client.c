#include "common.h"
#include "midi.h"
#include "node_manager.h"
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <jack/jack.h>
#include <jack/midiport.h>

static jack_port_t *in_midi;
static jack_port_t *out_audio;
static jack_client_t *client;

static int process(jack_nframes_t nframes, void *arg) {
    void *port_buf = jack_port_get_buffer(in_midi, nframes);
    jack_default_audio_sample_t *out =
        (jack_default_audio_sample_t *)jack_port_get_buffer(out_audio, nframes);
    jack_midi_event_t in_event;
    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

    // jack_midi_event_get(&in_event, port_buf, 0);

    for (uint32_t i = 0; i < event_count; i++) {
        jack_midi_event_get(&in_event, port_buf, i);

        //  NOTE: https://midi.org/summary-of-midi-1-0-messages

        // We don't care about channels currently

        if ((in_event.buffer[0] & 0xf0) == 0x90)
            midi_set_note_on(in_event.buffer[1], in_event.buffer[2],
                             in_event.time);
        else if ((in_event.buffer[0] & 0xf0) == 0x80)
            midi_set_note_off(in_event.buffer[1], in_event.buffer[2],
                              in_event.time);
    }

    node_consume_network(nframes, out);
    return 0;

    (void)arg;
}

static int sample_rate_changed(jack_nframes_t nframes, void *arg) {

    node_set_sample_rate(nframes);
    return 0;
    (void)arg;
}

int jack_init(void) {

    if ((client = jack_client_open("waveforest", JackNullOption, NULL)) == 0) {
        ERROR("JACK server not running");
        return 1;
    }

    node_set_sample_rate(jack_get_sample_rate(client));

    jack_set_process_callback(client, process, 0);
    jack_set_sample_rate_callback(client, sample_rate_changed, 0);

    in_midi = jack_port_register(client, "MIDI in", JACK_DEFAULT_MIDI_TYPE,
                                 JackPortIsInput, 0);
    out_audio = jack_port_register(
        client, "audio output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client)) {
        ERROR("cannot activate client");
        return 1;
    }

    // Try to auto-connect to a playback device
    const char **ports = jack_get_ports(client, NULL, NULL,
                                        JackPortIsPhysical | JackPortIsInput);
    const char **midi_ports = jack_get_ports(
        client, NULL, ".*midi.*", JackPortIsOutput | JackPortIsPhysical);

    if (ports && ports[0] &&
        jack_connect(client, jack_port_name(out_audio), ports[0]))
        WARNING("tried to connect to physical output port %s but couldn't",
                ports[0]);
    if (ports && ports[1] &&
        jack_connect(client, jack_port_name(out_audio), ports[1]))
        WARNING("tried to connect to physical output port %s but couldn't",
                ports[1]);

    if (midi_ports)
        for (uint32_t i = 0; midi_ports[i]; i++) {
            if (jack_connect(client, midi_ports[i], jack_port_name(in_midi)))
                WARNING("could not connect to MIDI port %s", midi_ports[i]);
        }

    jack_free(ports);
    jack_free(midi_ports);

    return 0;
}

void jack_deinit(void) {
    jack_client_close(client);
}

#include "gui/gui.h"
#include "jack_client.h"
#include "node_files.h"

#define LOG_SRC "waveforest: "
#include "log.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

void list_nodes(void) {
    StringVector nodes = node_files_enumerate();
    size_t i = 0;
    char *node_name;
    while ((node_name = stringvec_get(&nodes, i++))) {
        printf("node: %s\n", node_name);
    }
    stringvec_free(&nodes);
}

void cleanup(void) {
    printf("\n");
    INFO("cleaning up");
    gui_deinit();
    jack_deinit();
}

int main(int argc, char **argv) {
    node_files_set_directory("build/nodes/");

    signal(SIGQUIT, exit);
    signal(SIGHUP, exit);
    signal(SIGTERM, exit);
    signal(SIGINT, exit);
    atexit(cleanup);

    // list_nodes();

    Node node = {0};

    if (node_files_load("oscillator", &node))
        return 1;
    NodeHandle oscillator = node_new(node);

    // if (node_files_load("filter", &node))
    //     return 1;
    // NodeHandle filter = node_new(node);

    NodeInstanceHandle oscillator_instance = node_instantiate(oscillator);
    NodeInstanceHandle modulator_instance = node_instantiate(oscillator);

    InputPortHandle oscillator_freq = 0;
    InputPortHandle modulator_freq = 0;

    OutputPortHandle oscillator_out = 0;
    InputPortHandle oscillator_volume = 0;
    OutputPortHandle modulator_out = 0;

    if (node_get_output_handle(oscillator_instance, "output", &oscillator_out))
        return 1;
    if (node_get_input_handle(oscillator_instance, "phase", &oscillator_freq))
        return 1;

    if (node_get_output_handle(modulator_instance, "output", &modulator_out))
        return 1;
    if (node_get_input_handle(modulator_instance, "frequency", &modulator_freq))
        return 1;
    if (node_get_input_handle(oscillator_instance, "volume",
                              &oscillator_volume))
        return 1;

    if (node_connect_to_output(oscillator_instance, oscillator_out))
        return 1;

    node_connect(modulator_instance, modulator_out, oscillator_instance,
                 oscillator_freq);

    // node_set_default_value(modulator_instance, modulator_volume, 1.0);
    node_set_default_value(oscillator_instance, oscillator_volume, 0.2);
    // node_set_default_value(modulator_instance, modulator_freq, 0.5);

    jack_init();
    gui_init();

    gui_main();

    return 0;
}

#include "common.h"
#include "gui/gui.h"
#include "jack_client.h"
#include "node_files.h"

#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

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

    if (jack_init())
        return 1;
    gui_init();
    node_init();

    atexit(cleanup);

    gui_main();

    return 0;
}

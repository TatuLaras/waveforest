#include "common.h"
#include "gui/fuzzy_picker.h"
#include "gui/gui.h"
#include "jack_client.h"
#include "node_files.h"
#include "patch_files.h"

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
    fuzzy_picker_deinit();
}

#ifdef LSP_EDITOR
#define NODE_DIR ""
#define PATCH_DIR ""
#endif

int main(int argc, char **argv) {

    node_files_set_directory(NODE_DIR);
    patch_set_directory(PATCH_DIR);

    signal(SIGQUIT, exit);
    signal(SIGHUP, exit);
    signal(SIGTERM, exit);
    signal(SIGINT, exit);

    node_init();

    if (jack_init())
        return 1;
    gui_init();

    atexit(cleanup);

    gui_main();

    return 0;
}

#include "common.h"
#include "gui/fuzzy_picker.h"
#include "gui/gui.h"
#include "jack_client.h"
#include "node_files.h"
#include "patch_files.h"

#include <assert.h>
#include <linux/limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
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

    char *home_dir = 0;
    char patch_dir[PATH_MAX + 1] = {0};

    if ((home_dir = getenv("HOME")) == NULL) {
        home_dir = getpwuid(getuid())->pw_dir;
    }

    strncpy(patch_dir, home_dir, PATH_MAX);
    strncat(patch_dir, "/.waveforest", PATH_MAX - strlen(patch_dir));

    printf("This should show up %s\n", patch_dir);

    node_files_set_directory(NODE_DIR);
    patch_set_directory(patch_dir);

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

#include "common_header.h"

#include <stdio.h>
#include <stdlib.h>

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterPortFunction register_port) {

    printf("HELOUST filtteri\n");
    (*register_port)(handle, "control inputti tai jotain", PORT_INPUT_CONTROL);
    (*register_port)(handle, "outputti tai jotain", PORT_OUTPUT);
    return malloc(1);
}

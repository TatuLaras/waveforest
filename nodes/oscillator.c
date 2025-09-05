#include "common_header.h"

#include <stdio.h>
#include <stdlib.h>

void *node_instantiate(NodeInstanceHandle handle,
                       RegisterPortFunction register_port) {
    printf("HELOUST oscillatori\n");
    (*register_port)(handle, "inputti tai jotain", PORT_INPUT);
    (*register_port)(handle, "outputti tai jotain", PORT_OUTPUT);
    return malloc(1);
}

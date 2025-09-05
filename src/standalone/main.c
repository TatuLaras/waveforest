#include "node_files.h"
#include <stdint.h>
#include <stdio.h>

int main(int argc, char **argv) {

    node_files_set_directory("build/nodes/");
    StringVector nodes = node_files_enumerate();

    size_t i = 0;
    char *node_name;
    while ((node_name = stringvec_get(&nodes, i++))) {
        printf("node: %s\n", node_name);
    }

    Node node1 = {0};
    node_files_load("oscillator", &node1);
    NodeHandle node_handle = node_new(node1);
    node_instantiate(node_handle);

    printf("Exists: %i\n", node_exists("filter"));
    node_files_load("filter", &node1);
    NodeHandle node_handle2 = node_new(node1);
    printf("Exists: %i\n", node_exists("filter"));
    node_instantiate(node_handle2);

    printf("nh %u\n", node_handle);
    printf("%u\n", node_instantiate(node_handle));
    printf("%u\n", node_instantiate(node_handle));

    stringvec_free(&nodes);

    return 0;
}

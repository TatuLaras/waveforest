#include "node_manager.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

//  TODO: Non-port node instance options like dropdowns and checkboxes need to
//  be registered as well.

VEC_IMPLEMENT(Port, PortVec, portvec)
typedef struct {
    NodeInstanceHandle from_instance;
    PortHandle from_port;
    PortHandle to_port;
} Connection;

VEC_DECLARE(Connection, ConnectionVec, connectionvec)
VEC_IMPLEMENT(Connection, ConnectionVec, connectionvec)

typedef struct {
    PortVec ports;
    ConnectionVec incoming_connections;
    void *arg;
} NodeInstance;

VEC_DECLARE(NodeInstance, NodeInstanceVec, nodeinstancevec)
VEC_IMPLEMENT(NodeInstance, NodeInstanceVec, nodeinstancevec)

VEC_DECLARE(Node, NodeVec, nodevec)
VEC_IMPLEMENT(Node, NodeVec, nodevec)

static NodeVec nodes = {0};
static NodeInstanceVec node_instances = {0};

static void prepare_buffer(Buffer *buffer, uint64_t target_capacity) {

    uint64_t target_bytes = target_capacity * sizeof(float);

    if (!buffer->data) {
        buffer->data = calloc(1, target_bytes);
        buffer->data_allocated = target_capacity;
        return;
    }

    if (buffer->data_allocated >= target_capacity)
        return;

    buffer->data = realloc(buffer->data, target_bytes);
    memset(buffer->data, 0, target_bytes);
    buffer->data_allocated = target_capacity;
}

static PortHandle register_port(NodeInstanceHandle handle, char *name,
                                PortType type) {

    NodeInstance *node_instance = nodeinstancevec_get(&node_instances, handle);
    if (!node_instance) {
        fprintf(stderr, "ERROR: register_port() called with "
                        "invalid/non-existant node handle.\n");
        exit(1);
    }

    Port port = {.type = type};
    strncpy(port.name, name, MAX_NAME);

    return portvec_append(&node_instance->ports, port);
}

NodeHandle node_new(Node node) {

    if (!nodes.data)
        nodes = nodevec_init();

    return nodevec_append(&nodes, node);
}

int node_exists(const char *name) {
    for (size_t i = 0; i < nodes.data_used; i++) {
        if (!strncmp(nodes.data[i].name, name, MAX_NAME))
            return 1;
    }
    return 0;
}

NodeInstanceHandle node_instantiate(NodeHandle node_handle) {

    Node *node = nodevec_get(&nodes, node_handle);
    if (!node) {
        fprintf(stderr, "ERROR: node_instantiate() called with "
                        "invalid/non-existant node handle.\n");
        return 0;
    }

    NodeInstance instance = {
        .ports = portvec_init(),
        .incoming_connections = connectionvec_init(),
    };

    if (!node_instances.data)
        node_instances = nodeinstancevec_init();

    NodeInstanceHandle instance_handle =
        nodeinstancevec_append(&node_instances, instance);
    NodeInstance *appended_instance =
        nodeinstancevec_get(&node_instances, instance_handle);

    appended_instance->arg =
        (*node->functions.instantiate)(instance_handle, &register_port);

    return instance_handle;
}

void node_instances_connect(NodeInstanceHandle from_instance,
                            PortHandle from_port,
                            NodeInstanceHandle to_instance,
                            PortHandle to_port) {

    NodeInstance *from_instance_p =
        nodeinstancevec_get(&node_instances, from_instance);
    NodeInstance *to_instance_p =
        nodeinstancevec_get(&node_instances, to_instance);

    if (!from_instance_p || !to_instance_p) {
        fprintf(stderr, "ERROR: nodes_connect() called with "
                        "invalid/non-existant node handle.\n");
        return;
    }
    if (from_port >= from_instance_p->ports.data_used ||
        to_port >= to_instance_p->ports.data_used) {
        fprintf(stderr, "ERROR: nodes_connect() called with "
                        "invalid/non-existant port handle.\n");
        return;
    }
    if (from_instance_p->ports.data[from_port].type != PORT_OUTPUT ||
        (to_instance_p->ports.data[to_port].type != PORT_INPUT &&
         to_instance_p->ports.data[to_port].type != PORT_INPUT_CONTROL)) {
        fprintf(stderr, "ERROR: nodes_connect(): only connections from input "
                        "ports to output ports are allowed.\n");
        return;
    }

    connectionvec_append(&to_instance_p->incoming_connections,
                         (Connection){
                             .from_instance = from_instance,
                             .from_port = from_port,
                             .to_port = to_port,
                         });
}

PortVec node_instance_get_ports(NodeInstanceHandle instance) {
    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        fprintf(stderr, "ERROR: node_instance_get_ports() called with "
                        "invalid/non-existant node handle.\n");
        return (PortVec){0};
    }
    return node_instance->ports;
}

void node_free(Node *node) {

    if (node->dl_handle) {
        dlclose(node->dl_handle);
        node->dl_handle = 0;
    }
    //  TODO: Free all node instance buffers
}

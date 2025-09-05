#ifndef _NODE_MANAGER
#define _NODE_MANAGER

//  TODO: Non-port node instance options like dropdowns and checkboxes need to
//  be registered as well.

#include "common_node_types.h"
#include "vec.h"
#include <stdint.h>

#define MAX_NAME 128

// Node file function API
typedef void *(*NodeInstantiateFunction)(NodeHandle node_handle,
                                         RegisterPortFunction register_port);

typedef struct {
    char name[MAX_NAME];
    void *dl_handle;
    struct {
        NodeInstantiateFunction instantiate;
    } functions;
} Node;

typedef struct {
    float *data;
    uint64_t data_allocated;
} Buffer;

typedef struct {
    char name[MAX_NAME];
    PortType type;
    Buffer buffer;
} Port;

VEC_DECLARE(Port, PortVec, portvec)

NodeHandle node_new(Node node);
NodeInstanceHandle node_instantiate(NodeHandle node_handle);
void node_free(Node *node);
int node_exists(const char *name);
void node_instances_connect(NodeInstanceHandle from_instance,
                            PortHandle from_port,
                            NodeInstanceHandle to_instance, PortHandle to_port);
PortVec node_instance_get_ports(NodeInstanceHandle instance);

#endif

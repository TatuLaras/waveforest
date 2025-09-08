#ifndef _NODE_MANAGER
#define _NODE_MANAGER

//  TODO: Non-port node instance options like dropdowns and checkboxes need to
//  be registered as well.
//  TODO: On connecting node instances, ensure there is no loops in the network.

#include "common_node_types.h"
#include "vec.h"
#include <stdint.h>

// ----- Node plugin function API -----
typedef void *(*NodeInstantiateFunction)(
    NodeInstanceHandle handle, RegisterInputPortFunction register_input,
    RegisterOutputPortFunction register_output);
typedef void (*NodeProcessFunction)(void *arg, Info *info,
                                    float **input_buffers,
                                    float **output_buffers,
                                    uint32_t frame_count);

typedef struct {
    char name[MAX_NAME];
    void *dl_handle;
    struct {
        NodeInstantiateFunction instantiate;
        NodeProcessFunction process;
    } functions;
} Node;
VEC_DECLARE(Node, NodeVec, nodevec)

typedef struct {
    float *data;
    uint64_t data_allocated;
} Buffer;

typedef struct {
    char name[MAX_NAME];
    Buffer buffer;
} OutputPort;
VEC_DECLARE(OutputPort, OutputPortVec, outputportvec)

VEC_DECLARE(InputPort, InputPortVec, inputportvec)

typedef struct {
    OutputPortVec outputs;
    InputPortVec inputs;
    NodeHandle node;
    // Compared with an incrementing internal value to ensure no nodes are
    // processed multiple times
    uint8_t staleness;
    void *arg;
} NodeInstance;
VEC_DECLARE(NodeInstance, NodeInstanceVec, nodeinstancevec)

NodeHandle node_new(Node node);
NodeInstanceHandle node_instantiate(NodeHandle node_handle);
void node_free(Node *node);
int node_exists(const char *name);

void node_connect(NodeInstanceHandle output_instance,
                  OutputPortHandle output_port,
                  NodeInstanceHandle input_instance,
                  InputPortHandle input_port);
int node_connect_to_output(NodeInstanceHandle instance,
                           OutputPortHandle output_port);

int node_get_input_handle(NodeInstanceHandle instance, const char *name,
                          InputPortHandle *out_handle);
int node_get_output_handle(NodeInstanceHandle instance, const char *name,
                           OutputPortHandle *out_handle);

InputPortVec node_get_inputs(NodeInstanceHandle instance);
OutputPortVec node_get_outputs(NodeInstanceHandle instance);

NodeInstance *node_instance_get(NodeInstanceHandle instance);
Node *node_get(NodeHandle node);

void node_set_default_value(NodeInstanceHandle instance, InputPortHandle port,
                            float value);
void node_consume_network(uint32_t frame_count, float *out_frames);
void node_set_sample_rate(float sample_rate);
void node_register_midi_event(MidiEvent event);

#endif

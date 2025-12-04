#ifndef _NODE_MANAGER
#define _NODE_MANAGER

#include "common_node_types.h"
#include "vec.h"
#include <stdint.h>

#define INSTANCE_HANDLE_OUTPUT_PORT 0

// ----- Node plugin function API -----
typedef void *(*NodeInstantiateFunction)(
    NodeInstanceHandle handle, uint8_t *out_height_in_grid_units,
    RegisterInputPortFunction register_input,
    RegisterOutputPortFunction register_output);
typedef void (*NodeProcessFunction)(void *arg, Info *info,
                                    InputBuffer *input_buffers,
                                    float **output_buffers,
                                    uint32_t frame_count);

// Optionals
typedef void (*NodeFreeFunction)(void *arg);
typedef void (*NodeGuiFunction)(void *arg, Draw draw);

typedef struct {
    char name[MAX_NAME];
    void *dl_handle;
    struct {
        NodeInstantiateFunction instantiate;
        NodeFreeFunction free;
        NodeGuiFunction gui;
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
    int32_t x;
    int32_t y;
} NodePositioning;

typedef struct {
    OutputPortVec outputs;
    InputPortVec inputs;
    NodeHandle node;
    // Compared with an incrementing internal value to ensure no nodes are
    // processed multiple times
    uint8_t staleness;
    void *arg;
    uint8_t height;
    uint8_t is_deleted;
    NodePositioning positioning;
} NodeInstance;
VEC_DECLARE(NodeInstance, NodeInstanceVec, nodeinstancevec)

// Initialize module.
void node_init(void);
// Resets the module to its initial state.
void node_reset(void);
// Registers a new `node`. Usually used in tandem with node_files_load() of
// node_files.h.
NodeHandle node_new(Node node);
// After calling node_new(), call this to instantiate the loaded node into an
// actual node instance that you can connect to other node instances or the
// output to produce sound.
NodeInstanceHandle node_instantiate(NodeHandle node_handle);
// Destroys / deletes node instance and all its connections.
void node_instance_destroy(NodeInstanceHandle instance_handle);

// Frees a `node` s memory.
void node_free(Node *node);
// Returns 1 if a node with `name` has been loaded via node_new(), writing its
// handle into `out_handle`.
int node_get_by_name(const char *name, NodeHandle *out_handle);

// Connect the output port `output_port` of `output_instance` to the input
// port `input_port` of `input_instance`.
void node_connect(NodeInstanceHandle output_instance,
                  OutputPortHandle output_port,
                  NodeInstanceHandle input_instance,
                  InputPortHandle input_port);

// Disconnects input `port` of `instance`.
void node_disconnect(NodeInstanceHandle instance, InputPortHandle port);
// Connects `output_port` of node instance `instance` into the output so that
// the output of that port will be written to the final audio driver output
// buffers. Only one node instance can be connected to the output (use a
// dedicated mixing/add node if there is a need to output audible signals from
// multiple node instances).
int node_connect_to_output(NodeInstanceHandle instance,
                           OutputPortHandle output_port);
// Returns 1 if there is a no node connected to the network output. Will write
// its handle and port handle to `out_handle` and `out_port` respectively.
int node_get_outputting_node_instance(NodeInstanceHandle *out_handle,
                                      OutputPortHandle *out_port);

// Searches the node `instance` input ports for a port with `name` and writes
// the handle to `out_handle`. Returns 1 if no such port is found.
int node_get_input(NodeInstanceHandle instance, const char *name,
                   InputPortHandle *out_handle);
// Searches the node `instance` output ports for a port with `name` and writes
// the handle to `out_handle`. Returns 1 if no such port is found.
int node_get_output(NodeInstanceHandle instance, const char *name,
                    OutputPortHandle *out_handle);

// Returns a list of the input ports registered for this node instance. List
// elemenets can be accessed either directly or using the inputportvec
// functions (see vec.h for more information about vector usage).
InputPortVec node_get_inputs(NodeInstanceHandle instance);
// Returns a list of the output ports registered for this node instance. List
// elemenets can be accessed either directly or using the outputportvec
// functions (see vec.h for more information about vector usage).
OutputPortVec node_get_outputs(NodeInstanceHandle instance);

NodeInstance *node_instance_get(NodeInstanceHandle instance);
Node *node_get(NodeHandle node);

// Sets the manual (slider) value used for node instance input when there is
// nothing connected to it.
void node_set_manual_value(NodeInstanceHandle instance, InputPortHandle port,
                           float value);

// Process all nodes in the network and write the resulting PCM frames into
// `out_frames`.
void node_consume_network(uint32_t frame_count, float *out_frames);

void node_set_sample_rate(float sample_rate);

// Sets the position of a node, used for drawing them in a node editor.
void node_instance_set_positioning(NodeInstanceHandle instance,
                                   NodePositioning positioning);

// Returns the number of node instances.
uint32_t node_instance_count(void);

// Gets the current time in samples, with the granularity of one block of frames
// (e.g. 1024 samples).
uint64_t node_get_coarse_time(void);

#endif

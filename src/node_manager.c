#include "node_manager.h"
#include "common.h"
#include "midi.h"

#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define HARD_CLIP_LIMIT DB_CO(17)
#define MASTER_GAIN DB_CO(-16)

VEC_IMPLEMENT(NodeInstance, NodeInstanceVec, nodeinstancevec)
VEC_IMPLEMENT(Node, NodeVec, nodevec)
VEC_IMPLEMENT(OutputPort, OutputPortVec, outputportvec)
VEC_IMPLEMENT(InputPort, InputPortVec, inputportvec)

VEC_DECLARE(Buffer, BufferVec, buffervec)
VEC_IMPLEMENT(Buffer, BufferVec, buffervec)

VEC_DECLARE(float *, FloatPVec, floatpvec)
VEC_IMPLEMENT(float *, FloatPVec, floatpvec)

VEC_DECLARE(MidiEvent, MidiEventVec, midieventvec)
VEC_IMPLEMENT(MidiEvent, MidiEventVec, midieventvec)

static NodeVec nodes = {0};
static NodeInstanceVec node_instances = {0};
static uint8_t staleness_counter = 0;
static InputPort network_output_port = {0};
static Info info = {.sample_rate = 48000};
static FloatPVec temp_input_buffer_table = {0};
static FloatPVec temp_output_buffer_table = {0};
static BufferVec temp_constant_buffers = {0};

// Called from within a node plugin file to register a new input port for the
// node.
static PortHandle register_input(NodeInstanceHandle handle, InputPort port) {

    NodeInstance *node_instance = nodeinstancevec_get(&node_instances, handle);
    if (!node_instance) {
        ERROR("invalid node handle");
        exit(1);
    }

    port.manual.value =
        clamp(port.manual.default_value, port.manual.min, port.manual.max);
    return inputportvec_append(&node_instance->inputs, port);
}

// Called from within a node plugin file to register a new output port for the
// node.
static PortHandle register_output(NodeInstanceHandle handle, char *name) {

    NodeInstance *node_instance = nodeinstancevec_get(&node_instances, handle);
    if (!node_instance) {
        ERROR("invalid node handle");
        exit(1);
    }

    PortHandle port_handle =
        outputportvec_append(&node_instance->outputs, (OutputPort){0});
    char *name_buffer =
        outputportvec_get(&node_instance->outputs, port_handle)->name;

    strncpy(name_buffer, name, MAX_NAME);
    return port_handle;
}

void node_init(void) {
    node_instances = nodeinstancevec_init();
    // So that value zero can be used as a null value
    nodeinstancevec_append(&node_instances,
                           (NodeInstance){.positioning = {2, 2}});
}

NodeHandle node_new(Node node) {

    if (!nodes.data)
        nodes = nodevec_init();

    return nodevec_append(&nodes, node);
}

int node_get_by_name(const char *name, NodeHandle *out_handle) {
    for (size_t i = 0; i < nodes.data_used; i++) {
        if (!strncmp(nodes.data[i].name, name, MAX_NAME)) {
            if (out_handle)
                *out_handle = i;
            return 1;
        }
    }
    return 0;
}

NodeInstanceHandle node_instantiate(NodeHandle node_handle) {

    Node *node = nodevec_get(&nodes, node_handle);
    if (!node) {
        ERROR("invalid node handle");
        return 0;
    }

    NodeInstance instance = {.inputs = inputportvec_init(),
                             .outputs = outputportvec_init(),
                             .node = node_handle};

    NodeInstanceHandle instance_handle =
        nodeinstancevec_append(&node_instances, instance);
    NodeInstance *appended_instance =
        nodeinstancevec_get(&node_instances, instance_handle);

    appended_instance->arg = (*node->functions.instantiate)(
        instance_handle, &register_input, &register_output);

    return instance_handle;
}

void node_connect(NodeInstanceHandle output_instance,
                  OutputPortHandle output_port,
                  NodeInstanceHandle input_instance,
                  InputPortHandle input_port) {

    if (input_instance == INSTANCE_HANDLE_OUTPUT_PORT) {
        node_connect_to_output(output_instance, output_port);
        return;
    }

    NodeInstance *out = nodeinstancevec_get(&node_instances, output_instance);
    NodeInstance *in = nodeinstancevec_get(&node_instances, input_instance);

    if (!out || !in) {
        ERROR("invalid node instance handle");
        return;
    }

    InputPort *in_port = inputportvec_get(&in->inputs, input_port);

    if (output_port >= out->outputs.data_used || !in_port) {
        ERROR("invalid port handle %u %u", input_instance, input_port);
        return;
    }

    in_port->connection.output_instance = output_instance;
    in_port->connection.output_port = output_port;
}

void node_disconnect(NodeInstanceHandle instance, InputPortHandle port) {
    if (instance == INSTANCE_HANDLE_OUTPUT_PORT) {
        network_output_port = (InputPort){0};
        return;
    }

    InputPortVec inputs = node_get_inputs(instance);
    if (port > inputs.data_used) {
        ERROR("invalid port or node instance handle");
        return;
    }

    inputs.data[port].connection.output_instance = 0;
    inputs.data[port].connection.output_port = 0;
}

int node_connect_to_output(NodeInstanceHandle instance,
                           OutputPortHandle output_port) {
    NodeInstance *out = nodeinstancevec_get(&node_instances, instance);
    if (!out) {
        ERROR("invalid node handle");
        return 1;
    }
    OutputPort *port = outputportvec_get(&out->outputs, output_port);
    if (!port) {
        ERROR("invalid output port handle");
        return 1;
    }

    network_output_port.connection.output_instance = instance;
    network_output_port.connection.output_port = output_port;
    return 0;
}

int node_get_outputting_node_instance(NodeInstanceHandle *out_handle,
                                      OutputPortHandle *out_port) {
    if (!network_output_port.connection.output_instance)
        return 1;
    if (out_handle)
        *out_handle = network_output_port.connection.output_instance;
    if (out_port)
        *out_port = network_output_port.connection.output_port;
    return 0;
}

InputPortVec node_get_inputs(NodeInstanceHandle instance) {

    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        ERROR("invalid node handle");
        return (InputPortVec){0};
    }
    return node_instance->inputs;
}

int node_get_output_handle(NodeInstanceHandle instance, const char *name,
                           OutputPortHandle *out_handle) {

    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        ERROR("invalid node handle");
        return 1;
    }

    for (uint32_t i = 0; i < node_instance->outputs.data_used; i++) {
        if (!strcmp(node_instance->outputs.data[i].name, name)) {
            *out_handle = i;
            return 0;
        }
    }

    return 1;
}

int node_get_input_handle(NodeInstanceHandle instance, const char *name,
                          InputPortHandle *out_handle) {

    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        ERROR("invalid node handle");
        return 1;
    }

    for (uint32_t i = 0; i < node_instance->inputs.data_used; i++) {
        if (!strcmp(node_instance->inputs.data[i].name, name)) {
            *out_handle = i;
            return 0;
        }
    }

    return 1;
}

OutputPortVec node_get_outputs(NodeInstanceHandle instance) {

    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        ERROR("invalid node handle");
        return (OutputPortVec){0};
    }
    return node_instance->outputs;
}

// Makes sure the `buffer` has at least `target_capacity` floats of space.
static inline void prepare_buffer(Buffer *buffer, uint64_t target_capacity) {

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

static int process_node(uint32_t frame_count, NodeInstanceHandle instance) {

    NodeInstance *node_instance = node_instances.data + instance;
    if (node_instance->staleness == staleness_counter)
        return 0;

    Node *node = nodes.data + node_instance->node;

    // Make sure dependency/input nodes are all processed before this one
    //  TODO: Non-recursive approach to improve performance and prevent the risk
    //  of stack overflow on large node networks
    for (uint32_t i = 0; i < node_instance->inputs.data_used; i++) {
        NodeInstanceHandle dependency_instance =
            node_instance->inputs.data[i].connection.output_instance;
        if (dependency_instance)
            process_node(frame_count, dependency_instance);
    }

    temp_output_buffer_table.data_used = 0;
    temp_input_buffer_table.data_used = 0;
    uint32_t constant_buffer_i = 0;

    // Prepare inputs
    for (uint32_t i = 0; i < node_instance->inputs.data_used; i++) {
        InputPort *input_port = node_instance->inputs.data + i;

        // If an input port is not connected to anything a constant value will
        // be used
        //  TODO: A way to have a constant value without having to fill a hole
        //  buffer with it
        if (!input_port->connection.output_instance) {
            if (constant_buffer_i >= temp_constant_buffers.data_used)
                buffervec_append(&temp_constant_buffers, (Buffer){0});

            Buffer *buffer = temp_constant_buffers.data + constant_buffer_i;
            prepare_buffer(buffer, frame_count);

            for (uint32_t j = 0; j < frame_count; j++)
                buffer->data[j] = input_port->manual.value;

            floatpvec_append(&temp_input_buffer_table, buffer->data);
            constant_buffer_i++;
            continue;
        }

        NodeInstance *output_instance =
            node_instances.data + input_port->connection.output_instance;
        Buffer *buffer =
            &output_instance->outputs.data[input_port->connection.output_port]
                 .buffer;
        floatpvec_append(&temp_input_buffer_table, buffer->data);
    }

    // Prepare outputs
    for (uint32_t i = 0; i < node_instance->outputs.data_used; i++) {
        Buffer *buffer = &node_instance->outputs.data[i].buffer;
        prepare_buffer(buffer, frame_count);
        floatpvec_append(&temp_output_buffer_table, buffer->data);
    }

    (*node->functions.process)(node_instance->arg, &info,
                               temp_input_buffer_table.data,
                               temp_output_buffer_table.data, frame_count);
    node_instance->staleness = staleness_counter;

    return 0;
}

void node_consume_network(uint32_t frame_count, float *out_frames) {
    if (!temp_input_buffer_table.data)
        temp_input_buffer_table = floatpvec_init();
    if (!temp_output_buffer_table.data)
        temp_output_buffer_table = floatpvec_init();
    if (!temp_constant_buffers.data)
        temp_constant_buffers = buffervec_init();

    NodeInstance *output_instance = nodeinstancevec_get(
        &node_instances, network_output_port.connection.output_instance);
    if (!output_instance)
        return;

    if (network_output_port.connection.output_port >=
        output_instance->outputs.data_used) {
#ifdef DEBUG
        ERROR("out of range port handle");
        HINT("Is there a node connected to the output port?");
#endif
        // Prevents harsh repetition of previous samples if output is
        // disconnected while playing a note.
        memset(out_frames, 0, frame_count * sizeof(float));
        return;
    }

    float buffer[frame_count];
    memset(buffer, 0, frame_count * sizeof(float));

    for (uint32_t note_i = 0; note_i < MIDI_NOTES; note_i++) {

        if (!midi_note_on[note_i])
            continue;

        info.note.note = note_i;
        info.note.is_on = 1;
        staleness_counter++;
        int error = process_node(
            frame_count, network_output_port.connection.output_instance);
        if (error)
            return;
        float *data = output_instance->outputs
                          .data[network_output_port.connection.output_port]
                          .buffer.data;
        for (uint32_t i = 0; i < frame_count; i++) {
            buffer[i] += data[i];
        }
    }

    for (uint32_t i = 0; i < frame_count; i++) {
        float amplitude = buffer[i];
        amplitude *= MASTER_GAIN;

        float mag = fabsf(amplitude);
        if (mag > HARD_CLIP_LIMIT)
            amplitude *= HARD_CLIP_LIMIT / mag;
        out_frames[i] = amplitude;
    }

    info.coarse_time += frame_count;
}

void node_free(Node *node) {

    if (node->dl_handle) {
        dlclose(node->dl_handle);
        node->dl_handle = 0;
    }
    //  TODO: Free all node instance buffers
}

void node_set_sample_rate(float sample_rate) {
    INFO("new sample rate %f", sample_rate);
    info.sample_rate = sample_rate;
}

NodeInstance *node_instance_get(NodeInstanceHandle instance) {
    return nodeinstancevec_get(&node_instances, instance);
}

Node *node_get(NodeHandle node) {
    return nodevec_get(&nodes, node);
}

#include "node_manager.h"
#include "log.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

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
static MidiEventVec midi_events = {0};

static PortHandle register_input(NodeInstanceHandle handle, InputPort port) {

    NodeInstance *node_instance = nodeinstancevec_get(&node_instances, handle);
    if (!node_instance) {
        ERROR("invalid node handle");
        exit(1);
    }

    port.manual.value = port.manual.default_value;
    return inputportvec_append(&node_instance->inputs, port);
}

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
        ERROR("invalid node handle");
        return 0;
    }

    NodeInstance instance = {
        .inputs = inputportvec_init(),
        .outputs = outputportvec_init(),
        .node = node_handle,
    };

    if (!node_instances.data) {
        node_instances = nodeinstancevec_init();
        // So that value zero can be used as a null value
        nodeinstancevec_append(&node_instances, (NodeInstance){0});
    }

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

    NodeInstance *out = nodeinstancevec_get(&node_instances, output_instance);
    NodeInstance *in = nodeinstancevec_get(&node_instances, input_instance);

    if (!out || !in) {
        ERROR("invalid node instance handle");
        return;
    }

    InputPort *in_port = inputportvec_get(&in->inputs, input_port);

    if (output_port >= out->outputs.data_used || !in_port) {
        ERROR("invalid port handle");
        return;
    }

    in_port->connection.output_instance = output_instance;
    in_port->connection.output_port = output_port;
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
    if (!midi_events.data)
        midi_events = midieventvec_init();

    info.midi_events = midi_events.data;
    info.midi_event_count = midi_events.data_used;

    staleness_counter++;

    NodeInstance *output_instance = nodeinstancevec_get(
        &node_instances, network_output_port.connection.output_instance);
    if (!output_instance)
        return;

    if (network_output_port.connection.output_port >=
        output_instance->outputs.data_used) {
        ERROR("out of range port handle");
        exit(1);
        return;
    }

    int error = process_node(frame_count,
                             network_output_port.connection.output_instance);
    if (error)
        return;

    float *buffer = output_instance->outputs
                        .data[network_output_port.connection.output_port]
                        .buffer.data;
    memcpy(out_frames, buffer, frame_count * sizeof(float));

    midi_events.data_used = 0;
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

void node_register_midi_event(MidiEvent event) {
    midieventvec_append(&midi_events, event);
}

void node_set_default_value(NodeInstanceHandle instance, InputPortHandle port,
                            float value) {
    NodeInstance *node_instance =
        nodeinstancevec_get(&node_instances, instance);
    if (!node_instance) {
        ERROR("invalid node instance handle");
        return;
    }
    if (port >= node_instance->inputs.data_used) {
        ERROR("out of range port handle");
        return;
    }

    node_instance->inputs.data[port].manual.value = value;
}

NodeInstance *node_instance_get(NodeInstanceHandle instance) {
    return nodeinstancevec_get(&node_instances, instance);
}

Node *node_get(NodeHandle node) {
    return nodevec_get(&nodes, node);
}

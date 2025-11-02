#include "node_manager.h"
#include "common.h"
#include "midi.h"

#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
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

VEC_DECLARE(InputBuffer, InputBufferVec, inputbuffervec)
VEC_IMPLEMENT(InputBuffer, InputBufferVec, inputbuffervec)

VEC_DECLARE(MidiEvent, MidiEventVec, midieventvec)
VEC_IMPLEMENT(MidiEvent, MidiEventVec, midieventvec)

static NodeVec nodes = {0};
static NodeInstanceVec node_instances = {0};
static uint8_t staleness_counter = 0;
static InputPort network_output_port = {0};
static Info info = {.sample_rate = 48000, .bpm = 120};
static InputBufferVec input_buffers = {0};
static FloatPVec output_buffers = {0};

// Used to lock `nodes` and `node_instances` when appended to in order to
// prevent use after frees by the audio thread if the vector is resized.
static pthread_spinlock_t lock;

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

    if (pthread_spin_init(&lock, PTHREAD_PROCESS_SHARED)) {
        ERROR("could not initialize spinlock");
        exit(1);
    }

    node_instances = nodeinstancevec_init();
    // So that value zero can be used as a null value
    nodeinstancevec_append(&node_instances,
                           (NodeInstance){.positioning = {2, 2}});
    nodes = nodevec_init();
}

void node_reset(void) {

    for (uint32_t i = 1; i < node_instances.data_used; i++) {
        NodeInstance *node_instance = node_instance_get(i);
        if (!node_instance)
            continue;
        Node *node = node_get(node_instance->node);
        if (!node)
            continue;

        if (node->functions.free)
            node->functions.free(node_instance->arg);

        for (uint32_t j = 0; j < node_instance->outputs.data_used; j++) {
            Buffer buf = node_instance->outputs.data[j].buffer;
            if (buf.data)
                free(buf.data);
        }
        outputportvec_free(&node_instance->outputs);
        inputportvec_free(&node_instance->inputs);
    }

    for (uint32_t i = 0; i < nodes.data_used; i++) {
        Node *node = node_get(i);
        if (node)
            node_free(node);
    }

    node_instances.data_used = 1;
    nodes.data_used = 0;
}

NodeHandle node_new(Node node) {

    pthread_spin_lock(&lock);
    int result = nodevec_append(&nodes, node);
    pthread_spin_unlock(&lock);

    return result;
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

static uint8_t calculate_node_height(uint64_t port_count) {

    uint8_t height = 1;

    if (port_count >= 4) {
        height = 2 + (port_count - 4) / 5;
    }
    return height;
}

NodeInstanceHandle node_instantiate(NodeHandle node_handle) {

    Node *node = nodevec_get(&nodes, node_handle);
    if (!node) {
        ERROR("invalid node handle");
        return 0;
    }

    NodeInstance instance = {.inputs = inputportvec_init(),
                             .outputs = outputportvec_init(),
                             .node = node_handle,
                             .height = 1};

    pthread_spin_lock(&lock);

    NodeInstanceHandle instance_handle =
        nodeinstancevec_append(&node_instances, instance);

    NodeInstance *appended_instance =
        nodeinstancevec_get(&node_instances, instance_handle);

    appended_instance->arg = (*node->functions.instantiate)(
        instance_handle, &appended_instance->height, &register_input,
        &register_output);

    pthread_spin_unlock(&lock);
    return instance_handle;
}

void node_instance_destroy(NodeInstanceHandle instance_handle) {

    if (instance_handle == INSTANCE_HANDLE_OUTPUT_PORT)
        return;

    NodeInstance *instance = node_instance_get(instance_handle);
    if (!instance)
        return;

    for (NodeInstanceHandle i = 0; i < node_instances.data_used; i++) {

        NodeInstance *connected_instance = node_instances.data + i;
        for (InputPortHandle j = 0; j < connected_instance->inputs.data_used;
             j++) {

            InputPort *port = connected_instance->inputs.data + j;
            if (port->connection.output_instance != instance_handle)
                continue;

            port->connection.output_instance = 0;
            port->connection.output_port = 0;
        }
    }

    instance->is_deleted = 1;
    instance->inputs.data_used = 0;
    instance->outputs.data_used = 0;
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

int node_get_output(NodeInstanceHandle instance, const char *name,
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

int node_get_input(NodeInstanceHandle instance, const char *name,
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

// Processes a single node, aka. prepares all its buffers and calls the nodes
// process function.
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

    output_buffers.data_used = 0;
    input_buffers.data_used = 0;

    // Prepare inputs
    for (uint32_t i = 0; i < node_instance->inputs.data_used; i++) {
        InputPort *input_port = node_instance->inputs.data + i;

        if (!input_port->connection.output_instance) {
            inputbuffervec_append(
                &input_buffers,
                (InputBuffer){.value = input_port->manual.value});
            continue;
        }

        NodeInstance *output_instance =
            node_instances.data + input_port->connection.output_instance;
        Buffer *buffer =
            &output_instance->outputs.data[input_port->connection.output_port]
                 .buffer;
        inputbuffervec_append(&input_buffers,
                              (InputBuffer){.data = buffer->data});
    }

    // Prepare outputs
    for (uint32_t i = 0; i < node_instance->outputs.data_used; i++) {
        Buffer *buffer = &node_instance->outputs.data[i].buffer;
        prepare_buffer(buffer, frame_count);
        floatpvec_append(&output_buffers, buffer->data);
    }

    (*node->functions.process)(node_instance->arg, &info, input_buffers.data,
                               output_buffers.data, frame_count);
    node_instance->staleness = staleness_counter;

    return 0;
}

void node_consume_network(uint32_t frame_count, float *out_frames) {

    if (!input_buffers.data)
        input_buffers = inputbuffervec_init();
    if (!output_buffers.data)
        output_buffers = floatpvec_init();

    pthread_spin_lock(&lock);

    NodeInstance *output_instance = nodeinstancevec_get(
        &node_instances, network_output_port.connection.output_instance);
    if (!output_instance) {
        pthread_spin_unlock(&lock);
        return;
    }

    if (network_output_port.connection.output_port >=
        output_instance->outputs.data_used) {

        // Prevents harsh repetition of previous samples if output is
        // disconnected while playing a note.
        pthread_spin_unlock(&lock);
        memset(out_frames, 0, frame_count * sizeof(float));
        return;
    }

    float buffer[frame_count];
    memset(buffer, 0, frame_count * sizeof(float));

    for (uint32_t note_i = 0; note_i < MIDI_NOTES; note_i++) {
        NoteInfo *note_info = midi_get_note_info(note_i);

        if (!note_info)
            continue;
        double time_since_release =
            (info.coarse_time + frame_count - note_info->release_time) /
            info.sample_rate;
        if (!note_info->is_on && (time_since_release > MAX_RELEASE_TIME ||
                                  note_info->release_time == 0))
            continue;

        info.note = note_info;

        staleness_counter++;
        if (process_node(frame_count,
                         network_output_port.connection.output_instance)) {
            pthread_spin_unlock(&lock);
            return;
        }

        float *data = output_instance->outputs
                          .data[network_output_port.connection.output_port]
                          .buffer.data;

        for (uint32_t i = 0; i < frame_count; i++)
            buffer[i] += data[i];
    }

    pthread_spin_unlock(&lock);

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

uint64_t node_get_coarse_time(void) {
    return info.coarse_time;
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

void node_set_manual_value(NodeInstanceHandle instance, InputPortHandle port,
                           float value) {

    InputPortVec inputs = node_get_inputs(instance);
    if (port >= inputs.data_used)
        return;

    inputs.data[port].manual.value = value;
}

uint32_t node_instance_count(void) {
    return node_instances.data_used;
}

void node_instance_set_positioning(NodeInstanceHandle instance,
                                   NodePositioning positioning) {
    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance)
        return;
    node_instance->positioning = positioning;
}

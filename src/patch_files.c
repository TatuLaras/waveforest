#include "patch_files.h"
#include "common.h"
#include "common_node_types.h"
#include "general_buffer.h"
#include "node_files.h"
#include "node_manager.h"
#include "notify_user.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAGIC 0x3694
#define VERSION 0
#define EXTENSION ".patch"

enum PatchFlags : uint16_t {
    PATCH_FLAG_IS_PINNED = 1,
};

//  NOTE: Only grow these structs, don't shrink. That way backwards
//  compatability with old files is maintained.

typedef struct {
    char name[MAX_NAME];
    float manual_value;
    uint8_t is_connected;
    uint32_t connected_node_id;
    char connected_port_name[MAX_NAME];
} SerializedInputPort;

typedef struct {
    uint16_t magic;
    uint16_t header_size;
    uint16_t version;
    uint16_t flags;
    uint32_t node_instances_count;
    uint32_t node_instance_size;
    uint32_t largest_id;
    SerializedInputPort output;
    NodePositioning output_positioning;
} FileHeader;

typedef struct {
    char name[MAX_NAME];
    uint32_t id;
    uint32_t input_ports_count;
    uint32_t input_port_size;
    NodePositioning positioning;
    uint8_t custom_data[MAX_CUSTOM_DATA];
} SerializedNodeInstance;

static char patch_directory[PATH_MAX] = {0};

static inline int write_file(const char *file, GeneralBuffer *buf,
                             uint32_t node_instances_count, uint32_t largest_id,
                             SerializedInputPort output,
                             NodePositioning output_positioning) {

    FILE *fp = fopen(file, "w");
    if (!fp) {
        ERROR("could not open file '%s' for writing", file);
        perror("");
        return 1;
    }

    FileHeader header = {
        .magic = MAGIC,
        .header_size = sizeof(FileHeader),
        .version = VERSION,
        .node_instances_count = node_instances_count,
        .node_instance_size = sizeof(SerializedNodeInstance),
        .largest_id = largest_id,
        .output = output,
        .output_positioning = output_positioning,
    };

    int result = 1;
    result = result && fwrite(&header, sizeof header, 1, fp);
    result = result && fwrite(buf->data, buf->data_size, 1, fp);

    if (!result) {
        ERROR("could not write to file '%s'", file);
        perror("");
    }

    fclose(fp);
    return !result;
}

int patch_write(const char *file) {

    GeneralBuffer buf = genbuf_init();

    uint32_t instances_count = 0;
    NodeInstanceHandle instance_i = 0;
    NodeInstance *instance;
    // Skips index 0, we don't need to store the output port placeholder node
    while ((instance = node_instance_get(++instance_i))) {

        if (instance->is_deleted)
            continue;

        Node *node = node_get(instance->node);
        if (!node)
            continue;

        InputPortVec inputs = node_get_inputs(instance_i);

        // Node instance
        SerializedNodeInstance serialized_instance = {
            .id = instance_i,
            .input_ports_count = inputs.data_used,
            .input_port_size = sizeof(SerializedInputPort),
            .positioning = instance->positioning,
        };

        // Serialize custom data
        if (node->functions.custom_data)
            (*node->functions.custom_data)(instance->arg,
                                           serialized_instance.custom_data);

        strncpy(serialized_instance.name, node->name, MAX_NAME - 1);
        genbuf_append(&buf, &serialized_instance, sizeof serialized_instance);

        // Input ports

        uint32_t port_i = 0;
        InputPort *input;
        while ((input = inputportvec_get(&inputs, port_i++))) {

            uint8_t is_connected = input->connection.output_instance ? 1 : 0;

            SerializedInputPort serialized_port = {
                .manual_value = input->manual.value,
                .is_connected = is_connected,
                .connected_node_id = input->connection.output_instance,
            };
            strncpy(serialized_port.name, input->name, MAX_NAME - 1);

            if (is_connected) {

                OutputPortVec outputs =
                    node_get_outputs(input->connection.output_instance);
                OutputPort *output =
                    outputportvec_get(&outputs, input->connection.output_port);
                if (!output)
                    continue;

                strncpy(serialized_port.connected_port_name, output->name,
                        MAX_NAME - 1);
            }

            genbuf_append(&buf, &serialized_port, sizeof serialized_port);
        }
        instances_count++;
    }

    // Network output port

    SerializedInputPort output = {0};

    NodeInstanceHandle out_handle;
    OutputPortHandle out_port;
    if (!node_get_outputting_node_instance(&out_handle, &out_port)) {

        output.is_connected = 1;
        output.connected_node_id = out_handle;
        OutputPortVec outputs = node_get_outputs(out_handle);
        OutputPort *port = outputportvec_get(&outputs, out_port);
        if (port)
            strncpy(output.connected_port_name, port->name, MAX_NAME - 1);
    }

    // Output positioning
    NodeInstance *output_node = node_instance_get(INSTANCE_HANDLE_OUTPUT_PORT);
    NodePositioning output_positioning = {0};
    if (output_node)
        output_positioning = output_node->positioning;

    int result = write_file(file, &buf, instances_count, instance_i, output,
                            output_positioning);
    genbuf_free(&buf);
    return result;
}

static inline NodeInstanceHandle instantiate(char *node_name,
                                             uint8_t *custom_data) {
    NodeHandle node_handle = 0;
    node_get_by_name(node_name, &node_handle);

    if (!node_handle) {
        Node node;
        if (node_files_load(node_name, &node)) {
            USER_SHOULD_KNOW("Could not load node '%s'", node_name);
            return 0;
        }
        node_handle = node_new(node);
    }

    return node_instantiate(node_handle, custom_data);
}

static inline int deserialize_patch(uint8_t *buf, size_t buf_size) {

    uint16_t *magic = (uint16_t *)buf;
    if (*magic != MAGIC) {
        ERROR("not a patch file (wrong magic number)");
        return 1;
    }

    node_reset();

    uint16_t header_size =
        *(uint16_t *)(buf + offsetof(FileHeader, header_size));
    FileHeader header = {0};
    memcpy(&header, buf, header_size);

    uint8_t *cursor = buf + header_size;

    NodeInstanceHandle id_to_instance_mapping[header.largest_id];
    memset(id_to_instance_mapping, 0,
           header.largest_id * sizeof(NodeInstanceHandle));

    // First pass, node instances
    for (uint32_t i = 0; i < header.node_instances_count; i++) {

        SerializedNodeInstance instance = {0};
        memcpy(&instance, cursor, header.node_instance_size);
        cursor += header.node_instance_size +
                  instance.input_port_size * instance.input_ports_count;

        NodeInstanceHandle instance_handle =
            instantiate(instance.name, instance.custom_data);
        if (!instance_handle)
            continue;

        node_instance_set_positioning(instance_handle, instance.positioning);
        id_to_instance_mapping[instance.id] = instance_handle;
    }

    // Network output port
    if (header.output.is_connected) {
        NodeInstanceHandle output_instance =
            id_to_instance_mapping[header.output.connected_node_id];

        OutputPortHandle output_port;
        if (!node_get_output(output_instance, header.output.connected_port_name,
                             &output_port)) {
            node_connect_to_output(output_instance, output_port);
        }
    }

    node_instance_set_positioning(INSTANCE_HANDLE_OUTPUT_PORT,
                                  header.output_positioning);

    cursor = buf + header_size;

    // Second pass, connections
    for (uint32_t i = 0; i < header.node_instances_count; i++) {

        SerializedNodeInstance instance = {0};
        memcpy(&instance, cursor, header.node_instance_size);
        cursor += header.node_instance_size;

        NodeInstanceHandle input_instance = id_to_instance_mapping[instance.id];

        for (uint32_t input_i = 0; input_i < instance.input_ports_count;
             input_i++) {

            SerializedInputPort input = {0};
            memcpy(&input, cursor, instance.input_port_size);
            cursor += instance.input_port_size;

            InputPortHandle input_port;
            if (node_get_input(input_instance, input.name, &input_port))
                continue;

            if (!input.is_connected) {
                node_set_manual_value(input_instance, input_port,
                                      input.manual_value);
                continue;
            }

            NodeInstanceHandle output_instance =
                id_to_instance_mapping[input.connected_node_id];
            if (!output_instance)
                continue;

            OutputPortHandle output_port;

            if (node_get_output(output_instance, input.connected_port_name,
                                &output_port))
                continue;

            node_connect(output_instance, output_port, input_instance,
                         input_port);
        }
    }
    return 0;
}

int patch_read(const char *file) {

    FILE *fp = fopen(file, "r");
    if (!fp) {
        ERROR("could not open file '%s' for reading", file);
        perror("");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t *file_buffer = malloc(file_size);
    if (!file_buffer) {
        ERROR("out of memory");
        fclose(fp);
        return 1;
    }
    if (!fread(file_buffer, file_size, 1, fp)) {
        ERROR("could not read file '%s'", file);
        perror("");
        fclose(fp);
        free(file_buffer);
        return 1;
    }

    int result = deserialize_patch(file_buffer, file_size);

    fclose(fp);
    free(file_buffer);
    return result;
}

void patch_set_directory(const char *directory) {

    // Make directory if it doesn't exist
    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        INFO("make %s", directory);
        mkdir(directory, 0700);
    }

    strncpy(patch_directory, directory, PATH_MAX - 2);
    if (patch_directory[strlen(patch_directory) - 1] != '/')
        strcat(patch_directory, "/");
}

static inline int get_file_path(const char *name, char *out_path,
                                uint32_t path_max) {

    uint32_t patch_directory_length = strlen(patch_directory);
    uint32_t total_length =
        strlen(name) + patch_directory_length + 1 + strlen(EXTENSION);
    if (total_length >= path_max)
        return 1;
    strcpy(out_path, patch_directory);
    strcat(out_path, name);
    strcat(out_path, EXTENSION);
    return 0;
}

int patch_save(const char *name) {

    INFO("saving patch '%s'", name);

    char file_path[PATH_MAX + 1] = {0};
    if (get_file_path(name, file_path, PATH_MAX))
        return 1;

    patch_write(file_path);
    return 0;
}

int patch_load(const char *name) {

    INFO("loading patch '%s'", name);

    char file_path[PATH_MAX + 1] = {0};
    if (get_file_path(name, file_path, PATH_MAX))
        return 1;

    patch_read(file_path);
    return 0;
}

StringVector patch_files_enumerate(void) {
    StringVector vec = stringvec_init();
    get_basenames_with_suffix(patch_directory, &vec, EXTENSION);
    return vec;
}

#include <stdlib.h>

// Simplest node plugin that can serve as a minimal example for documentation
// purposes.

// Include this in each node plugin you write.
// Provides some types and constants, as well as declaring the function
// prototypes you need to implement. See this file for the different functions
// you can implement in your plugin.
#include "common_header.h"

static OutputPortHandle out;
static InputPortHandle in;

// This plugin doesn't need state but for demonstration purposes we're including
// some. Basically you define this struct, which you then allocate and return
// the pointer to in the node_instantiate function. In the other functions it
// will be provided back to you as the `arg` void pointer.
typedef struct {
    uint8_t some_value;
} State;

// Where you instantiate the node instance.
void *node_instantiate(NodeInstanceHandle handle, uint8_t *out_height,
                       RegisterInputPortFunction register_input,
                       RegisterOutputPortFunction register_output,
                       uint8_t *custom_data) {

    // Here you register your input and output ports. The host gives function
    // pointers to do this, which return a handle which you'll need to store for
    // later. You could store the handles in the instance-specific state struct,
    // but as the handles are given out determenistically and are
    // instance-ambivalent, you can store them in common statically allocated
    // memory just fine.

    // register_output() takes in the opaque handle given by the host, as well
    // as the name.
    out = register_output(handle, "output");

    // register_output() takes in the handle, and a struct defining the input
    // port. Fields you'll need:
    //
    // typedef struct {
    //     char name[MAX_NAME];
    //     struct {
    //         float default_value;
    //         float min;
    //         float max;
    //     } manual;
    // } InputPort;
    in = register_input(
        handle,
        (InputPort){.name = "input",
                    .manual = {.default_value = 0, .min = 0, .max = 100}});

    // Write the desired height of the node in grid units into `out_height`.
    *out_height = 1;

    // Allocate your state struct on the heap and return the pointer from this
    // function.
    return calloc(1, sizeof(State));

    // The host gives 1024 bytes of `custom_data` if there are some values that
    // the nodes need to persist between patch saves and loads. Usually not
    // needed but if there is a dropdown or something this is useful to have.
    // The State struct is not saved along with a patch file.
    // See node_custom_data() in common_header.h for more info.
    (void)custom_data;
}

// In node_free, you'll uninitialize the things allocated for the node instance.
void node_free(void *arg) {
    free(arg);
}

// Your process function.
void node_process(void *arg, Info *info, InputBuffer *input_bufs,
                  float **output_bufs, uint32_t frame_count) {

    // You'll access state like this:
    // State *state = (State *)arg;

    // Use the handles stored in the node_instantiate function to write to the
    // correct input and output buffers like this:
    for (uint32_t i = 0; i < frame_count; i++)
        output_bufs[out][i] = INPUT_GET_VALUE(input_bufs[in], i);

    (void)arg;
    (void)info;
}

// For more examples see the other node plugins in this folder!

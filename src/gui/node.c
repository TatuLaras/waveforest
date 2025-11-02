//  TODO: BAD! refactor
#include "gui/node.h"
#include "clay.h"
#include "common.h"
#include "gui/defines.h"
#include "gui/dropdown.h"
#include "gui/gui_common.h"

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    Clay_BoundingBox box;
    int is_output;
    uint64_t id;
} PortPosition;
VEC_DECLARE(PortPosition, PortPositionVec, portposvec)
VEC_IMPLEMENT(PortPosition, PortPositionVec, portposvec)

static PortPositionVec port_positions = {0};

static struct {
    NodeInstanceHandle from_instance;
    PortHandle from_port;
    NodeInstanceHandle to_instance;
    PortHandle to_port;
    uint8_t is_connecting;
    uint8_t has_destination;
    // 0 = input to output , 1 = output to input
    uint8_t start_is_output;
} port_connecting = {0};

static inline void handle_port_connecting(int is_output,
                                          NodeInstanceHandle instance,
                                          PortHandle port,
                                          Clay_BoundingBox box) {

    int mouse_inside = is_mouse_inside_box(box, 0, 0);

    if (!port_connecting.is_connecting && mouse_inside &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

        port_connecting.from_port = port;
        port_connecting.from_instance = instance;
        port_connecting.start_is_output = is_output;
        port_connecting.is_connecting = 1;

        if (!is_output)
            node_disconnect(instance, port);
        return;
    }

    if (!port_connecting.is_connecting)
        return;
    if (!mouse_inside)
        return;
    if (port_connecting.start_is_output == is_output)
        return;
    if (port_connecting.from_instance == instance)
        return;

    port_connecting.to_port = port;
    port_connecting.to_instance = instance;
    port_connecting.has_destination = 1;
}

static inline void component_port_socket(int is_output, uint64_t port_id,
                                         NodeInstanceHandle instance,
                                         PortHandle port) {

    Clay_Color color = COLOR_BG_SOCKET;
    Clay_ElementId socket_id = unique_id("socket", port_id);

    CLAY({
        socket_id,
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(SOCKET_DIAMETER),
                           .height = CLAY_SIZING_FIXED(SOCKET_DIAMETER)},
            },
    }) {

        Clay_ElementData data = Clay_GetElementData(socket_id);
        if (data.found) {
            portposvec_append(&port_positions,
                              (PortPosition){.box = data.boundingBox,
                                             .id = port_id,
                                             .is_output = is_output});
            handle_port_connecting(is_output, instance, port, data.boundingBox);
            if (is_mouse_inside_box(data.boundingBox, 0, 0))
                color = (Clay_Color)COLOR_BG_SOCKET_HL;
        }

        CLAY({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                  .height = CLAY_SIZING_GROW(0)}},
            .cornerRadius = CLAY_CORNER_RADIUS(SOCKET_CORNER_RADIUS),
            .backgroundColor = color,
            .floating =
                {
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                },
        }) {}
        CLAY({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                  .height = CLAY_SIZING_GROW(0)}},
            .border =
                {
                    .color = color,
                    .width = is_output
                                 ? (Clay_BorderWidth){0, SOCKET_CORNER_RADIUS,
                                                      0, 0, 0}
                                 : (Clay_BorderWidth){SOCKET_CORNER_RADIUS, 0,
                                                      0, 0, 0},
                },
        }) {}
    }
}

// Returns 1 if slider is being controlled
static inline int component_port_slider(InputPort *port) {

    Clay_ElementId slider_id = unique_id("slider", (uint64_t)port);

    // Calculate mouse position inside slider
    Clay_ElementData el_data = Clay_GetElementData(slider_id);
    float mouse_x_percentage = 0;
    int is_mouse_inside_slider =
        is_mouse_inside_box(el_data.boundingBox, &mouse_x_percentage, 0);

    static Clay_ElementId slider_being_controlled_id = {0};
    static float value_at_control_start = 0;
    static float percentage_at_control_start = 0;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && is_mouse_inside_slider) {
        port->manual.value = port->manual.default_value;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && is_mouse_inside_slider) {
        slider_being_controlled_id = slider_id;
        value_at_control_start = port->manual.value;
        percentage_at_control_start = mouse_x_percentage;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        slider_being_controlled_id = (Clay_ElementId){0};

    int is_slider_controlled = slider_id.id == slider_being_controlled_id.id;
    float snap_interval = 1;
    int is_snap =
        IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(MOUSE_BUTTON_SIDE);
    int is_fine_control = 0;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsMouseButtonDown(MOUSE_BUTTON_EXTRA)) {
        if (is_snap)
            snap_interval /= 10;
        else
            is_fine_control = 1;
    }

    if (is_slider_controlled) {

        if (is_fine_control) {

            port->manual.value =
                value_at_control_start +
                S(0.1) * (mouse_x_percentage - percentage_at_control_start);
            port->manual.value = min(port->manual.value, port->manual.max);
            port->manual.value = max(port->manual.value, port->manual.min);
        } else {

            mouse_x_percentage = clamp(mouse_x_percentage, 0, 1);
            port->manual.value =
                port->manual.min +
                (mouse_x_percentage * (port->manual.max - port->manual.min));
            if (is_snap)
                port->manual.value =
                    ceil(port->manual.value * (1 / snap_interval)) /
                    (1 / snap_interval);
        }
    }

    float percentage = (port->manual.value - port->manual.min) /
                       (port->manual.max - port->manual.min);

    CLAY({
        slider_id,
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {S(4), 0, 0, 0}),
            },
        .floating = {.attachTo = CLAY_ATTACH_TO_PARENT, .zIndex = 0},
    }) {

        if (is_slider_controlled) {

            static char number_chars[64] = {0};
            snprintf(number_chars, 63, "%.1f < %.3f < %.1f", port->manual.min,
                     port->manual.value, port->manual.max);
            Clay_String number = {.isStaticallyAllocated = 1,
                                  .chars = number_chars,
                                  .length = strlen(number_chars)};
            // Number
            CLAY({
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_FIT(0),
                                   .height = CLAY_SIZING_GROW(0)},
                    },
                .floating =
                    {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .zIndex = 0,
                        .attachPoints =
                            {
                                .parent = CLAY_ATTACH_POINT_CENTER_TOP,
                                .element = CLAY_ATTACH_POINT_CENTER_TOP,
                            },
                    },
            }) {
                CLAY_TEXT(number, CLAY_TEXT_CONFIG({
                                      .fontSize = FONT_SIZE,
                                      .textColor = COLOR_TEXT,
                                  }));
            }
        }
        CLAY({
            .layout =
                {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_GROW(0)},
                },
            .cornerRadius = CLAY_CORNER_RADIUS(S(2)),
            .backgroundColor = COLOR_BG_SOCKET,
        }) {
            CLAY({
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_PERCENT(percentage),
                                   .height = CLAY_SIZING_GROW(0)},
                    },
                .cornerRadius = CLAY_CORNER_RADIUS(S(2)),
                .backgroundColor = COLOR_SLIDER,
            }) {}
        }
    }

    return is_slider_controlled;
}

static inline void component_output(OutputPort *port,
                                    NodeInstanceHandle instance,
                                    PortHandle port_handle) {
    Clay_String port_name = {.isStaticallyAllocated = 1,
                             .chars = port->name,
                             .length = strlen(port->name)};

    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_FIT(0)},
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {S(6), 0, 0, 0}),
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
            },
        .cornerRadius = CLAY_CORNER_RADIUS(S(2)),

    }) {

        CLAY({
            .layout =
                {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_FIT(0)},
                    .padding =
                        CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 0, 0, S(2)}),
                },
        }) {
            CLAY({
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_GROW(0),
                                   .height = CLAY_SIZING_GROW(0)},
                        .padding =
                            CLAY__CONFIG_WRAPPER(Clay_Padding, {S(0), 0, 0, 0}),
                        .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                    },
            }) {
                CLAY_TEXT(port_name, CLAY_TEXT_CONFIG({
                                         .fontSize = FONT_SIZE,
                                         .textColor = COLOR_TEXT,
                                         .textAlignment = CLAY_TEXT_ALIGN_RIGHT,
                                     }));
            }

            component_port_socket(1, (uint64_t)port, instance, port_handle);
        }
    }
}

static inline void component_input(InputPort *port, NodeInstanceHandle instance,
                                   PortHandle port_handle) {
    Clay_String port_name = {.isStaticallyAllocated = 1,
                             .chars = port->name,
                             .length = strlen(port->name)};
    Clay_String empty = {.isStaticallyAllocated = 1, .chars = "-", .length = 1};
    int is_port_connected = port->connection.output_instance > 0;

    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_FIT(0)},
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {0, S(6), 0, 0}),
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
            },
        .cornerRadius = CLAY_CORNER_RADIUS(S(2)),

    }) {
        component_port_socket(0, (uint64_t)port, instance, port_handle);

        CLAY({
            .layout =
                {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_FIT(0)},
                    .padding =
                        CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 0, 0, S(2)}),
                },
        }) {
            // Reserve correct amount of space for text
            CLAY_TEXT(empty, CLAY_TEXT_CONFIG({
                                 .fontSize = FONT_SIZE,
                                 .textColor = {0, 0, 0, 0},
                             }));

            int hide_text = 0;

            if (!is_port_connected)
                hide_text = hide_text || component_port_slider(port);

            if (!hide_text)
                CLAY({
                    .layout =
                        {
                            .sizing = {.width = CLAY_SIZING_GROW(0),
                                       .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY__CONFIG_WRAPPER(
                                Clay_Padding,
                                {is_port_connected ? S(8) : S(10), 0, 0, 0}),
                            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                        },
                    .floating = {.attachTo = CLAY_ATTACH_TO_PARENT,
                                 .zIndex = 1},
                }) {
                    CLAY_TEXT(port_name, CLAY_TEXT_CONFIG({
                                             .fontSize = FONT_SIZE,
                                             .textColor = COLOR_TEXT,
                                         }));
                }
        }
    }
}

void component_node_finish_update(void) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        port_connecting.is_connecting) {

        port_connecting.is_connecting = 0;

        if (!port_connecting.has_destination)
            return;

        // Input to output
        if (port_connecting.start_is_output)
            node_connect(port_connecting.from_instance,
                         port_connecting.from_port, port_connecting.to_instance,
                         port_connecting.to_port);
        else
            node_connect(port_connecting.to_instance, port_connecting.to_port,
                         port_connecting.from_instance,
                         port_connecting.from_port);
    }

    port_connecting.to_port = 0;
    port_connecting.to_instance = 0;
    port_connecting.has_destination = 0;

    port_positions.data_used = 0;
}

static inline void component_network_output(Vector2 screen_top_left,
                                            int *out_is_title_hovered) {

    if (!port_positions.data)
        port_positions = portposvec_init();

    NodeInstance *node_instance =
        node_instance_get(INSTANCE_HANDLE_OUTPUT_PORT);
    if (!node_instance)
        return;

    Clay_ElementId node_id = CLAY_ID("output_node");
    Clay_ElementId node_title_id = CLAY_ID("output_node_title");

    CLAY({
        node_id,
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(GRID_UNIT),
                           .height = CLAY_SIZING_FIXED(GRID_UNIT)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding =
                    CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 0, S(4), S(8)}),
            },
        .floating =
            {.attachTo = CLAY_ATTACH_TO_PARENT,
             .offset =
                 {
                     .x = (node_instance->positioning.x - screen_top_left.x) *
                          GRID_UNIT,
                     .y = (node_instance->positioning.y - screen_top_left.y) *
                          GRID_UNIT,
                 }},
        .backgroundColor = COLOR_BG_1,
        .cornerRadius = CLAY_CORNER_RADIUS(S(4)),
    }) {
        // Node title
        CLAY({node_title_id, .layout = {
                                 .sizing = {.width = CLAY_SIZING_GROW(0),
                                            .height = CLAY_SIZING_FIT(0)},
                                 .padding = CLAY__CONFIG_WRAPPER(
                                     Clay_Padding, {S(8), 0, 0, S(11)}),
                             }}) {

            Clay_ElementData data = Clay_GetElementData(node_title_id);
            if (data.found && is_mouse_inside_box(data.boundingBox, 0, 0) &&
                out_is_title_hovered)
                *out_is_title_hovered = 1;

            CLAY_TEXT(CLAY_STRING("output"),
                      CLAY_TEXT_CONFIG(
                          {.fontSize = FONT_SIZE, .textColor = COLOR_TEXT}));
        }
        CLAY({.layout = {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_GROW(0)},
                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
                  .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
              }}) {

            component_port_socket(0, 0, INSTANCE_HANDLE_OUTPUT_PORT, 0);
        }
    }
}

void component_node(NodeInstanceHandle instance, Vector2 screen_top_left,
                    int *out_is_title_hovered) {

    if (instance == INSTANCE_HANDLE_OUTPUT_PORT) {
        component_network_output(screen_top_left, out_is_title_hovered);
        return;
    }

    if (!port_positions.data)
        port_positions = portposvec_init();

    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance || node_instance->is_deleted)
        return;

    Node *node = node_get(node_instance->node);
    if (!node)
        return;

    Clay_String name = {
        .isStaticallyAllocated = 1,
        .chars = node->name,
        .length = strlen(node->name),
    };

    InputPortVec inputs = node_get_inputs(instance);
    OutputPortVec outputs = node_get_outputs(instance);

    Clay_ElementId node_id = unique_id("node", instance);
    Clay_ElementId node_title_id = unique_id("nodetitle", instance);

    CLAY({
        node_id,
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(NODE_WIDTH * GRID_UNIT),
                           .height =
                               node_instance->height
                                   ? CLAY_SIZING_FIXED(node_instance->height *
                                                       GRID_UNIT)
                                   : CLAY_SIZING_FIT(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding =
                    CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 0, S(4), S(8)}),
            },
        .floating =
            {.attachTo = CLAY_ATTACH_TO_PARENT,
             .offset =
                 {
                     .x = (node_instance->positioning.x - screen_top_left.x) *
                          GRID_UNIT,
                     .y = (node_instance->positioning.y - screen_top_left.y) *
                          GRID_UNIT,
                 }},
        .backgroundColor = COLOR_BG_1,
        .cornerRadius = CLAY_CORNER_RADIUS(S(4)),
    }) {
        // Node title
        CLAY({node_title_id, .layout = {
                                 .sizing = {.width = CLAY_SIZING_GROW(0),
                                            .height = CLAY_SIZING_FIT(0)},
                                 .padding = CLAY__CONFIG_WRAPPER(
                                     Clay_Padding, {S(8), 0, 0, S(11)}),
                             }}) {

            Clay_ElementData data = Clay_GetElementData(node_title_id);
            if (data.found && is_mouse_inside_box(data.boundingBox, 0, 0) &&
                out_is_title_hovered)
                *out_is_title_hovered = 1;

            CLAY_TEXT(name, CLAY_TEXT_CONFIG({.fontSize = FONT_SIZE,
                                              .textColor = COLOR_TEXT}));
        }

        CLAY({.layout = {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(0)},
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              }}) {

            // Left column for input ports
            CLAY({
                .layout =
                    {
                        .sizing =
                            {
                                .width = CLAY_SIZING_PERCENT(2.0 / 3),
                                .height = CLAY_SIZING_FIT(0),
                            },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childGap = S(3),
                    },
            }) {

                for (uint32_t i = 0; i < inputs.data_used; i++) {
                    InputPort *input = inputs.data + i;
                    component_input(input, instance, i);
                }

                GAP(S(12));
                // Node's own UI
                if (node->functions.gui)
                    node->functions.gui(node_instance->arg,
                                        (Draw){
                                            .dropdown = component_dropdown,
                                        });
            }

            // Right column for output ports
            CLAY({
                .layout =
                    {
                        .sizing =
                            {
                                .width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIT(0),
                            },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childGap = S(3),
                    },
            }) {

                for (uint32_t i = 0; i < outputs.data_used; i++) {
                    OutputPort *output = outputs.data + i;
                    component_output(output, instance, i);
                }
            }
        }
    }
}

static inline void draw_connection(PortPosition *input_pos,
                                   PortPosition *output_pos) {
    Color color = (Color)COLOR_SLIDER;

    float padding = S(2);

    Vector2 start = {
        .x = output_pos->box.x + output_pos->box.width * 0.5,
        .y = output_pos->box.y + output_pos->box.height * 0.5,
    };
    Vector2 end = {
        .x = input_pos->box.x + input_pos->box.width * 0.5,
        .y = input_pos->box.y + input_pos->box.height * 0.5,
    };

    DrawLineEx(start, end, 4, color);

    float radius = SOCKET_DIAMETER / 2.0 - padding;
    DrawCircleV(start, radius, color);
    DrawCircleV(end, radius, color);
}

static inline void draw_pending_connection(PortPosition *pos) {
    Color color = (Color)COLOR_SLIDER;

    float padding = S(2);

    Vector2 start = {
        .x = pos->box.x + pos->box.width * 0.5,
        .y = pos->box.y + pos->box.height * 0.5,
    };

    DrawLineEx(start, GetMousePosition(), 4, color);

    float radius = SOCKET_DIAMETER / 2.0 - padding;
    DrawCircleV(start, radius, color);
}

// Should always come after component_node()
void component_node_connections(NodeInstanceHandle instance) {

    if (!port_positions.data || port_positions.data_used == 0)
        return;

    InputPortVec inputs = node_get_inputs(instance);

    if (port_connecting.is_connecting &&
        instance == port_connecting.from_instance) {

        uint64_t port_id = 0;
        if (port_connecting.start_is_output) {
            OutputPortVec outs = node_get_outputs(instance);
            port_id = (uint64_t)(outs.data + port_connecting.from_port);
        } else if (instance != INSTANCE_HANDLE_OUTPUT_PORT) {
            port_id = (uint64_t)(inputs.data + port_connecting.from_port);
        }

        for (uint32_t pos_i = 0; pos_i < port_positions.data_used; pos_i++) {

            PortPosition *pos = port_positions.data + pos_i;

            if (pos->is_output != port_connecting.start_is_output)
                continue;

            if (pos->id == port_id) {
                draw_pending_connection(pos);
                break;
            }
        }
    }

    if (instance == INSTANCE_HANDLE_OUTPUT_PORT) {

        NodeInstanceHandle output;
        OutputPortHandle port;
        if (node_get_outputting_node_instance(&output, &port))
            return;

        OutputPortVec outputs = node_get_outputs(output);
        if (port >= outputs.data_used)
            return;

        PortPosition *input_pos = 0;
        PortPosition *output_pos = 0;
        for (uint32_t pos_i = 0; pos_i < port_positions.data_used; pos_i++) {

            PortPosition *pos = port_positions.data + pos_i;

            if (pos->is_output && pos->id == (uint64_t)(outputs.data + port))
                output_pos = pos;

            if (!pos->is_output && pos->id == 0)
                input_pos = pos;
        }

        if (input_pos && output_pos)
            draw_connection(input_pos, output_pos);

        return;
    }

    OutputPortVec outputs;
    OutputPort *output;
    InputPort *input;

    for (uint32_t i = 0; i < inputs.data_used; i++) {
        input = inputs.data + i;

        outputs = node_get_outputs(input->connection.output_instance);
        if (input->connection.output_port >= outputs.data_used)
            continue;

        output = outputs.data + input->connection.output_port;

        PortPosition *input_pos = 0;
        PortPosition *output_pos = 0;

        for (uint32_t pos_i = 0; pos_i < port_positions.data_used; pos_i++) {

            PortPosition *pos = port_positions.data + pos_i;

            if (pos->is_output && pos->id == (uint64_t)output)
                output_pos = pos;

            if (!pos->is_output && pos->id == (uint64_t)input)
                input_pos = pos;
        }

        if (input_pos && output_pos)
            draw_connection(input_pos, output_pos);
    }
}

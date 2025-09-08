#include "gui/node.h"
#include "clay.h"
#include "common.h"
#include "gui/defines.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

static inline void component_port_socket(int connected) {
    const Clay_Color fill_color =
        connected ? COLOR_CONNECTION : COLOR_BG_SOCKET;

    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(SOCKET_DIAMETER),
                           .height = CLAY_SIZING_FIXED(SOCKET_DIAMETER)},
            },
    }) {
        CLAY({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                  .height = CLAY_SIZING_GROW(0)}},
            .cornerRadius = CLAY_CORNER_RADIUS(SOCKET_CORNER_RADIUS),
            .backgroundColor = fill_color,
            .floating = {.attachTo = CLAY_ATTACH_TO_PARENT},
        }) {}
        CLAY({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                  .height = CLAY_SIZING_GROW(0)}},
            .border =
                {
                    .color = fill_color,
                    .width = {SOCKET_CORNER_RADIUS, 0, 0, 0, 0},
                },
        }) {}
    }
}

// Returns 1 if slider is being controlled
static inline int component_port_slider(InputPort *port) {

    // Generate id for this specific slider
    char id_chars[64] = {0};
    snprintf(id_chars, 64, "%zu", (uint64_t)port);
    Clay_ElementId slider_id =
        Clay_GetElementId((Clay_String){.isStaticallyAllocated = 1,
                                        .chars = id_chars,
                                        .length = strlen(id_chars)});

    // Calculate mouse position inside slider
    Clay_ElementData el_data = Clay_GetElementData(slider_id);
    float mouse_x_percentage =
        (GetMouseX() - el_data.boundingBox.x) / el_data.boundingBox.width;
    int is_mouse_inside_slider =
        el_data.boundingBox.y <= GetMouseY() &&
        GetMouseY() - el_data.boundingBox.y <= el_data.boundingBox.height &&
        mouse_x_percentage <= 1.0 && mouse_x_percentage >= 0.0;

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
    float snap_interval = 0.1;
    int is_snap = IsKeyDown(KEY_LEFT_CONTROL);
    int is_fine_control = 0;
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (is_snap)
            snap_interval = 0.01;
        else
            is_fine_control = 1;
    }

    // Clamp

    if (is_slider_controlled) {
        if (is_fine_control) {
            port->manual.value =
                value_at_control_start +
                0.1 * (mouse_x_percentage - percentage_at_control_start);
            if (port->manual.value > port->manual.max)
                port->manual.value = port->manual.max;
            if (port->manual.value < port->manual.min)
                port->manual.value = port->manual.min;
        } else {
            mouse_x_percentage = (min(max(0.0, mouse_x_percentage), 1.0));

            port->manual.value = mouse_x_percentage * port->manual.max;
            if (is_snap)
                port->manual.value -= fmod(port->manual.value, snap_interval);
        }
    }

    float percentage = port->manual.value / port->manual.max;

    CLAY({
        slider_id,
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {4, 0, 0, 0}),
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
            .cornerRadius = CLAY_CORNER_RADIUS(2),
            .backgroundColor = COLOR_BG_SOCKET,
        }) {
            CLAY({
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_PERCENT(percentage),
                                   .height = CLAY_SIZING_GROW(0)},
                    },
                .cornerRadius = CLAY_CORNER_RADIUS(2),
                .backgroundColor = COLOR_SLIDER,
            }) {}
        }
    }

    return is_slider_controlled;
}

static inline void component_port(InputPort *port) {
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
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 6, 0, 0}),
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
            },
        .cornerRadius = CLAY_CORNER_RADIUS(2),

    }) {
        component_port_socket(is_port_connected);

        CLAY({
            .layout =
                {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 0, 0, 2}),
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
                                {is_port_connected ? 8 : 10, 0, 0, 0}),
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

void component_node(NodeInstanceHandle instance) {
    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance)
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

    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(200),
                           .height = CLAY_SIZING_FIT(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {0, 4, 4, 8}),
                .childGap = 3,
            },
        .backgroundColor = COLOR_BG_1,
        .cornerRadius = CLAY_CORNER_RADIUS(4),
    }) {
        // Node title
        CLAY({.layout = {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(0)},
                  .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {8, 0, 0, 8}),
              }}) {
            CLAY_TEXT(name, CLAY_TEXT_CONFIG({.fontSize = FONT_SIZE,
                                              .textColor = COLOR_TEXT}));
        }

        for (uint32_t i = 0; i < inputs.data_used; i++) {
            InputPort *input = inputs.data + i;
            component_port(input);
        }
    }
}

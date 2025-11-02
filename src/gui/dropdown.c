#include "gui/dropdown.h"
#include "clay.h"
#include "gui/defines.h"
#include "gui/gui_common.h"
#include <raylib.h>
#include <raymath.h>
#include <string.h>

static StringVector *open_dropdown = 0;
static int open_dropdown_changed = 0;

void component_dropdown(StringVector *options, uint16_t *selected_index) {

    char *selected_name = stringvec_get(options, *selected_index);
    if (!selected_name)
        return;

    CLAY({.layout = {
              .sizing = {.width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_FIT(0)},
              .padding = CLAY__CONFIG_WRAPPER(
                  Clay_Padding, {SOCKET_DIAMETER + S(4), S(6), 0, 0}),
          }}) {

        Clay_ElementId box_id = unique_id("dropdown", (uint64_t)options);
        Clay_ElementData data = Clay_GetElementData(box_id);

        int selected_hover =
            data.found && is_mouse_inside_box(data.boundingBox, 0, 0);

        if (selected_hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            open_dropdown != options) {

            open_dropdown = options;
            open_dropdown_changed = 1;
        }

        CLAY({
            box_id,
            .layout =
                {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_FIT(0)},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                    .padding = CLAY__CONFIG_WRAPPER(Clay_Padding,
                                                    {S(6), 0, S(4), S(4)}),
                },
            .cornerRadius = CLAY_CORNER_RADIUS(S(2)),
            .backgroundColor = selected_hover ? (Clay_Color)COLOR_BG_DROPDOWN
                                              : (Clay_Color)COLOR_BG_SOCKET,

        }) {

            Clay_String selected_item_name = {
                .chars = selected_name,
                .length = strlen(selected_name),
            };

            CLAY_TEXT(selected_item_name, CLAY_TEXT_CONFIG({
                                              .fontSize = FONT_SIZE,
                                              .textColor = COLOR_TEXT,
                                          }));
        }

        if (open_dropdown == options) {

            CLAY({
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_GROW(0),
                                   .height = CLAY_SIZING_FIT(0)},
                        .padding = CLAY__CONFIG_WRAPPER(
                            Clay_Padding, {SOCKET_DIAMETER + S(4), S(6), 0, 0}),
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .floating =
                    {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
                        .attachPoints =
                            {
                                .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                            },
                    },
            }) {

                char *name = 0;
                uint32_t i = 0;

                while ((name = stringvec_get(options, i))) {

                    if (i == *selected_index) {
                        i++;
                        continue;
                    }

                    Clay_ElementId option_id =
                        unique_id("dropdownoption", (uint64_t)name);
                    Clay_ElementData option_data =
                        Clay_GetElementData(option_id);

                    int option_hover =
                        option_data.found &&
                        is_mouse_inside_box(option_data.boundingBox, 0, 0);

                    if (option_hover &&
                        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        open_dropdown_changed = 0;
                        open_dropdown = 0;
                        *selected_index = i;
                        break;
                    }

                    CLAY({
                        option_id,
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0),
                                           .height = CLAY_SIZING_FIT(0)},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                                .padding = CLAY__CONFIG_WRAPPER(
                                    Clay_Padding, {S(6), 0, S(4), S(4)}),
                            },
                        .cornerRadius = CLAY_CORNER_RADIUS(S(2)),
                        .backgroundColor = option_hover
                                               ? (Clay_Color)COLOR_BG_DROPDOWN
                                               : (Clay_Color)COLOR_BG_SOCKET,

                    }) {

                        Clay_String selected_item_name = {
                            .chars = name,
                            .length = strlen(name),
                        };

                        CLAY_TEXT(selected_item_name,
                                  CLAY_TEXT_CONFIG({
                                      .fontSize = FONT_SIZE,
                                      .textColor = COLOR_TEXT,
                                  }));
                    }

                    i++;
                }
            }
        }
    }
}

void component_dropdown_end_of_frame(void) {

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !open_dropdown_changed)
        open_dropdown = 0;

    open_dropdown_changed = 0;
}

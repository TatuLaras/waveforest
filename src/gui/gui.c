#include "clay.h"
#include "common.h"
#include "gui/defines.h"
#include "gui/dropdown.h"
#include "gui/fuzzy_picker.h"
#include "gui/gui_common.h"
#include "gui/node.h"
#include "gui/text_prompt.h"
#include "node_files.h"
#include "node_manager.h"
#include "notify_user.h"
#include "patch_files.h"
#include "raylib_renderer.h"
#include "shortcuts.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

float scale = 1.0;

enum {
    FONT_ID_BASE,
};
static Font fonts[1] = {0};
static uint8_t empty_custom_data[MAX_CUSTOM_DATA] = {0};

static struct {
    Vector2 screen_pos;
    int is_panning_in_progress;
    Vector2 pan_start_screen_pos;
} panning = {0};

static struct {
    NodeInstanceHandle instance;
    int is_moving;
} node_moving;

static int show_help_message = 1;

static NodeInstanceHandle hovered_node = 0;

static char patch_name[MAX_NAME + 1] = {0};

static void error_handler(Clay_ErrorData errorData) {

    printf("%s\n", errorData.errorText.chars);
}

static inline int overlaps(int a_start, int a_length, int b_start,
                           int b_length) {

    return a_start < b_start + b_length && b_start < a_start + a_length;
}

static inline void make_not_overlap(NodeInstanceHandle instance) {

    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance)
        return;

    uint32_t node_height = 1;
    if (node_instance->height)
        node_height = node_instance->height;

    NodeInstance *other;
    int overlapped = 1;

    while (overlapped) {

        overlapped = 0;

        for (uint32_t i = 0; (other = node_instance_get(i)); i++) {

            if (i == instance || other->is_deleted)
                continue;

            uint8_t node_width = instance ? NODE_WIDTH : 1;
            uint8_t other_width = i ? NODE_WIDTH : 1;
            if (!overlaps(node_instance->positioning.x, node_width,
                          other->positioning.x, other_width))
                continue;

            uint32_t other_height = 1;
            if (other->height)
                other_height = other->height;

            while (overlaps(node_instance->positioning.y, node_height,
                            other->positioning.y, other_height)) {
                node_instance->positioning.y++;
                overlapped = 1;
            }
        }
    }
}

static inline void draw_background_grid(void) {

    ClearBackground((Color)COLOR_BG_0);

    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();
    float offset_x = fmod(panning.screen_pos.x, 1.0) * GRID_UNIT;
    float offset_y = fmod(panning.screen_pos.y, 1.0) * GRID_UNIT;

    for (uint32_t x = 0; x <= screen_width + GRID_UNIT; x += GRID_UNIT)
        DrawLineEx((Vector2){x - offset_x, 0},
                   (Vector2){x - offset_x, screen_height}, 1,
                   (Color)COLOR_BG_1);
    for (uint32_t y = 0; y <= screen_height + GRID_UNIT; y += GRID_UNIT)
        DrawLineEx((Vector2){0, y - offset_y},
                   (Vector2){screen_width, y - offset_y}, 1, (Color)COLOR_BG_1);

    if (!show_help_message)
        return;

    char *help_message = "\
h to show this message\n\
Mouse3 to pan around\n\
Scroll to zoom\n\
Space to add node\n\
Delete to delete node\n\
Ctrl+s to save patch\n\
Ctrl+o to open patch\n\
Ctrl+n for new patch\n\
";
    DrawTextEx(fonts[FONT_ID_BASE], help_message, (Vector2){14, 14},
               FONT_SIZE_FIXED, 0, (Color)COLOR_BG_2);
}

static inline Vector2 get_mouse_world_pos(void) {
    Vector2 mouse_pos = {
        .x = GetMouseX() / GRID_UNIT,
        .y = GetMouseY() / GRID_UNIT,
    };
    Vector2 world_pos = panning.screen_pos;
    world_pos = Vector2Add(world_pos, mouse_pos);
    return world_pos;
}

static inline void handle_zoom(void) {

    float mousewheel = GetMouseWheelMove();

    Vector2 mouse_before = get_mouse_world_pos();

    if (mousewheel < 0) {
        scale -= 0.1;
        show_help_message = 0;
    } else if (mousewheel > 0) {
        scale += 0.1;
        show_help_message = 0;
    }
    scale = max(scale, 0.2);

    Vector2 mouse_after = get_mouse_world_pos();
    panning.screen_pos = Vector2Add(panning.screen_pos,
                                    Vector2Subtract(mouse_before, mouse_after));
}

static inline void handle_panning(void) {

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        panning.screen_pos = Vector2Add(
            panning.screen_pos,
            Vector2Multiply(GetMouseDelta(),
                            (Vector2){-1 / GRID_UNIT, -1 / GRID_UNIT}));
        show_help_message = 0;
    }
}

static inline void handle_node_moving(void) {

    if ((IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ||
         !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) &&
        node_moving.is_moving) {

        make_not_overlap(node_moving.instance);
        node_moving.is_moving = 0;
    }

    if (!node_moving.is_moving)
        return;

    show_help_message = 0;
    Vector2 mouse_world_pos = get_mouse_world_pos();
    NodeInstance *instance = node_instance_get(node_moving.instance);
    if (!instance)
        return;
    instance->positioning.x = floor(mouse_world_pos.x);
    instance->positioning.y = floor(mouse_world_pos.y);
    make_not_overlap(node_moving.instance);
}

static inline void instantiate_node_at_mouse(NodeHandle handle) {

    NodeInstanceHandle instance = node_instantiate(handle, empty_custom_data);
    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance)
        return;

    Vector2 mouse_world_pos = get_mouse_world_pos();
    node_instance->positioning.x = floor(mouse_world_pos.x);
    node_instance->positioning.y = floor(mouse_world_pos.y);
    make_not_overlap(instance);
}

static inline void init_view(void) {

    scale = 1.0;
    panning.screen_pos = (Vector2){0};
}

static inline void accept_node_picking_result(char *name) {

    NodeHandle existing;
    if (node_get_by_name(name, &existing)) {
        instantiate_node_at_mouse(existing);
        return;
    }

    Node node;
    if (node_files_load(name, &node))
        return;

    NodeHandle handle = node_new(node);
    instantiate_node_at_mouse(handle);
}

static inline void accept_patch_picking_result(char *name) {

    init_view();
    patch_load(name);
    strncpy(patch_name, name, MAX_NAME);
}

static inline void accept_patch_name_and_save(char *name) {

    strncpy(patch_name, name, MAX_NAME);
    patch_save(patch_name);
}

static inline void new_patch(void) {

    init_view();
    node_reset();
    patch_name[0] = 0;
}

static inline void handle_shortcuts(void) {

    int was_none = 0;
    switch (shortcuts_get_action(GetKeyPressed(), IsKeyDown(KEY_LEFT_SHIFT),
                                 IsKeyDown(KEY_LEFT_CONTROL),
                                 IsKeyDown(KEY_LEFT_ALT))) {

    case ACTION_SAVE_PATCH: {
        if (!(*patch_name))
            text_prompt("patch name: ", accept_patch_name_and_save);
        else
            patch_save(patch_name);
    } break;

    case ACTION_SAVE_PATCH_AS:
        text_prompt("patch name: ", accept_patch_name_and_save);
        break;

    case ACTION_LOAD_PATCH:
        fuzzy_picker_start_picking("load patch: ", patch_files_enumerate(),
                                   accept_patch_picking_result);
        break;

    case ACTION_DELETE_NODE_INSTANCE:
        node_instance_destroy(hovered_node);
        break;

    case ACTION_PICK_NODE:
        fuzzy_picker_start_picking("add node: ", node_files_enumerate(),
                                   accept_node_picking_result);
        break;

    case ACTION_NEW_PATCH:
        new_patch();
        break;

    case ACTION_TOGGLE_HELP:
        was_none = 1;
        show_help_message = !show_help_message;
        break;

    case ACTION_NONE:
        was_none = 1;
        break;
    }

    if (!was_none)
        show_help_message = 0;
}

static inline void handle_inputs(void) {

    if (fuzzy_picker_handle_inputs())
        return;
    if (text_prompt_handle_inputs())
        return;

    handle_zoom();
    handle_panning();
    handle_node_moving();
    handle_shortcuts();
}

void gui_deinit(void) {
    uirend_deinit();
}

void gui_init(void) {

    uirend_init();

    fonts[FONT_ID_BASE] =
        LoadFont(RESOURCE_PATH "fonts/MonaspaceArgon-SemiBold.otf");

    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    uint64_t size = Clay_MinMemorySize();
    Clay_Arena arena =
        Clay_CreateArenaWithCapacityAndMemory(size, malloc(size));

    Clay_Initialize(arena, (Clay_Dimensions){screen_width, screen_height},
                    (Clay_ErrorHandler){error_handler, 0});

    Clay_SetMeasureTextFunction(&uirend_measure_text, fonts);
}

static inline void notifications_render(void) {

    CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                     .layoutDirection = CLAY_TOP_TO_BOTTOM}}) {

        nu_get_notification_begin();
        char *message;
        while ((message = nu_get_notification_iterate())) {

            Clay_ElementId id = unique_id("msg", (uint64_t)message);
            Clay_ElementData data = Clay_GetElementData(id);
            int is_mouse_over =
                data.found && is_mouse_inside_box(data.boundingBox, 0, 0);

            if (is_mouse_over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                nu_dismiss(message);

            // Notify box
            CLAY({
                id,
                .layout =
                    {
                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                        .padding =
                            CLAY__CONFIG_WRAPPER(Clay_Padding, {14, 8, 8, 8}),
                    },
                .backgroundColor = is_mouse_over ? (Clay_Color)COLOR_SLIDER
                                                 : (Clay_Color)COLOR_BG_1,
                .border = {.width = {0, 0, 0, 1, 0}, .color = COLOR_BG_0},
            }) {
                Clay_String msg = {
                    .isStaticallyAllocated = 1,
                    .chars = message,
                    .length = strlen(message),
                };
                CLAY_TEXT(msg, CLAY_TEXT_CONFIG({
                                   .fontSize = FONT_SIZE_FIXED,
                                   .textColor = COLOR_TEXT,
                               }));
            }
        }
    }
}

static inline void render_overlays(void) {

    if (fuzzy_picker_render())
        return;
    if (text_prompt_render())
        return;

    notifications_render();
}

void gui_main(void) {
    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    while (uirend_loop()) {

        handle_inputs();
        hovered_node = 0;

        if (IsWindowResized()) {
            screen_width = GetScreenWidth();
            screen_height = GetScreenHeight();
            Clay_SetLayoutDimensions(
                (Clay_Dimensions){screen_width, screen_height});
        }

        Clay_UpdateScrollContainers(1, (Clay_Vector2){0, GetMouseWheelMove()},
                                    GetFrameTime());

        Clay_SetPointerState((Clay_Vector2){GetMouseX(), GetMouseY()},
                             IsMouseButtonDown(MOUSE_BUTTON_LEFT));

        Clay_BeginLayout();

        CLAY({
            .id = CLAY_ID("OuterContainer"),
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 8,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
        }) {

            uint32_t i = 0;
            NodeInstance *instance;
            int is_instance_title_hovered;

            while ((instance = node_instance_get(i))) {

                is_instance_title_hovered = 0;
                component_node(i, panning.screen_pos,
                               &is_instance_title_hovered);

                if (is_instance_title_hovered) {

                    hovered_node = i;

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                        !node_moving.is_moving) {
                        node_moving.instance = i;
                        node_moving.is_moving = 1;
                    }
                }

                i++;
            }
        }

        Clay_RenderCommandArray render_commands = Clay_EndLayout();

        BeginDrawing();

        draw_background_grid();
        uirend_render(render_commands, fonts);

        // Connections
        uint32_t i = 0;
        NodeInstance *instance;
        while ((instance = node_instance_get(i))) {
            component_node_connections(i);
            i++;
        }
        component_node_finish_update();

        Clay_UpdateScrollContainers(1, (Clay_Vector2){0, GetMouseWheelMove()},
                                    GetFrameTime());

        Clay_SetPointerState((Clay_Vector2){GetMouseX(), GetMouseY()},
                             IsMouseButtonDown(MOUSE_BUTTON_LEFT));

        // Overlaying UI
        Clay_BeginLayout();

        render_overlays();

        Clay_RenderCommandArray overlay_render_commands = Clay_EndLayout();
        uirend_render(overlay_render_commands, fonts);

        component_dropdown_end_of_frame();
        EndDrawing();
    }
}

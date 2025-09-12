#include "clay.h"
#include "common.h"
#include "gui/defines.h"
#include "gui/node.h"
#include "node_files.h"
#include "node_manager.h"
#include "raylib_renderer.h"
#include "string_vector.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

#define MAX_NODE_RESULTS 10

float scale = 1.0;

enum {
    FONT_ID_BASE,
};
static Font fonts[1] = {0};

static struct {
    Vector2 screen_pos;
    int is_panning_in_progress;
    Vector2 pan_start_screen_pos;
} panning = {0};

static struct {
    int is_picking;
    uint32_t search_buf_used;
    uint32_t selected_result;
    char search_buf[MAX_NAME];
    StringVector results;
} node_picking = {0};

static struct {
    NodeInstanceHandle instance;
    int is_moving;
} node_moving;

void error_handler(Clay_ErrorData errorData) {
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
    for (uint32_t i = 0; (other = node_instance_get(i)); i++) {

        if (i == instance)
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

    if (mousewheel < 0)
        scale -= 0.1;
    else if (mousewheel > 0)
        scale += 0.1;
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

    Vector2 mouse_world_pos = get_mouse_world_pos();
    NodeInstance *instance = node_instance_get(node_moving.instance);
    if (!instance)
        return;
    instance->positioning.x = floor(mouse_world_pos.x);
    instance->positioning.y = floor(mouse_world_pos.y);
    make_not_overlap(node_moving.instance);
}

static inline void refresh_node_picking_results_list(void) {
    if (!node_picking.results.data)
        node_picking.results = stringvec_init();

    stringvec_truncate(&node_picking.results);
    node_picking.selected_result = 0;

    StringVector nodes = node_files_enumerate();

    char *node_name = 0;
    uint32_t node_i = 0;
    uint32_t result_count = 0;
    while ((node_name = stringvec_get(&nodes, node_i++)) &&
           result_count < MAX_NODE_RESULTS) {
        int is_match = 1;
        uint32_t search_i = 0;
        uint32_t node_name_i = 0;
        while (node_name[node_name_i] &&
               search_i < node_picking.search_buf_used) {
            if (node_name[node_name_i] < 'a' && node_name[node_name_i] > 'z')
                continue;
            if (node_name[node_name_i] != node_picking.search_buf[search_i]) {
                is_match = 0;
                break;
            }
            node_name_i++;
            search_i++;
        }
        if (is_match) {
            stringvec_append(&node_picking.results, node_name,
                             strlen(node_name));
            result_count++;
        }
    }

    stringvec_free(&nodes);
}

static inline void instantiate_node_at_mouse(NodeHandle handle) {

    NodeInstanceHandle instance = node_instantiate(handle);
    NodeInstance *node_instance = node_instance_get(instance);
    if (!node_instance)
        return;

    Vector2 mouse_world_pos = get_mouse_world_pos();
    node_instance->positioning.x = floor(mouse_world_pos.x);
    node_instance->positioning.y = floor(mouse_world_pos.y);
    make_not_overlap(instance);
}
static inline void accept_node_result(void) {

    char *node_name =
        stringvec_get(&node_picking.results, node_picking.selected_result);
    if (!node_name)
        return;

    NodeHandle existing;
    if (node_get_by_name(node_name, &existing)) {
        instantiate_node_at_mouse(existing);
        return;
    }

    Node node;
    if (node_files_load(node_name, &node))
        return;

    NodeHandle handle = node_new(node);
    instantiate_node_at_mouse(handle);
}

static inline void handle_node_picking(void) {
    if (IsKeyPressed(KEY_SPACE)) {

        if (node_picking.is_picking) {
            node_picking.search_buf_used = 0;
            accept_node_result();
        } else {
            refresh_node_picking_results_list();
        }

        node_picking.is_picking = !node_picking.is_picking;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        node_picking.search_buf_used = 0;
        node_picking.is_picking = 0;
    }

    if (!node_picking.is_picking)
        return;

    if (IsKeyPressed(KEY_UP) ||
        (IsKeyPressed(KEY_N) && IsKeyDown(KEY_LEFT_CONTROL))) {
        uint32_t result_count = stringvec_count(&node_picking.results);
        if (node_picking.selected_result < result_count - 1) {
            node_picking.selected_result++;
        }
        return;
    }

    if (IsKeyPressed(KEY_DOWN) ||
        (IsKeyPressed(KEY_P) && IsKeyDown(KEY_LEFT_CONTROL))) {
        if (node_picking.selected_result > 0)
            node_picking.selected_result--;
        return;
    }

    if (IsKeyPressed(KEY_BACKSPACE) && node_picking.search_buf_used > 0) {
        node_picking.search_buf_used--;
        refresh_node_picking_results_list();
    }

    if (node_picking.search_buf_used >= MAX_NAME)
        return;

    int key = GetKeyPressed();
    if (key < KEY_A || key > KEY_Z)
        return;

    char character = 'a' + (key - KEY_A);
    node_picking.search_buf[node_picking.search_buf_used++] = character;
    refresh_node_picking_results_list();
}

static inline void handle_inputs(void) {

    handle_zoom();
    handle_panning();
    handle_node_moving();
    handle_node_picking();
}

void gui_deinit(void) {
    uirend_deinit();
    if (node_picking.results.data)
        stringvec_free(&node_picking.results);
}

void gui_init(void) {

    uirend_init();

    fonts[FONT_ID_BASE] = LoadFont("res/fonts/MonaspaceArgon-SemiBold.otf");

    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    uint64_t size = Clay_MinMemorySize();
    Clay_Arena arena =
        Clay_CreateArenaWithCapacityAndMemory(size, malloc(size));

    Clay_Initialize(arena, (Clay_Dimensions){screen_width, screen_height},
                    (Clay_ErrorHandler){error_handler, 0});

    Clay_SetMeasureTextFunction(&uirend_measure_text, fonts);
}

void gui_main(void) {
    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    while (uirend_loop()) {

        handle_inputs();

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

                if (is_instance_title_hovered &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                    !node_moving.is_moving) {

                    node_moving.instance = i;
                    node_moving.is_moving = 1;
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

        // Node picker
        if (node_picking.is_picking)
            CLAY({
                .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                           .layoutDirection = CLAY_TOP_TO_BOTTOM},
                .backgroundColor = COLOR_BG_1,

            }) {
                // Search field
                CLAY({
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                            .padding = CLAY__CONFIG_WRAPPER(Clay_Padding,
                                                            {8, 3, 3, 3}),
                        },
                    .border = {.width = {1, 1, 1, 1, 0}, .color = COLOR_BG_2},
                }) {
                    Clay_String field_content = {
                        .isStaticallyAllocated = 1,
                        .chars = node_picking.search_buf,
                        .length = node_picking.search_buf_used,
                    };
                    CLAY_TEXT(field_content, CLAY_TEXT_CONFIG({
                                                 .fontSize = FONT_SIZE_FIXED,
                                                 .textColor = COLOR_TEXT,
                                             }));
                    CLAY_TEXT(CLAY_STRING("|"), CLAY_TEXT_CONFIG({
                                                    .fontSize = FONT_SIZE_FIXED,
                                                    .textColor = COLOR_TEXT,
                                                }));
                }

                // Results
                char *node_name;
                uint32_t node_name_i = 0;
                while ((node_name = stringvec_get(&node_picking.results,
                                                  node_name_i))) {
                    int result_selected =
                        node_name_i == node_picking.selected_result;
                    CLAY({
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(0),
                                           CLAY_SIZING_FIT(0)},
                                .padding = CLAY__CONFIG_WRAPPER(Clay_Padding,
                                                                {8, 3, 3, 3}),
                            },
                        .border = {.width = {1, 1, result_selected, 1, 0},
                                   .color = result_selected
                                                ? (Clay_Color)COLOR_BG_2
                                                : (Clay_Color)COLOR_BG_1},
                        .backgroundColor = result_selected
                                               ? (Clay_Color)COLOR_BG_1
                                               : (Clay_Color)COLOR_BG_0,
                    }) {
                        Clay_String str = {
                            .isStaticallyAllocated = 1,
                            .chars = node_name,
                            .length = strlen(node_name),
                        };
                        CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                                           .fontSize = FONT_SIZE_FIXED,
                                           .textColor = COLOR_TEXT,
                                       }));
                    }
                    node_name_i++;
                }
            }

        Clay_RenderCommandArray overlay_render_commands = Clay_EndLayout();
        uirend_render(overlay_render_commands, fonts);
        EndDrawing();
    }
}

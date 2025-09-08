#include "clay.h"
#include "gui/defines.h"
#include "gui/node.h"
#include "node_manager.h"
#include "raylib_renderer.h"
#include <raylib.h>
#include <stdio.h>

enum {
    FONT_ID_BASE,
};
static Font fonts[1] = {0};

void error_handler(Clay_ErrorData errorData) {
    printf("%s\n", errorData.errorText.chars);
}

void gui_deinit(void) {
    uirend_deinit();
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
                    (Clay_ErrorHandler){error_handler});

    Clay_SetMeasureTextFunction(&uirend_measure_text, fonts);
}

void gui_main(void) {

    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    while (uirend_loop()) {

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
            .backgroundColor = COLOR_BG_0,
        }) {
            uint32_t i = 1;
            NodeInstance *instance;
            while ((instance = node_instance_get(i))) {
                component_node(i);
                i++;
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();
        uirend_render(renderCommands, fonts);
    }
}

#include "gui/text_prompt.h"
#include "clay.h"
#include "common_node_types.h"
#include "gui/defines.h"
#include "gui/gui_common.h"
#include "log.h"
#include <ctype.h>
#include <raylib.h>
#include <string.h>

static int is_active = 0;

static struct {
    char chars[MAX_NAME + 1];
    uint32_t length;
} buf = {0};

static SubmitCallback callback = 0;
static char text[MAX_NAME + 1] = {0};

void text_prompt(const char *prompt_text, SubmitCallback submit_callback) {

    is_active = 1;
    callback = submit_callback;
    buf.length = 0;

    if (prompt_text) {
        memset(text, 0, sizeof(text));
        strncpy(text, prompt_text, MAX_NAME);
    }
}

static inline void cancel(void) {

    is_active = 0;
    buf.length = 0;
    callback = 0;
}

static inline void accept(void) {

    buf.chars[buf.length] = 0;

    if (callback)
        callback(buf.chars);

    cancel();
}

int text_prompt_handle_inputs(void) {

    if (!is_active)
        return 0;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        cancel();
        return 1;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        accept();
        return 1;
    }

    type_into_buffer(buf.chars, &buf.length, MAX_NAME);
    return 1;
}

int text_prompt_render(void) {

    if (!is_active)
        return 0;

    CLAY({
        .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                   .layoutDirection = CLAY_TOP_TO_BOTTOM},
        .backgroundColor = COLOR_BG_1,

    }) {
        // Input field
        CLAY({
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                    .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {8, 3, 3, 3}),
                },
            .border = {.width = {1, 1, 1, 1, 0}, .color = COLOR_BG_2},
        }) {

            Clay_String prompt_text = {
                .isStaticallyAllocated = 1,
                .chars = text,
                .length = strlen(text),
            };
            CLAY_TEXT(prompt_text, CLAY_TEXT_CONFIG({
                                       .fontSize = FONT_SIZE_FIXED,
                                       .textColor = COLOR_TEXT,
                                   }));

            Clay_String field_content = {
                .isStaticallyAllocated = 1,
                .chars = buf.chars,
                .length = buf.length,
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
    }

    return 1;
}

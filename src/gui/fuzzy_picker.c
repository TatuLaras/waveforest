#include "gui/fuzzy_picker.h"
#include "clay.h"
#include "common_node_types.h"
#include "gui/defines.h"
#include "gui/gui_common.h"
#include "log.h"
#include <raylib.h>
#include <string.h>

static int is_picking = 0;
static uint32_t selected_result = 0;

static struct {
    char chars[MAX_NAME];
    uint32_t length;
} search_buf = {0};

// TODO: Remember to free!
static StringVector results = {0};
static StringVector all = {0};
static OptionSelectedCallback callback = 0;

static char prompt_text[MAX_NAME + 1] = {0};

static inline void refresh_results(void) {

    if (!results.data)
        results = stringvec_init();

    stringvec_truncate(&results);
    selected_result = 0;

    char *candidate_name = 0;
    uint32_t candidate_i = 0;
    uint32_t result_count = 0;

    while ((candidate_name = stringvec_get(&all, candidate_i++)) &&
           result_count < MAX_RESULTS) {

        int is_match = 1;
        uint32_t search_buf_i = 0;
        uint32_t candidate_name_i = 0;

        while (candidate_name[candidate_name_i] &&
               search_buf_i < search_buf.length) {

            if (candidate_name[candidate_name_i] ==
                search_buf.chars[search_buf_i]) {
                candidate_name_i++;
                search_buf_i++;
                is_match = 1;
                continue;
            }

            is_match = 0;
            candidate_name_i++;
        }

        is_match = is_match && search_buf_i == search_buf.length;

        if (is_match) {
            stringvec_append(&results, candidate_name, strlen(candidate_name));
            result_count++;
        }
    }
}

void fuzzy_picker_start_picking(const char *text, StringVector options,
                                OptionSelectedCallback selected_callback) {

    all = options;
    callback = selected_callback;
    is_picking = 1;

    if (text) {
        memset(prompt_text, 0, sizeof(prompt_text));
        strncpy(prompt_text, text, MAX_NAME);
    }

    refresh_results();
}

static inline void stop_picking(void) {

    if (all.data)
        stringvec_free(&all);
    if (results.data)
        stringvec_free(&results);

    is_picking = 0;
    selected_result = 0;
    callback = 0;
    search_buf.length = 0;
}

static inline void accept_current_result(void) {

    if (!results.data)
        return;

    char *node_name = stringvec_get(&results, selected_result);
    if (!node_name || !callback)
        return;

    callback(node_name);
    stop_picking();
}

int fuzzy_picker_handle_inputs(void) {

    if (!is_picking)
        return 0;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        stop_picking();
        return 1;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        accept_current_result();
        return 1;
    }

    if (IsKeyPressed(KEY_DOWN) ||
        (IsKeyPressed(KEY_N) && IsKeyDown(KEY_LEFT_CONTROL))) {
        uint32_t result_count = stringvec_count(&results);
        if (selected_result < result_count - 1) {
            selected_result++;
        }
        return 1;
    }

    if (IsKeyPressed(KEY_UP) ||
        (IsKeyPressed(KEY_P) && IsKeyDown(KEY_LEFT_CONTROL))) {
        if (selected_result > 0)
            selected_result--;
        return 1;
    }

    if (type_into_buffer(search_buf.chars, &search_buf.length, MAX_NAME))
        refresh_results();

    return 1;
}

int fuzzy_picker_render(void) {

    if (!is_picking)
        return 0;

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
                    .padding = CLAY__CONFIG_WRAPPER(Clay_Padding, {8, 3, 3, 3}),
                },
            .border = {.width = {1, 1, 1, 1, 0}, .color = COLOR_BG_2},
        }) {
            Clay_String prompt_text_clay = {
                .isStaticallyAllocated = 1,
                .chars = prompt_text,
                .length = strlen(prompt_text),
            };
            CLAY_TEXT(prompt_text_clay, CLAY_TEXT_CONFIG({
                                            .fontSize = FONT_SIZE_FIXED,
                                            .textColor = COLOR_TEXT,
                                        }));

            Clay_String field_content = {
                .isStaticallyAllocated = 1,
                .chars = search_buf.chars,
                .length = search_buf.length,
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
        char *result_name;
        uint32_t result_i = 0;
        while ((result_name = stringvec_get(&results, result_i))) {
            int result_selected = result_i == selected_result;
            CLAY({
                .layout =
                    {
                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
                        .padding =
                            CLAY__CONFIG_WRAPPER(Clay_Padding, {8, 3, 3, 3}),
                    },
                .border = {.width = {1, 1, result_selected, 1, 0},
                           .color = result_selected ? (Clay_Color)COLOR_BG_2
                                                    : (Clay_Color)COLOR_BG_1},
                .backgroundColor = result_selected ? (Clay_Color)COLOR_BG_1
                                                   : (Clay_Color)COLOR_BG_0,

            }) {
                Clay_String str = {
                    .isStaticallyAllocated = 1,
                    .chars = result_name,
                    .length = strlen(result_name),
                };
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                                   .fontSize = FONT_SIZE_FIXED,
                                   .textColor = COLOR_TEXT,
                               }));
            }
            result_i++;
        }
    }

    return 1;
}

void fuzzy_picker_deinit(void) {
    if (all.data)
        stringvec_free(&all);
    if (results.data)
        stringvec_free(&results);
}

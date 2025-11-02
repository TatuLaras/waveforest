#include "gui/gui_common.h"

#include <ctype.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

int is_mouse_inside_box(Clay_BoundingBox box, float *out_percentage_x,
                        float *out_percentage_y) {

    float x_percent = (GetMouseX() - box.x) / box.width;

    if (out_percentage_x)
        *out_percentage_x = x_percent;
    if (out_percentage_y)
        *out_percentage_y = (GetMouseY() - box.y) / box.height;

    return box.y < GetMouseY() && GetMouseY() - box.y <= box.height &&
           x_percent <= 1.0 && x_percent >= 0.0;
}

Clay_ElementId unique_id(char *prefix, uint64_t uid) {
    char id_chars[64] = {0};
    snprintf(id_chars, 64, "%s%zu", prefix, uid);
    return Clay_GetElementId((Clay_String){.isStaticallyAllocated = 1,
                                           .chars = id_chars,
                                           .length = strlen(id_chars)});
}

#define REPEAT_COOLDOWN 0.5
#define TIME_BETWEEN_REPEATS 0.05

int type_into_buffer(char *buf_chars, uint32_t *buf_length, uint32_t buf_max) {

    static double backspace_pressed_time = 0;
    static double char_last_deleted_time = 0;

    if (IsKeyPressed(KEY_BACKSPACE) && IsKeyDown(KEY_LEFT_CONTROL)) {
        *buf_length = 0;
        return 1;
    }

    if (IsKeyPressed(KEY_BACKSPACE) && *buf_length > 0) {
        *buf_length -= 1;
        backspace_pressed_time = GetTime();
        char_last_deleted_time = GetTime();
        return 1;
    }

    if (IsKeyDown(KEY_BACKSPACE) && *buf_length > 0 &&
        GetTime() - backspace_pressed_time >= REPEAT_COOLDOWN &&
        GetTime() - char_last_deleted_time >= TIME_BETWEEN_REPEATS) {

        char_last_deleted_time = GetTime();
        *buf_length -= 1;
        return 1;
    }

    if (*buf_length >= buf_max)
        return 0;

    char character = GetCharPressed();
    if (!character)
        return 0;
    if (!isalnum(character) && character != '_' && character != '-' &&
        character != ' ')
        return 0;

    buf_chars[*buf_length] = character;
    *buf_length += 1;
    return 1;
}

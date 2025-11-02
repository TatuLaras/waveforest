#include "shortcuts.h"

#include <raylib.h>
#include <stddef.h>

#define ARRAY_LENGTH(x) ((sizeof x) / sizeof(*x))

typedef enum {
    MODE_DEFAULT,
} Mode;

typedef enum {
    MATCH_NONE,
    MATCH_FULL,
    MATCH_PARTIAL,
} ShortcutMatchType;

typedef struct {
    ShortcutAction action;
    KeyboardKey keypresses[4];
    int require_shift_down;
    int require_ctrl_down;
    int require_alt_down;
    int is_global;
    Mode mode;
} Shortcut;

typedef struct {
    KeyboardKey keypresses[4];
    uint8_t keypresses_stored;
} ShortcutBuffer;
ShortcutBuffer shortcut_buffer = {0};

static Mode mode = 0;

// Exclusive to default mode (MODE_DEFAULT) if not specified otherwise
static Shortcut shortcuts[] = {
    {
        .action = ACTION_SAVE_PATCH,
        .keypresses = {KEY_S},
        .is_global = 1,
        .require_ctrl_down = 1,
    },
    {
        .action = ACTION_SAVE_PATCH_AS,
        .keypresses = {KEY_S},
        .is_global = 1,
        .require_ctrl_down = 1,
        .require_shift_down = 1,
    },
    {
        .action = ACTION_LOAD_PATCH,
        .keypresses = {KEY_O},
        .is_global = 1,
        .require_ctrl_down = 1,
    },
    {
        .action = ACTION_DELETE_NODE_INSTANCE,
        .keypresses = {KEY_X},
        .is_global = 1,
    },
    {
        .action = ACTION_DELETE_NODE_INSTANCE,
        .keypresses = {KEY_DELETE},
        .is_global = 1,
    },
    {
        .action = ACTION_DELETE_NODE_INSTANCE,
        .keypresses = {KEY_BACKSPACE},
        .is_global = 1,
    },
    {
        .action = ACTION_PICK_NODE,
        .keypresses = {KEY_SPACE},
        .is_global = 1,
    },
};

// Resets the list of keypresses.
static void flush_keypress_buffer(void) {
    shortcut_buffer.keypresses_stored = 0;
}

// Search the list of shortcuts and return match if found, flush keypress buffer
// if not even a partial (starts with) match is found.
static Shortcut get_matching_shortcut(int shift_down, int ctrl_down,
                                      int alt_down) {
    int partial_match_exists = 0;
    for (size_t i = 0; i < ARRAY_LENGTH(shortcuts); i++) {

        if ((shift_down != shortcuts[i].require_shift_down) ||
            (ctrl_down != shortcuts[i].require_ctrl_down) ||
            (alt_down != shortcuts[i].require_alt_down) ||
            (!shortcuts[i].is_global && shortcuts[i].mode != mode))
            continue;

        ShortcutMatchType match_type = MATCH_FULL;
        size_t j = 0;
        while (j < 4 && shortcuts[i].keypresses[j]) {
            if (j >= shortcut_buffer.keypresses_stored) {
                match_type = MATCH_PARTIAL;
                break;
            }

            if (shortcuts[i].keypresses[j] != shortcut_buffer.keypresses[j]) {
                match_type = MATCH_NONE;
                break;
            }

            j++;
        }

        if (match_type == MATCH_FULL)
            return shortcuts[i];
        if (match_type == MATCH_PARTIAL)
            partial_match_exists = 1;
    }

    if (!partial_match_exists)
        flush_keypress_buffer();

    return (Shortcut){0};
}

// Adds a key to the list of keypresses.
static void append_key(KeyboardKey key) {
    if (shortcut_buffer.keypresses_stored >= 4)
        return;

    shortcut_buffer.keypresses[shortcut_buffer.keypresses_stored++] = key;
}

ShortcutAction shortcuts_get_action(KeyboardKey key, int shift_down,
                                    int ctrl_down, int alt_down) {
    if (key == KEY_NULL)
        return ACTION_NONE;

    if (key == KEY_ESCAPE) {
        flush_keypress_buffer();
        return ACTION_NONE;
    }

    append_key(key);

    Shortcut active_shortcut =
        get_matching_shortcut(shift_down, ctrl_down, alt_down);

    if (active_shortcut.action == ACTION_NONE)
        return ACTION_NONE;

    flush_keypress_buffer();

    return active_shortcut.action;
}

int shortcuts_waiting_for_keypress(void) {
    return shortcut_buffer.keypresses_stored > 0;
}

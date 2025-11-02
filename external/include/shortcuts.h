#ifndef _SHORTCUTS
#define _SHORTCUTS

// A shortcut handling module to handle multi-key sequential key combinations
// easier.

#include <raylib.h>
#include <stdint.h>

typedef enum {
    ACTION_NONE,
    ACTION_SAVE_PATCH,
    ACTION_SAVE_PATCH_AS,
    ACTION_LOAD_PATCH,
    ACTION_DELETE_NODE_INSTANCE,
    ACTION_PICK_NODE,
} ShortcutAction;

// Registers the currently pressed key and if an action has taken place it
// returns that, otherwise returns `ACTION_NONE`.
ShortcutAction shortcuts_get_action(KeyboardKey key, int shift_down,
                                    int ctrl_down, int alt_down);
// Returns true if we are waiting for another keypress (for example 'g' has been
// pressed and either an axis or a cancellation is required).
int shortcuts_waiting_for_keypress(void);

#endif

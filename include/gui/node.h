#ifndef _NODE
#define _NODE

#include "node_manager.h"
#include <raylib.h>

// Draws a node instance.
void component_node(NodeInstanceHandle instance, Vector2 screen_top_left,
                    int *out_is_title_hovered);
void component_node_connections(NodeInstanceHandle instance);
// Updates this components internal buffers for a draw cycle, call once at the
// end of every update.
void component_node_finish_update(void);

#endif

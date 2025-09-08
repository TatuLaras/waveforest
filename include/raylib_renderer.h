#include "clay.h"

#include <raylib.h>

void uirend_init(void);
void uirend_deinit(void);
void uirend_render(Clay_RenderCommandArray renderCommands, Font *fonts);
int uirend_loop(void);
Clay_Dimensions uirend_measure_text(Clay_StringSlice text,
                                    Clay_TextElementConfig *config,
                                    void *userData);

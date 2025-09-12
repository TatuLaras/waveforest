#define COLOR_BG_0 {0x21, 0x0d, 0x1f, 0xff}
#define COLOR_BG_1 {0x49, 0x1d, 0x45, 0xff}
#define COLOR_BG_2 {0x83, 0x34, 0x7c, 0xff}
#define COLOR_TEXT {0xff, 0xff, 0xff, 0xff}
#define COLOR_TEXT_MUTED {0xad, 0xd8, 0xe6, 0xff}
#define COLOR_BG_SOCKET COLOR_BG_0
#define COLOR_BG_SOCKET_HL {0xff, 0xff, 0xff, 0xff}
#define COLOR_SLIDER {0xad, 0x41, 0x72, 0xff}
#define COLOR_CONNECTION COLOR_SLIDER

extern float scale;

// Scales the value according to the scale
#define S(value) (value * scale)
#define GRID_UNIT S(100)
#define FONT_SIZE_FIXED 15
#define FONT_SIZE S(FONT_SIZE_FIXED)
#define SOCKET_DIAMETER S(12)
#define SOCKET_CORNER_RADIUS (SOCKET_DIAMETER / 2.0)

#define NODE_WIDTH 3

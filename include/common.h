#ifndef _COMMON
#define _COMMON

#define LOG_SRC "waveforest: "
#include "log.h"
#include <stdio.h>

#define max(a, b) (((a) > b) ? a : b)
#define min(a, b) (((a) < b) ? a : b)
#define clamp(val, a, b) min(max(val, a), b)

#endif

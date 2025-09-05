#ifndef _COMMON_NODE_TYPES
#define _COMMON_NODE_TYPES

#include <stdint.h>

typedef uint32_t NodeHandle;
typedef uint32_t NodeInstanceHandle;
typedef uint16_t PortHandle;

/*
 * Collection of node-related types so they are also available for node source
 * files via including this file in nodes/common_header.h.
 */

typedef enum {
    PORT_OUTPUT,
    PORT_INPUT,
    // Same as PORT_INPUT but indicates that the input can be manually
    // controlled for example via an UI nob.
    PORT_INPUT_CONTROL,
} PortType;

typedef PortHandle (*RegisterPortFunction)(NodeInstanceHandle handle,
                                           char *name, PortType type);

#endif

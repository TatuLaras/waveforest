#ifndef _MACROS
#define _MACROS

#ifdef LOG_DISABLE_ERROR
#define ERROR(...)
#else
#ifdef DEBUG
#define ERROR(message, ...)                                                    \
    fprintf(stderr,                                                            \
            "ERROR: %s:%u"                                                     \
            " %s(): " message "\n",                                            \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define ERROR(message, ...)                                                    \
    fprintf(stderr, "ERROR: " message "\n", ##__VA_ARGS__)
#endif
#endif

#ifdef LOG_DISABLE_WARNING
#define WARNING(...)
#else
#ifdef DEBUG
#define WARNING(message, ...)                                                  \
    fprintf(stderr,                                                            \
            "WARNING: %s:%u"                                                   \
            " %s(): " message "\n",                                            \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define WARNING(message, ...)                                                  \
    fprintf(stderr, "WARNING: " message "\n", ##__VA_ARGS__)
#endif
#endif

#ifdef LOG_DISABLE_INFO
#define INFO(...)
#else
#ifdef DEBUG
#define INFO(message, ...)                                                     \
    printf("INFO: %s:%u"                                                       \
           " %s(): " message "\n",                                             \
           __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#else
#define INFO(message, ...) fprintf(stderr, "INFO: " message "\n", ##__VA_ARGS__)
#endif
#endif

#ifdef LOG_DISABLE_HINT
#define HINT(...)
#else
#define HINT(message, ...) printf("HINT: " message "\n", ##__VA_ARGS__)
#endif

#endif

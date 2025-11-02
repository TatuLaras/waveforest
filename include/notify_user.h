#ifndef _NOTIFY_USER
#define _NOTIFY_USER

#include <stdio.h>

#define MAX_MESSAGE 254

#define USER_SHOULD_KNOW(format, ...)                                          \
    {                                                                          \
        char _nu_notify_msg[MAX_MESSAGE + 1] = {0};                            \
        snprintf(_nu_notify_msg, MAX_MESSAGE, format, ##__VA_ARGS__);          \
        nu_notify_user(_nu_notify_msg);                                        \
    }

void nu_notify_user(const char *message);
void nu_get_notification_begin(void);
char *nu_get_notification_iterate(void);
void nu_dismiss(const char *notification);

#endif

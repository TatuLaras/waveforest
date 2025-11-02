#include "notify_user.h"
#include <stdint.h>
#include <string.h>

#define MAX_NOTIFICATIONS 8

typedef struct {
    char message[MAX_MESSAGE + 1];
    uint8_t is_active;
} Notification;

static Notification notifications[MAX_NOTIFICATIONS] = {0};
static uint8_t notification_i = 0;
static uint8_t read_i = 0;

void nu_notify_user(const char *message) {
    notifications[notification_i] = (Notification){.is_active = 1};
    strncpy(notifications[notification_i].message, message, MAX_MESSAGE);
    notification_i = (notification_i + 1) % MAX_NOTIFICATIONS;
}

void nu_get_notification_begin(void) {
    read_i = 0;
}

char *nu_get_notification_iterate(void) {
    while (read_i < MAX_NOTIFICATIONS) {
        Notification *notif = notifications + read_i++;
        if (notif->is_active)
            return notif->message;
    }
    return 0;
}

void nu_dismiss(const char *notification) {
    for (uint8_t i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (notifications[i].message == notification)
            notifications[i].is_active = 0;
    }
}

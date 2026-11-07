/*
    Desktop (Windows / Linux / macOS) stand-ins for the iOS-only bridges.
    Android has its own implementations in android_platform.cpp.

    Copyright (C) 2026 M8NLR
    GPL-3.0-or-later
*/

#include "ios_live_activity.h"
#include "ios_notifications.h"

extern "C" bool ios_live_activity_is_available(void)
{
    return false;
}

extern "C" void ios_live_activity_start_or_update(const char *, const char *, const char *, const char *, const char *)
{
}

extern "C" void ios_live_activity_end(void)
{
}

extern "C" void ios_live_activity_end_all(void)
{
}

void ios_notifications_init(DSNotificationTapCallback, DSNotificationDeliveredCallback)
{
}

void ios_notifications_request_permission(DSNotificationPermissionCallback cb)
{
    if (cb)
        cb(false);
}

void ios_notifications_check_permission(DSNotificationPermissionCallback cb)
{
    if (cb)
        cb(false);
}

void ios_notifications_replace_scheduled_alerts(const std::vector<DSScheduledAlert> &)
{
}

void ios_notifications_set_sms_tap_callback(DSSmsNotificationTapCallback)
{
}

void ios_notifications_post_sms_alert(const QString &, const QString &, const QString &)
{
}

void ios_keyboard_observer_init(DSKeyboardHeightCallback cb)
{
    if (cb)
        cb(0);
}

void ios_set_chat_keyboard_mode(bool)
{
}

/*
    Android stand-ins for the iOS-only bridges (Live Activity, notifications,
    keyboard inset). The radio core calls these unconditionally.

    Copyright (C) 2026
    GPL-3.0-or-later
*/

#include "ios_live_activity.h"
#include "ios_notifications.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QFutureWatcher>
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>

namespace {
DSNotificationTapCallback s_netTap = nullptr;
DSNotificationDeliveredCallback s_netDelivered = nullptr;
DSSmsNotificationTapCallback s_smsTap = nullptr;

QJniObject androidContext()
{
    return QJniObject(QNativeInterface::QAndroidApplication::context());
}

int androidSdk()
{
    return QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
}

void deliverPermission(DSNotificationPermissionCallback cb, bool granted)
{
    if (cb) {
        cb(granted);
    }
}

void watchPermission(const QFuture<QtAndroidPrivate::PermissionResult> &future,
                     DSNotificationPermissionCallback cb)
{
    auto *watcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(qApp);
    QObject::connect(watcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished,
                     watcher, [watcher, cb]() {
        deliverPermission(cb, watcher->result() == QtAndroidPrivate::PermissionResult::Authorized);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

QByteArray jstringToUtf8(JNIEnv *env, jstring value)
{
    if (!value) {
        return {};
    }
    const char *chars = env->GetStringUTFChars(value, nullptr);
    QByteArray out = chars ? QByteArray(chars) : QByteArray();
    if (chars) {
        env->ReleaseStringUTFChars(value, chars);
    }
    return out;
}

void jniNetTapped(JNIEnv *env, jclass, jstring netId)
{
    if (!s_netTap) {
        return;
    }
    const QByteArray id = jstringToUtf8(env, netId);
    s_netTap(id.constData());
}

void jniSmsTapped(JNIEnv *env, jclass, jstring threadId)
{
    if (!s_smsTap) {
        return;
    }
    const QByteArray id = jstringToUtf8(env, threadId);
    s_smsTap(id.constData());
}

void jniNetDelivered(JNIEnv *env, jclass, jstring netId)
{
    if (!s_netDelivered) {
        return;
    }
    const QByteArray id = jstringToUtf8(env, netId);
    s_netDelivered(id.constData());
}

void registerNativeCallbacks()
{
    JNINativeMethod methods[] = {
        {"nativeNetTapped", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniNetTapped)},
        {"nativeSmsTapped", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniSmsTapped)},
        {"nativeNetDelivered", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniNetDelivered)},
    };
    QJniEnvironment env;
    if (!env.registerNativeMethods("org/dudetronics/droidstar/PlatformBridge",
                                   methods,
                                   int(sizeof(methods) / sizeof(methods[0])))) {
        qWarning("Failed to register PlatformBridge native methods");
    }
}

void consumePendingTaps()
{
    QJniObject context = androidContext();
    if (!context.isValid()) {
        return;
    }
    QJniObject::callStaticMethod<void>("org/dudetronics/droidstar/PlatformBridge",
                                       "drainPendingTaps",
                                       "(Landroid/content/Context;)V",
                                       context.object());
}
} // namespace

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

void ios_notifications_init(DSNotificationTapCallback tapCb, DSNotificationDeliveredCallback deliveredCb)
{
    s_netTap = tapCb;
    s_netDelivered = deliveredCb;
    registerNativeCallbacks();
    QJniObject context = androidContext();
    if (context.isValid()) {
        QJniObject::callStaticMethod<void>("org/dudetronics/droidstar/PlatformBridge",
                                           "setNativeReady",
                                           "(Landroid/content/Context;Z)V",
                                           context.object(),
                                           jboolean(JNI_TRUE));
    }
    if (auto *gui = qobject_cast<QGuiApplication *>(qApp)) {
        QObject::connect(gui, &QGuiApplication::applicationStateChanged, gui,
                         [](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive) {
                consumePendingTaps();
            }
        });
    }
}

void ios_notifications_request_permission(DSNotificationPermissionCallback cb)
{
    if (androidSdk() < 33) {
        deliverPermission(cb, true);
        return;
    }
    watchPermission(QtAndroidPrivate::requestPermission(QStringLiteral("android.permission.POST_NOTIFICATIONS")), cb);
}

void ios_notifications_check_permission(DSNotificationPermissionCallback cb)
{
    if (androidSdk() < 33) {
        deliverPermission(cb, true);
        return;
    }
    watchPermission(QtAndroidPrivate::checkPermission(QStringLiteral("android.permission.POST_NOTIFICATIONS")), cb);
}

void ios_notifications_replace_scheduled_alerts(const std::vector<DSScheduledAlert> &alerts)
{
    QJniObject context = androidContext();
    if (!context.isValid()) {
        return;
    }
    QJniEnvironment env;
    const jsize n = static_cast<jsize>(alerts.size());
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray ids = env->NewObjectArray(n, stringClass, nullptr);
    jobjectArray netIds = env->NewObjectArray(n, stringClass, nullptr);
    jobjectArray titles = env->NewObjectArray(n, stringClass, nullptr);
    jobjectArray bodies = env->NewObjectArray(n, stringClass, nullptr);
    jlongArray whenMs = env->NewLongArray(n);
    std::vector<jlong> times(static_cast<size_t>(n));
    for (jsize i = 0; i < n; ++i) {
        const DSScheduledAlert &alert = alerts[static_cast<size_t>(i)];
        env->SetObjectArrayElement(ids, i, QJniObject::fromString(alert.identifier).object<jstring>());
        env->SetObjectArrayElement(netIds, i, QJniObject::fromString(alert.netId).object<jstring>());
        env->SetObjectArrayElement(titles, i, QJniObject::fromString(alert.title).object<jstring>());
        const QString body = alert.subtitle.isEmpty()
                                 ? alert.body
                                 : alert.subtitle + QStringLiteral("\n") + alert.body;
        env->SetObjectArrayElement(bodies, i, QJniObject::fromString(body).object<jstring>());
        times[static_cast<size_t>(i)] = alert.fireTimestampSecs * 1000LL;
    }
    if (n > 0) {
        env->SetLongArrayRegion(whenMs, 0, n, times.data());
    }
    QJniObject::callStaticMethod<void>(
        "org/dudetronics/droidstar/PlatformBridge",
        "replaceAlerts",
        "(Landroid/content/Context;[Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[J)V",
        context.object(),
        ids,
        netIds,
        titles,
        bodies,
        whenMs);
}

void ios_notifications_set_sms_tap_callback(DSSmsNotificationTapCallback tapCb)
{
    s_smsTap = tapCb;
}

void ios_notifications_post_sms_alert(const QString &threadId, const QString &title, const QString &body)
{
    QJniObject context = androidContext();
    if (!context.isValid()) {
        return;
    }
    QJniObject::callStaticMethod<void>(
        "org/dudetronics/droidstar/PlatformBridge",
        "postSms",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
        context.object(),
        QJniObject::fromString(threadId).object<jstring>(),
        QJniObject::fromString(title).object<jstring>(),
        QJniObject::fromString(body).object<jstring>());
}

void ios_keyboard_observer_init(DSKeyboardHeightCallback cb)
{
    // Android resizes the Qt window for the IME, so an extra inset would double-pad the chat.
    if (cb) {
        cb(0);
    }
}

void ios_set_chat_keyboard_mode(bool)
{
}

void android_start_radio_service()
{
    QJniObject context = androidContext();
    if (!context.isValid()) {
        return;
    }
    QJniObject::callStaticMethod<void>("org/dudetronics/droidstar/RadioService",
                                       "start",
                                       "(Landroid/content/Context;)V",
                                       context.object());
}

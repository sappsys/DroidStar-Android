/*
    Copyright (C) 2026 M8NLR

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "droidstar.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QVariant>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#endif

namespace {

bool looksLikeSettings(const QStringList &keys)
{
    const QStringList sentinels = {
        QStringLiteral("CALLSIGN"),
        QStringLiteral("DMRID"),
        QStringLiteral("MODE"),
        QStringLiteral("TXTOGGLE"),
        QStringLiteral("AUDIO_NR_MODE"),
        QStringLiteral("BMPASSWORD"),
        QStringLiteral("BMPASSWORD_B64")
    };
    for (const QString &key : sentinels) {
        if (keys.contains(key))
            return true;
    }
    return false;
}

#ifdef Q_OS_ANDROID
QPointer<DroidStar> s_configOwner;
int s_activeGen = 0;

void jniConfigDone(JNIEnv *env, jclass, jboolean ok, jstring kind)
{
    QString mode;
    if (kind) {
        const char *chars = env->GetStringUTFChars(kind, nullptr);
        mode = QString::fromUtf8(chars ? chars : "");
        if (chars)
            env->ReleaseStringUTFChars(kind, chars);
    }
    DroidStar *owner = s_configOwner.data();
    if (!owner)
        return;
    const bool success = ok == JNI_TRUE;
    const int gen = s_activeGen;
    QMetaObject::invokeMethod(owner, [owner, success, mode, gen]() {
        if (s_configOwner.data() != owner)
            return;
        owner->complete_config_transfer(gen, success, mode);
    }, Qt::QueuedConnection);
}

void ensureConfigNative()
{
    static bool registered = false;
    if (registered)
        return;
    const JNINativeMethod method = {
        "nativeConfigTransferDone",
        "(ZLjava/lang/String;)V",
        reinterpret_cast<void *>(jniConfigDone)
    };
    QJniEnvironment env;
    registered = env.registerNativeMethods("org/dudetronics/droidstar/PlatformBridge", &method, 1);
}

QString transferPath(const QString &leaf)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + leaf;
}

bool beginPicker(DroidStar *self, int gen, const QString &mode, const QString &path)
{
    ensureConfigNative();
    s_configOwner = self;
    s_activeGen = gen;
    const QJniObject context(QNativeInterface::QAndroidApplication::context());
    if (!context.isValid())
        return false;
    QJniObject::callStaticMethod<void>(
        "org/dudetronics/droidstar/PlatformBridge",
        "beginConfigTransfer",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
        context.object<jobject>(),
        QJniObject::fromString(mode).object<jstring>(),
        QJniObject::fromString(path).object<jstring>());
    QJniEnvironment env;
    return !env.checkAndClearExceptions();
}
#endif

} // namespace

void DroidStar::save_config()
{
#ifndef Q_OS_ANDROID
    emit config_transfer(QStringLiteral("failed"), QStringLiteral("Save the settings file from the Android build."));
#else
    // Read the settings file in place. Export must not rewrite or replace it.
    QFile source(m_settings->fileName());
    if (!source.open(QIODevice::ReadOnly)) {
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("Could not read the settings file."));
        return;
    }
    const QByteArray bytes = source.readAll();
    source.close();
    if (bytes.isEmpty()) {
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("There are no settings to export yet."));
        return;
    }
    const QString path = transferPath(QStringLiteral("droidstar-export.conf"));
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) || out.write(bytes) != bytes.size()) {
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("Could not read the settings file."));
        return;
    }
    out.close();
    if (!start_config_picker(QStringLiteral("export"), path))
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("Could not open the file picker."));
#endif
}

void DroidStar::restore_config()
{
#ifndef Q_OS_ANDROID
    emit config_transfer(QStringLiteral("failed"), QStringLiteral("Restore the settings file from the Android build."));
#else
    const QString path = transferPath(QStringLiteral("droidstar-restore.conf"));
    QFile::remove(path);
    if (!start_config_picker(QStringLiteral("import"), path))
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("Could not open the file picker."));
#endif
}

void DroidStar::complete_config_transfer(int gen, bool ok, const QString &kind)
{
    if (gen != m_configTransferGen || !m_configPickerOpen)
        return;
    m_configPickerOpen = false;
    if (kind == QLatin1String("cancel")) {
        emit config_transfer(QStringLiteral("cancelled"), QString());
        return;
    }
    if (kind == QLatin1String("export") || m_configTransferMode != QLatin1String("import")) {
        emit config_transfer(ok ? QStringLiteral("saved") : QStringLiteral("failed"),
                             ok ? QStringLiteral("Exported a copy. The settings in the app are unchanged.")
                                : QStringLiteral("Could not write the settings file."));
        return;
    }
    if (!ok) {
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("Could not read that file."));
        return;
    }

    bool applied = false;
    {
        QSettings incoming(m_configTransferPath, QSettings::IniFormat);
        incoming.sync();
        const QStringList keys = incoming.allKeys();
        if (incoming.status() == QSettings::NoError && looksLikeSettings(keys)) {
            // Keep keys this version already has. The file only overwrites keys it contains.
            for (const QString &key : keys)
                m_settings->setValue(key, incoming.value(key));
            m_settings->sync();
            applied = true;
        }
    }
    QFile::remove(m_configTransferPath);
    if (!applied) {
        emit config_transfer(QStringLiteral("failed"), QStringLiteral("That file is not a DroidStar settings file."));
        return;
    }
    emit config_transfer(QStringLiteral("restored"), QStringLiteral("Restored. Settings already in this version were kept. Starting the app again."));
#ifdef Q_OS_ANDROID
    QTimer::singleShot(400, this, []() {
        const QJniObject context(QNativeInterface::QAndroidApplication::context());
        QJniObject::callStaticMethod<void>(
            "org/dudetronics/droidstar/PlatformBridge",
            "restartApp",
            "(Landroid/content/Context;)V",
            context.object<jobject>());
    });
#endif
}

#ifdef Q_OS_ANDROID
bool DroidStar::start_config_picker(const QString &mode, const QString &path)
{
    m_configTransferMode = mode;
    m_configTransferPath = path;
    m_configPickerOpen = true;
    const int gen = ++m_configTransferGen;
    auto *conn = new QMetaObject::Connection;
    *conn = QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
                             [this, gen, conn](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive)
            return;
        QObject::disconnect(*conn);
        delete conn;
        QTimer::singleShot(800, this, [this, gen]() {
            if (!m_configPickerOpen || m_configTransferGen != gen)
                return;
            m_configPickerOpen = false;
            emit config_transfer(QStringLiteral("cancelled"), QString());
        });
    });
    if (beginPicker(this, gen, mode, path))
        return true;
    m_configPickerOpen = false;
    QObject::disconnect(*conn);
    delete conn;
    return false;
}
#endif

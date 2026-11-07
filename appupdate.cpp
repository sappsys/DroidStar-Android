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
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslError>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

namespace {

struct AppVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int letter = 0;
    bool ok = false;
};

AppVersion parseAppVersion(QString text)
{
    text = text.trimmed();
    if (text.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        text = text.mid(1);
    static const QRegularExpression re(QStringLiteral("^(\\d+)\\.(\\d+)\\.(\\d+)([A-Za-z])?$"));
    const QRegularExpressionMatch match = re.match(text);
    AppVersion version;
    if (!match.hasMatch())
        return version;
    version.major = match.captured(1).toInt();
    version.minor = match.captured(2).toInt();
    version.patch = match.captured(3).toInt();
    if (!match.captured(4).isEmpty())
        version.letter = match.captured(4).at(0).toLower().unicode() - QChar(QLatin1Char('a')).unicode() + 1;
    version.ok = true;
    return version;
}

bool isNewerVersion(const AppVersion &candidate, const AppVersion &current)
{
    if (!candidate.ok || !current.ok)
        return false;
    if (candidate.major != current.major)
        return candidate.major > current.major;
    if (candidate.minor != current.minor)
        return candidate.minor > current.minor;
    if (candidate.patch != current.patch)
        return candidate.patch > current.patch;
    return candidate.letter > current.letter;
}

#ifdef Q_OS_ANDROID
QString androidFilesDir()
{
    const QJniObject context(QNativeInterface::QAndroidApplication::context());
    if (!context.isValid())
        return QString();
    const QJniObject dir = context.callObjectMethod("getFilesDir", "()Ljava/io/File;");
    if (!dir.isValid())
        return QString();
    return dir.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString();
}
#endif

} // namespace

void DroidStar::publish_app_update(const QString &phase, const QString &available, bool newer, int percent, const QString &detail)
{
    m_appUpdatePhase = phase;
    m_appUpdateAvailable = available;
    m_appUpdateNewer = newer;
    m_appUpdatePercent = percent;
    m_appUpdateDetail = detail;
    emit appUpdateChanged();
    emit app_update_state(phase, available, newer, percent, detail);
}

bool DroidStar::launch_app_update_installer(const QString &path)
{
#ifdef Q_OS_ANDROID
    const QJniObject context(QNativeInterface::QAndroidApplication::context());
    if (!context.isValid())
        return false;
    const jboolean allowed = QJniObject::callStaticMethod<jboolean>(
        "org/dudetronics/droidstar/PlatformBridge",
        "canInstallPackages",
        "(Landroid/content/Context;)Z",
        context.object<jobject>());
    if (!allowed) {
        QJniObject::callStaticMethod<void>(
            "org/dudetronics/droidstar/PlatformBridge",
            "openUnknownSourcesSettings",
            "(Landroid/content/Context;)V",
            context.object<jobject>());
        publish_app_update(QStringLiteral("ready"), m_appUpdateVersion, true, -1,
                              QStringLiteral("Allow this app to install updates, then press Update again."));
        return false;
    }
    const jboolean started = QJniObject::callStaticMethod<jboolean>(
        "org/dudetronics/droidstar/PlatformBridge",
        "installApk",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object<jobject>(),
        QJniObject::fromString(path).object<jstring>());
    if (!started) {
        publish_app_update(QStringLiteral("failed"), m_appUpdateVersion, true, -1,
                              QStringLiteral("The installer could not be opened."));
        return false;
    }
    publish_app_update(QStringLiteral("ready"), m_appUpdateVersion, true, -1, QString());
    return true;
#else
    Q_UNUSED(path);
    return false;
#endif
}

void DroidStar::settle_app_update_reply(QNetworkReply *reply)
{
    if (!reply || m_appUpdateReply != reply)
        return;
    if (m_appUpdatePhase == QLatin1String("downloading"))
        return;
    m_appUpdateReply = nullptr;

    const bool finished = reply->isFinished();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = (finished && reply->error() == QNetworkReply::NoError) ? reply->readAll() : QByteArray();
    if (!finished || reply->error() != QNetworkReply::NoError)
        qDebug() << "Update check ended:" << status << reply->errorString();
    reply->disconnect(this);
    if (!finished)
        reply->abort();
    reply->deleteLater();

    // A finished empty list is a real answer: nothing has been published. A transport
    // error, a timeout, or a body that is not the release list is a failed check.
    if (!finished || reply->error() != QNetworkReply::NoError || status >= 400) {
        m_appUpdateNewer = false;
        m_appUpdateUrl.clear();
        m_appUpdateVersion.clear();
        publish_app_update(QStringLiteral("failed"), QString(), false, -1, QString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    QJsonArray releases;
    if (doc.isArray())
        releases = doc.array();
    else if (doc.isObject())
        releases.append(doc.object());
    else if (!body.isEmpty()) {
        m_appUpdateNewer = false;
        m_appUpdateUrl.clear();
        m_appUpdateVersion.clear();
        publish_app_update(QStringLiteral("failed"), QString(), false, -1, QString());
        return;
    }

    const AppVersion current = parseAppVersion(get_software_build());
    AppVersion published;
    QString version;
    QString url;
    qint64 size = 0;
    bool sawRelease = false;
    bool sawApk = false;
    for (const QJsonValue &relVal : releases) {
        const QJsonObject rel = relVal.toObject();
        if (rel.value(QStringLiteral("draft")).toBool() || rel.value(QStringLiteral("prerelease")).toBool())
            continue;
        const AppVersion candidate = parseAppVersion(rel.value(QStringLiteral("tag_name")).toString());
        if (!candidate.ok)
            continue;
        sawRelease = true;
        QString assetUrl;
        qint64 assetSize = 0;
        const QJsonArray assets = rel.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue &value : assets) {
            const QJsonObject asset = value.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            const QString download = asset.value(QStringLiteral("browser_download_url")).toString();
            if (!name.endsWith(QLatin1String(".apk"), Qt::CaseInsensitive))
                continue;
            if (name.contains(QLatin1String("debug"), Qt::CaseInsensitive))
                continue;
            if (!name.contains(QLatin1String("arm64"), Qt::CaseInsensitive))
                continue;
            if (!download.startsWith(QLatin1String("https://github.com/sappsys/DroidStar-Android/")))
                continue;
            assetUrl = download;
            assetSize = asset.value(QStringLiteral("size")).toVariant().toLongLong();
            break;
        }
        if (assetUrl.isEmpty())
            continue;
        sawApk = true;
        if (published.ok && !isNewerVersion(candidate, published))
            continue;
        published = candidate;
        QString tag = rel.value(QStringLiteral("tag_name")).toString().trimmed();
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            tag = tag.mid(1);
        version = tag;
        url = assetUrl;
        size = assetSize;
    }

    m_appUpdateUrl = url;
    m_appUpdateSize = size;
    m_appUpdateVersion = version;
    m_appUpdatePath.clear();
    if (sawRelease && !sawApk) {
        m_appUpdateNewer = false;
        publish_app_update(QStringLiteral("failed"), QString(), false, -1, QString());
        return;
    }
    m_appUpdateNewer = !url.isEmpty() && isNewerVersion(published, current);
    publish_app_update(QStringLiteral("ready"),
                          m_appUpdateNewer ? m_appUpdateVersion : QString(),
                          m_appUpdateNewer,
                          -1,
                          QString());
}

void DroidStar::check_app_update()
{
#ifndef Q_OS_ANDROID
    publish_app_update(QStringLiteral("ready"), QString(), false, -1, QString());
    return;
#else
    if (m_appUpdateReply)
        return;
    if (m_appUpdatePhase == QLatin1String("downloading"))
        return;
    publish_app_update(QStringLiteral("checking"), QString(), false, -1, QString());
    if (!m_appUpdateNam)
        m_appUpdateNam = new QNetworkAccessManager(this);

    // The list returns an empty array when nothing has been published. /releases/latest
    // answers 404 in that case, and Qt's HTTP/2 client can leave that reply unfinished.
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/sappsys/DroidStar-Android/releases?per_page=10")));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DroidStar/2026"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(8000);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_appUpdateNam->get(request);
    m_appUpdateReply = reply;
    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        settle_app_update_reply(reply);
    });
    QTimer::singleShot(8000, this, [this, reply]() {
        if (m_appUpdatePhase != QLatin1String("checking"))
            return;
        if (m_appUpdateReply == reply)
            settle_app_update_reply(reply);
        if (m_appUpdatePhase == QLatin1String("checking")) {
            m_appUpdateNewer = false;
            m_appUpdateUrl.clear();
            publish_app_update(QStringLiteral("failed"), QString(), false, -1, QString());
        }
    });
#endif
}

void DroidStar::install_app_update()
{
#ifndef Q_OS_ANDROID
    return;
#else
    if (!m_appUpdateNewer || m_appUpdateUrl.isEmpty() || m_appUpdateReply)
        return;

    const QString root = androidFilesDir();
    if (root.isEmpty()) {
        publish_app_update(QStringLiteral("failed"), m_appUpdateVersion, true, -1,
                              QStringLiteral("Could not save the update."));
        return;
    }
    const QString dir = root + QStringLiteral("/updates");
    QDir().mkpath(dir);
    QString safe = m_appUpdateVersion;
    safe.remove(QRegularExpression(QStringLiteral("[^0-9A-Za-z.]")));
    m_appUpdatePath = dir + QStringLiteral("/DroidStar-") + safe + QStringLiteral("-arm64.apk");

    const QFileInfo existing(m_appUpdatePath);
    if (existing.isFile() && existing.size() > 0
        && (m_appUpdateSize <= 0 || existing.size() == m_appUpdateSize)) {
        launch_app_update_installer(m_appUpdatePath);
        return;
    }
    QFile::remove(m_appUpdatePath);

    m_appUpdateFile = new QFile(m_appUpdatePath);
    if (!m_appUpdateFile->open(QIODevice::WriteOnly)) {
        delete m_appUpdateFile;
        m_appUpdateFile = nullptr;
        publish_app_update(QStringLiteral("failed"), m_appUpdateVersion, true, -1,
                              QStringLiteral("Could not save the update."));
        return;
    }

    if (!m_appUpdateNam)
        m_appUpdateNam = new QNetworkAccessManager(this);
    QNetworkRequest request{QUrl(m_appUpdateUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DroidStar/2026"));
    request.setRawHeader("Accept", "application/octet-stream");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_appUpdatePercent = -1;
    publish_app_update(QStringLiteral("downloading"), m_appUpdateVersion, true, 0, QString());
    m_appUpdateReply = m_appUpdateNam->get(request);

    connect(m_appUpdateReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_appUpdateReply && m_appUpdateFile)
            m_appUpdateFile->write(m_appUpdateReply->readAll());
    });
    connect(m_appUpdateReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        const int percent = total > 0 ? int((received * 100) / total) : -1;
        if (percent == m_appUpdatePercent)
            return;
        m_appUpdatePercent = percent;
        publish_app_update(QStringLiteral("downloading"), m_appUpdateVersion, true, percent, QString());
    });
    connect(m_appUpdateReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_appUpdateReply;
        m_appUpdateReply = nullptr;
        const QString path = m_appUpdatePath;
        if (m_appUpdateFile) {
            if (reply)
                m_appUpdateFile->write(reply->readAll());
            m_appUpdateFile->close();
            delete m_appUpdateFile;
            m_appUpdateFile = nullptr;
        }
        if (reply)
            reply->deleteLater();
        if (!reply || reply->error() != QNetworkReply::NoError) {
            QFile::remove(path);
            publish_app_update(QStringLiteral("failed"), m_appUpdateVersion, true, -1,
                                  QStringLiteral("The download did not finish."));
            return;
        }
        const qint64 size = QFileInfo(path).size();
        if (size <= 0 || (m_appUpdateSize > 0 && size != m_appUpdateSize)) {
            QFile::remove(path);
            publish_app_update(QStringLiteral("failed"), m_appUpdateVersion, true, -1,
                                  QStringLiteral("The download was incomplete."));
            return;
        }
        launch_app_update_installer(path);
    });
#endif
}

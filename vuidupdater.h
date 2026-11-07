/*
    Copyright (C) 2025 Rohith Namboothiri

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

#ifndef VUIDUPDATER_H
#define VUIDUPDATER_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QSet>

class VUIDUpdater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fetchedFirstName READ fetchedFirstName WRITE setFetchedFirstName NOTIFY fetchedFirstNameChanged)
    Q_PROPERTY(QString fetchedCountry READ fetchedCountry WRITE setFetchedCountry NOTIFY fetchedCountryChanged)

public:
    explicit VUIDUpdater(QObject *parent = nullptr)
        : QObject(parent),
          networkAccessManager(new QNetworkAccessManager(this))
    {
        connect(networkAccessManager, &QNetworkAccessManager::finished,
                this, &VUIDUpdater::onNetworkReply);
    }

    Q_INVOKABLE void fetchFirstNameFromAPI(unsigned int data1)
    {
        if (!data1) return;
        m_lastRequestedId = data1;

        auto it = m_cache.constFind(data1);
        if (it != m_cache.constEnd()) {
            const CachedUser cached = it.value();
            setFetchedFirstName(cached.name);
            // cached.country already includes "City, State • Country" when known
            setFetchedCountryDisplay(cached.country);
            emit userLookupReady(data1, cached.callsign, cached.name, m_fetchedCountry);
            return;
        }
        // IDs radioid.net does not know (hotspot/ESSID numbers, service IDs) would otherwise be
        // re-queried on every new RX turn; remember misses for this session (same outcome: no emit).
        if (m_unknownIds.contains(data1)) {
            return;
        }

        QUrl url("https://radioid.net/api/users?id=" + QString::number(data1));
        QNetworkRequest request(url);
        request.setRawHeader("Connection", "Keep-Alive");
        request.setTransferTimeout(8000);

        networkAccessManager->get(request);
    }

    QString fetchedFirstName() const { return m_fetchedFirstName; }
    QString fetchedCountry() const { return m_fetchedCountry; }

    void setFetchedFirstName(const QString &firstName)
    {
        m_fetchedFirstName = firstName;
        emit fetchedFirstNameChanged(m_fetchedFirstName);
    }

    static QString shortenCountry(QString country)
    {
        if (country == QLatin1String("United States"))
            return QStringLiteral("US");
        if (country == QLatin1String("United Kingdom"))
            return QStringLiteral("UK");
        return country;
    }

    static QString formatCityState(const QString &city, const QString &state)
    {
        const QString c = city.trimmed();
        const QString s = state.trimmed();
        if (!c.isEmpty() && !s.isEmpty())
            return c + QStringLiteral(", ") + s;
        if (!c.isEmpty())
            return c;
        return s;
    }

    // Place string shown after the name: "City, State • Country" (parts omitted if empty).
    static QString formatPlaceDisplay(const QString &city, const QString &state, const QString &country)
    {
        const QString loc = formatCityState(city, state);
        const QString c = shortenCountry(country.trimmed());
        if (!loc.isEmpty() && !c.isEmpty())
            return loc + QStringLiteral(" • ") + c;
        if (!loc.isEmpty())
            return loc;
        return c;
    }

    void setFetchedCountry(const QString &country)
    {
        setFetchedCountryDisplay(shortenCountry(country));
    }

    void setFetchedCountryDisplay(const QString &display)
    {
        if (m_fetchedCountry == display)
            return;
        m_fetchedCountry = display;
        emit fetchedCountryChanged(m_fetchedCountry);
    }

signals:
    void fetchedFirstNameChanged(const QString &firstName);
    void fetchedCountryChanged(const QString &country);
    // Emits one complete record for callers (e.g., realtime QSO logger in App2026.qml).
    void userLookupReady(unsigned int dmrId, const QString &callsign, const QString &name, const QString &country);

private slots:
    void onNetworkReply(QNetworkReply *reply)
    {
        if (!reply) return;

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        const QByteArray response_data = reply->readAll();
        qDebug() << "onNetworkReply received:" << response_data;

        QJsonParseError parseError;
        QJsonDocument json = QJsonDocument::fromJson(response_data, &parseError);

        if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
            qDebug() << "Failed to parse JSON response:" << parseError.errorString();
            reply->deleteLater();
            return;
        }

        QJsonObject jsonObject = json.object();

        // radioid.net response (as per your logs) uses "rows": [ { "name": "...", "country": "..." } ]
        // We'll also support older/alternate formats safely.
        QJsonArray list = jsonObject.value("rows").toArray();
        if (list.isEmpty()) {
            list = jsonObject.value("results").toArray();
        }

        if (list.isEmpty()) {
            qDebug() << "rows/results array is empty or missing.";
            unsigned int missedId = QUrlQuery(reply->url()).queryItemValue(QStringLiteral("id")).toUInt();
            if (!missedId) missedId = m_lastRequestedId;
            if (missedId) {
                if (m_unknownIds.size() >= 1024) {
                    m_unknownIds.clear();
                }
                m_unknownIds.insert(missedId);
            }
            reply->deleteLater();
            return;
        }

        QJsonObject first = list.first().toObject();

        // Prefer "name", fallback to "fname" if needed
        QString name = first.value("name").toString();
        if (name.isEmpty()) {
            name = first.value("fname").toString();
        }

        const QString country = first.value("country").toString();
        const QString city = first.value("city").toString();
        // radioid.net uses "state"; some dumps use "province"
        QString state = first.value("state").toString();
        if (state.trimmed().isEmpty())
            state = first.value("province").toString();
        QString callsign = first.value("callsign").toString();
        unsigned int dmrId = first.value("radio_id").toInt();
        if (!dmrId) {
            // Some responses use "id"
            dmrId = first.value("id").toInt();
        }
        if (!dmrId) {
            // Fall back to what we requested (best-effort)
            dmrId = m_lastRequestedId;
        }

        const QString place = formatPlaceDisplay(city, state, country);
        qDebug() << "Parsed name:" << name;
        qDebug() << "Parsed place:" << place;

        setFetchedFirstName(name);
        setFetchedCountryDisplay(place);
        if (dmrId > 0) {
            if (m_cache.size() >= 1024) {
                m_cache.clear();
            }
            m_cache.insert(dmrId, CachedUser{callsign, name, place});
        }
        emit userLookupReady(dmrId, callsign, name, m_fetchedCountry);

        reply->deleteLater();
    }

private:
    struct CachedUser {
        QString callsign;
        QString name;
        QString country;
    };
    QHash<unsigned int, CachedUser> m_cache;
    QSet<unsigned int> m_unknownIds;
    QString m_fetchedFirstName;
    QString m_fetchedCountry;
    unsigned int m_lastRequestedId = 0;
    QNetworkAccessManager *networkAccessManager;
};

#endif // VUIDUPDATER_H

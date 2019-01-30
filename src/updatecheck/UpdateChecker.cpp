/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UpdateChecker.h"
#include "core/Config.h"
#include "config-keepassx.h"
#include <QJsonObject>
#include <QtNetwork>
#include <QNetworkAccessManager>

UpdateChecker* UpdateChecker::m_instance(nullptr);

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_netMgr(new QNetworkAccessManager(this))
    , m_reply(nullptr)
{
}

UpdateChecker::~UpdateChecker()
{
}

void UpdateChecker::checkForUpdates(bool manuallyRequested)
{
    m_isManuallyRequested = manuallyRequested;
    m_bytesReceived.clear();

    QString apiUrlStr = QString("https://api.github.com/repos/keepassxreboot/keepassxc/releases");

    if (!config()->get("GUI/CheckForUpdatesIncludeBetas", false).toBool()) {
        apiUrlStr += "/latest";
    }

    QUrl apiUrl = QUrl(apiUrlStr);

    QNetworkRequest request(apiUrl);
    request.setRawHeader("Accept", "application/json");

    m_reply = m_netMgr->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &UpdateChecker::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &UpdateChecker::fetchReadyRead);
}

void UpdateChecker::fetchReadyRead()
{
    m_bytesReceived += m_reply->readAll();
}

void UpdateChecker::fetchFinished()
{
    bool error = (m_reply->error() != QNetworkReply::NoError);
    bool hasNewVersion = false;
    QUrl url = m_reply->url();
    QString version = "";

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!error) {
        QJsonDocument jsonResponse = QJsonDocument::fromJson(m_bytesReceived);
        QJsonObject jsonObject = jsonResponse.object();

        if (config()->get("GUI/CheckForUpdatesIncludeBetas", false).toBool()) {
            QJsonArray jsonArray = jsonResponse.array();
            jsonObject = jsonArray.at(0).toObject();
        }

        if (!jsonObject.value("tag_name").isUndefined()) {
            version = jsonObject.value("tag_name").toString();
            hasNewVersion = compareVersions(version, QString(KEEPASSXC_VERSION));
        }
    } else {
        version = "error";
    }

    emit updateCheckFinished(hasNewVersion, version, m_isManuallyRequested);
}

bool UpdateChecker::compareVersions(const QString& remoteVersion, const QString& localVersion)
{
    if (localVersion == remoteVersion) {
        return false; // Currently using updated version
    }

    QRegularExpression verRegex("^(\\d+(\\.\\d+){0,2})(-\\w+)?$", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch lmatch = verRegex.match(localVersion);
    QRegularExpressionMatch rmatch = verRegex.match(remoteVersion);

    if (!lmatch.captured(1).isNull() && !rmatch.captured(1).isNull()) {
        if (lmatch.captured(1) == rmatch.captured(1) && !lmatch.captured(3).isNull()) {
            // Same version, but installed version has snapshot/beta suffix and should be updated to stable
            return true;
        }

        QStringList lparts = lmatch.captured(1).split(".");
        QStringList rparts = rmatch.captured(1).split(".");

        if (lparts.length() < 3)
            lparts << "0";

        if (rparts.length() < 3)
            rparts << "0";

        for (int i = 0; i < 3; i++) {
            int l = lparts[i].toInt();
            int r = rparts[i].toInt();

            if (l == r)
                continue;

            if (l > r) {
                return false; // Installed version is newer than release
            } else {
                return true; // Installed version is outdated
            }
        }

        return false; // Installed version is the same
    }

    return false; // Invalid version string
}

UpdateChecker* UpdateChecker::instance()
{
    if (!m_instance) {
        m_instance = new UpdateChecker();
    }

    return m_instance;
}

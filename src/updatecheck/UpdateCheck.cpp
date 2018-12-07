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

#include "UpdateCheck.h"
#include "config-keepassx.h"
#include <QJsonObject>

UpdateCheck* UpdateCheck::m_instance(nullptr);

UpdateCheck::UpdateCheck(QObject* parent)
    : QObject(parent)
    , m_netMgr(new QNetworkAccessManager(this))
    , m_reply(nullptr)
{
}

UpdateCheck::~UpdateCheck()
{
}

void UpdateCheck::checkForUpdates()
{
    m_bytesReceived.clear();

    QNetworkRequest request(QUrl("https://api.github.com/repos/keepassxreboot/keepassxc/releases/latest"));
    request.setRawHeader("Accept", "application/json");

    m_reply = m_netMgr->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &UpdateCheck::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &UpdateCheck::fetchReadyRead);
}

void UpdateCheck::fetchReadyRead()
{
    m_bytesReceived += m_reply->readAll();
}

void UpdateCheck::fetchFinished()
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

        if (!jsonObject.value("tag_name").isUndefined()) {
            version = jsonObject.value("tag_name").toString();
            hasNewVersion = compareVersions(version, QString(KEEPASSXC_VERSION));
        }
    } else {
        version = "error";
    }

    emit updateCheckFinished(hasNewVersion, version);
}

bool UpdateCheck::compareVersions(const QString& remoteVersion, const QString& localVersion)
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

UpdateCheck* UpdateCheck::instance()
{
    if (!m_instance) {
        m_instance = new UpdateCheck();
    }

    return m_instance;
}

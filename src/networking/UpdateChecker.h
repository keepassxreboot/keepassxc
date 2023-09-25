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

#ifndef KEEPASSXC_UPDATECHECK_H
#define KEEPASSXC_UPDATECHECK_H
#include <QObject>

class QNetworkReply;

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    void checkForUpdates(bool manuallyRequested);
    static bool compareVersions(const QString& localVersion, const QString& remoteVersion);
    static UpdateChecker* instance();

    static const QString ErrorVersion;

signals:
    void updateCheckFinished(bool hasNewVersion, QString version, bool isManuallyRequested);

private slots:
    void fetchFinished();
    void fetchReadyRead();

private:
    QNetworkReply* m_reply;
    QByteArray m_bytesReceived;
    bool m_isManuallyRequested;

    static UpdateChecker* m_instance;

    Q_DISABLE_COPY(UpdateChecker)
};

inline UpdateChecker* updateCheck()
{
    return UpdateChecker::instance();
}

#endif // KEEPASSXC_UPDATECHECK_H

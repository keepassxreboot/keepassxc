/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REMOTESETTINGSCUSTOMDATAHANDLER_H
#define KEEPASSXC_REMOTESETTINGSCUSTOMDATAHANDLER_H

#include "RemoteSettings.h"

#include "core/Database.h"

#include <QList>
#include <QVariant>

class RemoteSettingsCustomDataHandler : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSettingsCustomDataHandler(QObject* parent, QSharedPointer<Database> db);
    ~RemoteSettingsCustomDataHandler() override;

    QList<RemoteSettings*> getRemoteProgramEntries();
    void addRemoteSettingsEntry(RemoteSettings* params);
    void removeRemoteSettingsEntry(const QString& name);
    RemoteSettings* getRemoteSettingsEntry(const QString& name);

    void syncConfig();

private:
    RemoteSettingsCustomDataHandler();
    QList<RemoteSettings*> m_lastRemoteProgramEntries;

    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_REMOTESETTINGSCUSTOMDATAHANDLER_H

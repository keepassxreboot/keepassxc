/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_PASSKEYIMPORTER_H
#define KEEPASSXC_PASSKEYIMPORTER_H

#include "core/Database.h"
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QUuid>
#include <QWidget>

class Entry;

class PasskeyImporter : public QObject
{
    Q_OBJECT

public:
    explicit PasskeyImporter(QWidget* parent = nullptr);

    void importPasskey(QSharedPointer<Database>& database, Entry* entry = nullptr);
    bool showImportDialog(QSharedPointer<Database>& database,
                          Entry* entry,
                          const QString& url,
                          const QString& relyingParty,
                          const QString& username,
                          const QString& credentialId,
                          const QString& userHandle,
                          const QString& privateKey,
                          const QString& titleText = {},
                          const QString& infoText = {},
                          const QString& importButtonText = {});

private:
    void importSelectedFile(QFile& file, QSharedPointer<Database>& database, Entry* entry);
    Group* getDefaultGroup(QSharedPointer<Database>& database) const;

private:
    QPointer<QWidget> m_parent;
};

#endif // KEEPASSXC_PASSKEYIMPORTER_H

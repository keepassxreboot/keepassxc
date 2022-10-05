/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASEEDIT_H
#define KEEPASSXC_DATABASEEDIT_H

#include "DatabaseCommand.h"

class DatabaseEdit : public DatabaseCommand
{
public:
    DatabaseEdit();

    int executeWithDatabase(QSharedPointer<Database> db, QSharedPointer<QCommandLineParser> parser) override;

    static const QCommandLineOption UnsetKeyFileOption;
    static const QCommandLineOption UnsetPasswordOption;

private:
    QSharedPointer<CompositeKey> getNewDatabaseKey(QSharedPointer<Database> database,
                                                   bool updatePassword,
                                                   bool removePassword,
                                                   QString newFileKeyPath,
                                                   bool removeKeyFile);
};

#endif // KEEPASSXC_DATABASEEDIT_H

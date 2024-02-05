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
#ifndef KEEPASSXC_FIXTUREWITHDB_H
#define KEEPASSXC_FIXTUREWITHDB_H

#include "../tests/util/TemporaryFile.h"
#include "FixtureBase.h"

class Database;
class DatabaseTabWidget;
class DatabaseWidget;

// Every test starts with opening a temp database
class FixtureWithDb : public FixtureBase
{
public:
    FixtureWithDb();
    ~FixtureWithDb();

protected:
    void unlockDbByPassword(const QString& password);
    Entry* newEntry(const QString& title, const QString& user, const QString& pwd);
    Entry* getCurrentEntry();
    void checkDatabase(const QString& filePath, const QString& expectedDbName);
    void checkDatabase(const QString& filePath = {});

protected:
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;
    TemporaryFile m_dbFile;
    QString m_dbFileName;
    QString m_dbFilePath;
};

#endif // KEEPASSXC_FIXTUREWITHDB_H

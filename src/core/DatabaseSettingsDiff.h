/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASESETTINGSDIFF_H
#define KEEPASSXC_DATABASESETTINGSDIFF_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>

class Database;
class Group;

/**
 * One database-level setting of an imported XML file that differs from the database the file is imported
 * into. Importing into an existing database only adds folders and entries; these are the things that would otherwise be
 * dropped without a word. The user decides for each one whether it is applied.
 */
struct DatabaseSettingChange
{
    QString id; // stable, e.g. "name" or "customData:<key>"
    QString label; // what the user sees
    QString currentValue; // in the open database, as a person reads it
    QString fileValue; // in the imported file, as a person reads it
    QString note; // extra plain-language warning, may be empty
    QList<QUuid> uuids; // folders or icons the setting refers to (captured before the imported tree is moved)
};

namespace DatabaseSettingsDiff
{
    /**
     * The settings in the imported database that differ from the open database. Call this before the imported folders
     * are moved into the open database.
     */
    QList<DatabaseSettingChange> compare(const Database& open, const Database& imported);

    /**
     * Apply the chosen changes to the open database.
     *
     * @param importedGroups the folder of the file (by its UUID in the file) -> the folder that now exists in the open
     *                       database; needed for the recycle bin and templates folder
     * @param keepFileDates  false (an open database): the user changes the setting now, so it is dated now. True (a new
     *                       database that is a copy of the file): the dates the settings were changed on are taken from
     *                       the file; they decide which side wins when databases are merged later.
     * @return the labels of the chosen changes that could not be applied (so that the user can be told)
     */
    QStringList apply(Database& open,
                      const Database& imported,
                      const QList<DatabaseSettingChange>& chosen,
                      const QHash<QUuid, Group*>& importedGroups,
                      bool keepFileDates = false);

    /**
     * Use the folders and entries of the imported database as the content of a NEW database (its empty root folder is
     * replaced by the root folder of the file), then apply the chosen settings. Call compare() first.
     *
     * @return the labels of the chosen changes that could not be applied
     */
    QStringList adoptImportedTree(Database& newDb, Database& imported, const QList<DatabaseSettingChange>& chosen);

    /**
     * Pairs every folder of a tree with its copy (made with Group::clone), by the folder's UUID before copying.
     */
    QHash<QUuid, Group*> matchGroups(const Group* original, Group* copy);

    /**
     * Things the XML file stores about itself or about the screen that are not settings of the open database. They are
     * named to the user so that nothing is skipped silently.
     */
    QStringList notOffered();
} // namespace DatabaseSettingsDiff

#endif // KEEPASSXC_DATABASESETTINGSDIFF_H

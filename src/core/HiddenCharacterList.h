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

#ifndef KEEPASSXC_HIDDENCHARACTERLIST_H
#define KEEPASSXC_HIDDENCHARACTERLIST_H

#include <QString>
#include <QStringList>

// the list of what an XML export stored as base64 because XML cannot hold it as text (hidden control
// characters and the like), saved as a text file next to the export. The message in the window shows only the first
// few lines; this file has all of them.
namespace HiddenCharacterList
{
    // Where the list for an XML export written to exportFilePath is saved: next to it, with ".hidden-characters.txt"
    // in place of a ".xml" at the end of the name
    QString filePathFor(const QString& exportFilePath);

    // Writes the list (one line per place, as Database::extract reports them: they name the field, never a value).
    // Replaces an older file. Returns false and says why in error when it could not be written.
    bool save(const QString& filePath, const QStringList& places, QString* error);
} // namespace HiddenCharacterList

#endif // KEEPASSXC_HIDDENCHARACTERLIST_H

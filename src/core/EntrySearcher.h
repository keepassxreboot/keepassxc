/*
 *  Copyright (C) 2014 Florian Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_ENTRYSEARCHER_H
#define KEEPASSX_ENTRYSEARCHER_H

#include <QString>

class Group;
class Entry;

class EntrySearcher
{
public:
    QList<Entry*> search(const QString& searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity);

private:
    QList<Entry*> searchEntries(const QString& searchTerm, const QList<Entry*>& entries, Qt::CaseSensitivity caseSensitivity);
    bool matchEntry(const QString& searchTerm, Entry* entry, Qt::CaseSensitivity caseSensitivity);
    bool wordMatch(const QString& word, Entry* entry, Qt::CaseSensitivity caseSensitivity);
};

#endif // KEEPASSX_ENTRYSEARCHER_H

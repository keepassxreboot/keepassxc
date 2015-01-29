/*
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "EntrySearcher.h"

#include "core/Group.h"

QList<Entry*> EntrySearcher::search(const QString &searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    if (!group->resolveSearchingEnabled()) {
        return QList<Entry*>();
    }

    return searchEntries(searchTerm, group, caseSensitivity);
}

QList<Entry*> EntrySearcher::searchEntries(const QString& searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    QList<Entry*> searchResult;

    Q_FOREACH (Entry* entry, group->entries()) {
        searchResult.append(matchEntry(searchTerm, entry, caseSensitivity));
    }
    Q_FOREACH (Group* childGroup, group->children()) {
        if (childGroup->searchingEnabled() != Group::Disable) {
            searchResult.append(searchEntries(searchTerm, childGroup, caseSensitivity));
        }
    }

    return searchResult;
}

QList<Entry*> EntrySearcher::matchEntry(const QString& searchTerm, Entry* entry, Qt::CaseSensitivity caseSensitivity)
{
    QStringList wordList = searchTerm.split(QRegExp("\\s"), QString::SkipEmptyParts);
    Q_FOREACH (const QString& word, wordList) {
        if (!wordMatch(word, entry, caseSensitivity)) {
            return QList<Entry*>();
        }
    }

    return QList<Entry*>() << entry;
}

bool EntrySearcher::wordMatch(const QString& word, Entry* entry, Qt::CaseSensitivity caseSensitivity)
{
    return entry->title().contains(word, caseSensitivity) ||
            entry->username().contains(word, caseSensitivity) ||
            entry->url().contains(word, caseSensitivity) ||
            entry->notes().contains(word, caseSensitivity);
}

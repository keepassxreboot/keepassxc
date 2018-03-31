/*
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "EntrySearcher.h"

#include "core/Group.h"

QList<Entry*> EntrySearcher::search(const QString& searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    if (!group->resolveSearchingEnabled()) {
        return QList<Entry*>();
    }

    return searchEntries(searchTerm, group, caseSensitivity);
}

QList<Entry*>
EntrySearcher::searchEntries(const QString& searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    QList<Entry*> searchResult;

    const QList<Entry*> entryList = group->entries();
    for (Entry* entry : entryList) {
        searchResult.append(matchEntry(searchTerm, entry, caseSensitivity));
    }

    const QList<Group*> children = group->children();
    for (Group* childGroup : children) {
        if (childGroup->searchingEnabled() != Group::Disable) {
            if (matchGroup(searchTerm, childGroup, caseSensitivity)) {
                searchResult.append(childGroup->entriesRecursive());
            } else {
                searchResult.append(searchEntries(searchTerm, childGroup, caseSensitivity));
            }
        }
    }

    return searchResult;
}

QList<Entry*> EntrySearcher::matchEntry(const QString& searchTerm, Entry* entry, Qt::CaseSensitivity caseSensitivity)
{
    const QStringList wordList = searchTerm.split(QRegExp("\\s"), QString::SkipEmptyParts);
    for (const QString& word : wordList) {
        if (!wordMatch(word, entry, caseSensitivity)) {
            return QList<Entry*>();
        }
    }

    return QList<Entry*>() << entry;
}

bool EntrySearcher::wordMatch(const QString& word, Entry* entry, Qt::CaseSensitivity caseSensitivity)
{
    return entry->resolvePlaceholder(entry->title()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->username()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->url()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->notes()).contains(word, caseSensitivity);
}

bool EntrySearcher::matchGroup(const QString& searchTerm, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    const QStringList wordList = searchTerm.split(QRegExp("\\s"), QString::SkipEmptyParts);
    for (const QString& word : wordList) {
        if (!wordMatch(word, group, caseSensitivity)) {
            return false;
        }
    }

    return true;
}

bool EntrySearcher::wordMatch(const QString& word, const Group* group, Qt::CaseSensitivity caseSensitivity)
{
    return group->name().contains(word, caseSensitivity) || group->notes().contains(word, caseSensitivity);
}

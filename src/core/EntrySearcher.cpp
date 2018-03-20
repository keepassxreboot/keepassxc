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
    QList<Entry*> results;

    if (group->resolveSearchingEnabled()) {
        results.append(searchEntries(searchTerm, group->entries(), caseSensitivity));
    }

    for (Group* childGroup : group->children()) {
        if (childGroup->resolveSearchingEnabled()) {
            results.append(searchEntries(searchTerm, childGroup->entries(), caseSensitivity));
        }
    }

    return results;
}

QList<Entry*> EntrySearcher::searchEntries(const QString& searchTerm, const QList<Entry*>& entries,
                                           Qt::CaseSensitivity caseSensitivity)
{
    QList<Entry*> results;
    for (Entry* entry : entries) {
       if (matchEntry(searchTerm, entry, caseSensitivity)) {
           results.append(entry);
       }
    }
    return results;
}

bool EntrySearcher::matchEntry(const QString& searchTerm, Entry* entry,
                                        Qt::CaseSensitivity caseSensitivity)
{
    const QStringList wordList = searchTerm.split(QRegExp("\\s"), QString::SkipEmptyParts);
    for (const QString& word : wordList) {
        if (!wordMatch(word, entry, caseSensitivity)) {
            return false;
        }
    }

    return true;
}

bool EntrySearcher::wordMatch(const QString& word, Entry* entry, Qt::CaseSensitivity caseSensitivity)
{
    return entry->resolvePlaceholder(entry->title()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->username()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->url()).contains(word, caseSensitivity)
           || entry->resolvePlaceholder(entry->notes()).contains(word, caseSensitivity);
}

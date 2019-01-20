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
#include "core/Tools.h"

EntrySearcher::EntrySearcher(bool caseSensitive)
    : m_caseSensitive(caseSensitive)
    , m_termParser(R"re(([-!*+]+)?(?:(\w*):)?(?:(?=")"((?:[^"\\]|\\.)*)"|([^ ]*))( |$))re")
// Group 1 = modifiers, Group 2 = field, Group 3 = quoted string, Group 4 = unquoted string
{
}

QList<Entry*> EntrySearcher::search(const QString& searchString, const Group* baseGroup, bool forceSearch)
{
    Q_ASSERT(baseGroup);

    QList<Entry*> results;
    for (const auto group : baseGroup->groupsRecursive(true)) {
        if (forceSearch || group->resolveSearchingEnabled()) {
            results.append(searchEntries(searchString, group->entries()));
        }
    }

    return results;
}

QList<Entry*> EntrySearcher::searchEntries(const QString& searchString, const QList<Entry*>& entries)
{
    QList<Entry*> results;
    for (Entry* entry : entries) {
        if (searchEntryImpl(searchString, entry)) {
            results.append(entry);
        }
    }
    return results;
}

void EntrySearcher::setCaseSensitive(bool state)
{
    m_caseSensitive = state;
}

bool EntrySearcher::isCaseSensitive()
{
    return m_caseSensitive;
}

bool EntrySearcher::searchEntryImpl(const QString& searchString, Entry* entry)
{
    // Pre-load in case they are needed
    auto attributes = QStringList(entry->attributes()->keys());
    auto attachments = QStringList(entry->attachments()->keys());

    bool found;
    auto searchTerms = parseSearchTerms(searchString);

    for (const auto& term : searchTerms) {
        switch (term->field) {
        case Field::Title:
            found = term->regex.match(entry->resolvePlaceholder(entry->title())).hasMatch();
            break;
        case Field::Username:
            found = term->regex.match(entry->resolvePlaceholder(entry->username())).hasMatch();
            break;
        case Field::Password:
            found = term->regex.match(entry->resolvePlaceholder(entry->password())).hasMatch();
            break;
        case Field::Url:
            found = term->regex.match(entry->resolvePlaceholder(entry->url())).hasMatch();
            break;
        case Field::Notes:
            found = term->regex.match(entry->notes()).hasMatch();
            break;
        case Field::Attribute:
            found = !attributes.filter(term->regex).empty();
            break;
        case Field::Attachment:
            found = !attachments.filter(term->regex).empty();
            break;
        default:
            // Terms without a specific field try to match title, username, url, and notes
            found = term->regex.match(entry->resolvePlaceholder(entry->title())).hasMatch()
                    || term->regex.match(entry->resolvePlaceholder(entry->username())).hasMatch()
                    || term->regex.match(entry->resolvePlaceholder(entry->url())).hasMatch()
                    || term->regex.match(entry->notes()).hasMatch();
        }

        // Short circuit if we failed to match or we matched and are excluding this term
        if ((!found && !term->exclude) || (found && term->exclude)) {
            return false;
        }
    }

    return true;
}

QList<QSharedPointer<EntrySearcher::SearchTerm>> EntrySearcher::parseSearchTerms(const QString& searchString)
{
    auto terms = QList<QSharedPointer<SearchTerm>>();

    auto results = m_termParser.globalMatch(searchString);
    while (results.hasNext()) {
        auto result = results.next();
        auto term = QSharedPointer<SearchTerm>::create();

        // Quoted string group
        term->word = result.captured(3);

        // If empty, use the unquoted string group
        if (term->word.isEmpty()) {
            term->word = result.captured(4);
        }

        // If still empty, ignore this match
        if (term->word.isEmpty()) {
            continue;
        }

        auto mods = result.captured(1);

        // Convert term to regex
        term->regex = Tools::convertToRegex(term->word, !mods.contains("*"), mods.contains("+"), m_caseSensitive);

        // Exclude modifier
        term->exclude = mods.contains("-") || mods.contains("!");

        // Determine the field to search
        QString field = result.captured(2);
        if (!field.isEmpty()) {
            auto cs = Qt::CaseInsensitive;
            if (field.compare("title", cs) == 0) {
                term->field = Field::Title;
            } else if (field.startsWith("user", cs)) {
                term->field = Field::Username;
            } else if (field.startsWith("pass", cs)) {
                term->field = Field::Password;
            } else if (field.compare("url", cs) == 0) {
                term->field = Field::Url;
            } else if (field.compare("notes", cs) == 0) {
                term->field = Field::Notes;
            } else if (field.startsWith("attr", cs)) {
                term->field = Field::Attribute;
            } else if (field.startsWith("attach", cs)) {
                term->field = Field::Attachment;
            } else {
                term->field = Field::Undefined;
            }
        }

        terms.append(term);
    }

    return terms;
}

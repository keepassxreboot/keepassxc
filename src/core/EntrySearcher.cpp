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

/**
 * Search group, and its children, directly by provided search terms
 * @param searchTerms  search terms
 * @param baseGroup group to start search from, cannot be null
 * @param forceSearch ignore group search settings
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::search(const QList<SearchTerm>& searchTerms, const Group* baseGroup, bool forceSearch)
{
    Q_ASSERT(baseGroup);
    m_searchTerms = searchTerms;
    return repeat(baseGroup, forceSearch);
}

/**
 * Search group, and its children, by parsing the provided search
 * string for search terms.
 *
 * @param searchString search terms
 * @param baseGroup group to start search from, cannot be null
 * @param forceSearch ignore group search settings
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::search(const QString& searchString, const Group* baseGroup, bool forceSearch)
{
    Q_ASSERT(baseGroup);

    parseSearchTerms(searchString);
    return repeat(baseGroup, forceSearch);
}

/**
 * Repeat the last search starting from the given group
 *
 * @param baseGroup group to start search from, cannot be null
 * @param forceSearch ignore group search settings
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::repeat(const Group* baseGroup, bool forceSearch)
{
    Q_ASSERT(baseGroup);

    QList<Entry*> results;
    for (const auto group : baseGroup->groupsRecursive(true)) {
        if (forceSearch || group->resolveSearchingEnabled()) {
            for (auto* entry : group->entries()) {
                if (searchEntryImpl(entry)) {
                    results.append(entry);
                }
            }
        }
    }
    return results;
}

/**
 * Search provided entries by the provided search terms
 *
 * @param searchTerms search terms
 * @param entries list of entries to include in the search
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::searchEntries(const QList<SearchTerm>& searchTerms, const QList<Entry*>& entries)
{
    m_searchTerms = searchTerms;
    return repeatEntries(entries);
}

/**
 * Search provided entries by parsing the search string
 * for search terms.
 *
 * @param searchString search terms
 * @param entries list of entries to include in the search
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::searchEntries(const QString& searchString, const QList<Entry*>& entries)
{
    parseSearchTerms(searchString);
    return repeatEntries(entries);
}

/**
 * Repeat the last search on the given entries
 *
 * @param entries list of entries to include in the search
 * @return list of entries that match the search terms
 */
QList<Entry*> EntrySearcher::repeatEntries(const QList<Entry*>& entries)
{
    QList<Entry*> results;
    for (auto* entry : entries) {
        if (searchEntryImpl(entry)) {
            results.append(entry);
        }
    }
    return results;
}

/**
 * Set the next search to be case sensitive or not
 *
 * @param state
 */
void EntrySearcher::setCaseSensitive(bool state)
{
    m_caseSensitive = state;
}

bool EntrySearcher::isCaseSensitive()
{
    return m_caseSensitive;
}

bool EntrySearcher::searchEntryImpl(Entry* entry)
{
    // Pre-load in case they are needed
    auto attributes_keys = entry->attributes()->customKeys();
    auto attributes = QStringList(attributes_keys + entry->attributes()->values(attributes_keys));
    auto attachments = QStringList(entry->attachments()->keys());

    bool found;
    for (const auto& term : m_searchTerms) {
        switch (term.field) {
        case Field::Title:
            found = term.regex.match(entry->resolvePlaceholder(entry->title())).hasMatch();
            break;
        case Field::Username:
            found = term.regex.match(entry->resolvePlaceholder(entry->username())).hasMatch();
            break;
        case Field::Password:
            found = term.regex.match(entry->resolvePlaceholder(entry->password())).hasMatch();
            break;
        case Field::Url:
            found = term.regex.match(entry->resolvePlaceholder(entry->url())).hasMatch();
            break;
        case Field::Notes:
            found = term.regex.match(entry->notes()).hasMatch();
            break;
        case Field::AttributeKV:
            found = !attributes.filter(term.regex).empty();
            break;
        case Field::Attachment:
            found = !attachments.filter(term.regex).empty();
            break;
        case Field::AttributeValue:
            // skip protected attributes
            if (entry->attributes()->isProtected(term.word)) {
                continue;
            }
            found = entry->attributes()->contains(term.word)
                    && term.regex.match(entry->attributes()->value(term.word)).hasMatch();
            break;
        default:
            // Terms without a specific field try to match title, username, url, and notes
            found = term.regex.match(entry->resolvePlaceholder(entry->title())).hasMatch()
                    || term.regex.match(entry->resolvePlaceholder(entry->username())).hasMatch()
                    || term.regex.match(entry->resolvePlaceholder(entry->url())).hasMatch()
                    || term.regex.match(entry->notes()).hasMatch();
        }

        // Short circuit if we failed to match or we matched and are excluding this term
        if ((!found && !term.exclude) || (found && term.exclude)) {
            return false;
        }
    }

    return true;
}

void EntrySearcher::parseSearchTerms(const QString& searchString)
{
    static const QList<QPair<QString, Field>> fieldnames{
        {QStringLiteral("attachment"), Field::Attachment},
        {QStringLiteral("attribute"), Field::AttributeKV},
        {QStringLiteral("notes"), Field::Notes},
        {QStringLiteral("pw"), Field::Password},
        {QStringLiteral("password"), Field::Password},
        {QStringLiteral("title"), Field::Title},
        {QStringLiteral("u"), Field::Username}, // u: stands for username rather than url
        {QStringLiteral("url"), Field::Url},
        {QStringLiteral("username"), Field::Username}};

    m_searchTerms.clear();
    auto results = m_termParser.globalMatch(searchString);
    while (results.hasNext()) {
        auto result = results.next();
        SearchTerm term{};

        // Quoted string group
        term.word = result.captured(3);

        // If empty, use the unquoted string group
        if (term.word.isEmpty()) {
            term.word = result.captured(4);
        }

        // If still empty, ignore this match
        if (term.word.isEmpty()) {
            continue;
        }

        auto mods = result.captured(1);

        // Convert term to regex
        term.regex = Tools::convertToRegex(term.word, !mods.contains("*"), mods.contains("+"), m_caseSensitive);

        // Exclude modifier
        term.exclude = mods.contains("-") || mods.contains("!");

        // Determine the field to search
        term.field = Field::Undefined;

        QString field = result.captured(2);
        if (!field.isEmpty()) {
            if (field.startsWith("_", Qt::CaseInsensitive)) {
                term.field = Field::AttributeValue;
                // searching a custom attribute
                // in this case term.word is the attribute key (removing the leading "_")
                // and term.regex is used to match attribute value
                term.word = field.mid(1);
            } else {
                for (const auto& pair : fieldnames) {
                    if (pair.first.startsWith(field, Qt::CaseInsensitive)) {
                        term.field = pair.second;
                        break;
                    }
                }
            }
        }

        m_searchTerms.append(term);
    }
}

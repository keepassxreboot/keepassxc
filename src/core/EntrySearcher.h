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

#include <QRegularExpression>

class Group;
class Entry;

class EntrySearcher
{
public:
    enum class Field
    {
        Undefined,
        Title,
        Username,
        Password,
        Url,
        Notes,
        AttributeKV,
        Attachment,
        AttributeValue,
        Group,
        Tag,
        Is,
        Has,
        Uuid
    };

    struct SearchTerm
    {
        Field field;
        QString word;
        QRegularExpression regex;
        bool exclude;
    };

    explicit EntrySearcher(bool caseSensitive = false, bool includeProtected = false);

    QList<Entry*> search(const QList<SearchTerm>& searchTerms, const Group* baseGroup, bool forceSearch = false);
    QList<Entry*> search(const QString& searchString, const Group* baseGroup, bool forceSearch = false);
    QList<Entry*> repeat(const Group* baseGroup, bool forceSearch = false);

    QList<Entry*> searchEntries(const QList<SearchTerm>& searchTerms, const QList<Entry*>& entries);
    QList<Entry*> searchEntries(const QString& searchString, const QList<Entry*>& entries);
    QList<Entry*> repeatEntries(const QList<Entry*>& entries);

    void setCaseSensitive(bool state) { m_caseSensitive = state; } // Set the next search to be case sensitive or not
    bool isCaseSensitive() const { return m_caseSensitive; }

    void setIncludeProtected(bool state) { m_includeProtected = state; } // Set the next search to include protected fields
    bool isIncludeProtected() const { return m_includeProtected; }

private:
    bool searchEntryImpl(const Entry* entry);
    void parseSearchTerms(const QString& searchString);

    bool m_caseSensitive;
    bool m_includeProtected;
    QList<SearchTerm> m_searchTerms;

    friend class TestEntrySearcher;
};

#endif // KEEPASSX_ENTRYSEARCHER_H

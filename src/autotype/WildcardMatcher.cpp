/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "WildcardMatcher.h"

#include <QStringList>

const QChar WildcardMatcher::Wildcard = '*';
const Qt::CaseSensitivity WildcardMatcher::Sensitivity = Qt::CaseInsensitive;

WildcardMatcher::WildcardMatcher(const QString& text)
    : m_text(text)
{
}

bool WildcardMatcher::match(const QString& pattern)
{
    m_pattern = pattern;

    if (patternContainsWildcard()) {
        return matchWithWildcards();
    } else {
        return patternEqualsText();
    }
}

bool WildcardMatcher::patternContainsWildcard()
{
    return m_pattern.contains(Wildcard);
}

bool WildcardMatcher::patternEqualsText()
{
    return m_text.compare(m_pattern, Sensitivity) == 0;
}

bool WildcardMatcher::matchWithWildcards()
{
    QStringList parts = m_pattern.split(Wildcard, QString::KeepEmptyParts);
    Q_ASSERT(parts.size() >= 2);

    if (startOrEndDoesNotMatch(parts)) {
        return false;
    }

    return partsMatch(parts);
}

bool WildcardMatcher::startOrEndDoesNotMatch(const QStringList& parts)
{
    return !m_text.startsWith(parts.first(), Sensitivity) || !m_text.endsWith(parts.last(), Sensitivity);
}

bool WildcardMatcher::partsMatch(const QStringList& parts)
{
    int index = 0;
    for (const QString& part : parts) {
        int matchIndex = getMatchIndex(part, index);
        if (noMatchFound(matchIndex)) {
            return false;
        }
        index = calculateNewIndex(matchIndex, part.length());
    }

    return true;
}

int WildcardMatcher::getMatchIndex(const QString& part, int startIndex)
{
    return m_text.indexOf(part, startIndex, Sensitivity);
}

bool WildcardMatcher::noMatchFound(int index)
{
    return index == -1;
}

int WildcardMatcher::calculateNewIndex(int matchIndex, int partLength)
{
    return matchIndex + partLength;
}

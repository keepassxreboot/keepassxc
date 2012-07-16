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

#include "AutoTypeAssociations.h"

bool AutoTypeAssociations::Association::operator==(const AutoTypeAssociations::Association& other) const
{
    return window == other.window && sequence == other.sequence;
}

bool AutoTypeAssociations::Association::operator!=(const AutoTypeAssociations::Association& other) const
{
    return window != other.window || sequence != other.sequence;
}


AutoTypeAssociations::AutoTypeAssociations(QObject* parent)
    : QObject(parent)
{
}

void AutoTypeAssociations::copyDataFrom(const AutoTypeAssociations* other)
{
    if (m_associations == other->m_associations) {
        return;
    }

    Q_EMIT aboutToReset();
    m_associations = other->m_associations;
    Q_EMIT reset();
    Q_EMIT modified();
}

void AutoTypeAssociations::add(const AutoTypeAssociations::Association& association)
{
    int index = m_associations.size();
    Q_EMIT aboutToAdd(index);
    m_associations.append(association);
    Q_EMIT added(index);
    Q_EMIT modified();
}

void AutoTypeAssociations::remove(int index)
{
    Q_ASSERT(index >= 0 && index < m_associations.size());

    Q_EMIT aboutToRemove(index);
    m_associations.removeAt(index);
    Q_EMIT removed(index);
    Q_EMIT modified();
}

void AutoTypeAssociations::removeEmpty()
{
    QMutableListIterator<AutoTypeAssociations::Association> i(m_associations);
    while (i.hasNext()) {
        const Association& assoc = i.next();
        if (assoc.window.isEmpty() && assoc.sequence.isEmpty()) {
            i.remove();
        }
    }
}

void AutoTypeAssociations::update(int index, const AutoTypeAssociations::Association& association)
{
    Q_ASSERT(index >= 0 && index < m_associations.size());

    if (m_associations.at(index) != association) {
        m_associations[index] = association;
        Q_EMIT dataChanged(index);
        Q_EMIT modified();
    }
}

AutoTypeAssociations::Association AutoTypeAssociations::get(int index) const
{
    Q_ASSERT(index >= 0 && index < m_associations.size());

    return m_associations.at(index);
}

QList<AutoTypeAssociations::Association> AutoTypeAssociations::getAll() const
{
    return m_associations;
}

int AutoTypeAssociations::size() const
{
    return m_associations.size();
}

void AutoTypeAssociations::clear()
{
    m_associations.clear();
}

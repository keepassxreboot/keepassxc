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
    : ModifiableObject(parent)
{
}

void AutoTypeAssociations::copyDataFrom(const AutoTypeAssociations* other)
{
    if (m_associations == other->m_associations) {
        return;
    }

    emit aboutToReset();
    m_associations = other->m_associations;
    emit reset();
    emitModified();
}

void AutoTypeAssociations::add(const AutoTypeAssociations::Association& association)
{
    int index = m_associations.size();
    emit aboutToAdd(index);
    m_associations.append(association);
    emit added(index);
    emitModified();
}

void AutoTypeAssociations::remove(int index)
{
    Q_ASSERT(index >= 0 && index < m_associations.size());

    emit aboutToRemove(index);
    m_associations.removeAt(index);
    emit removed(index);
    emitModified();
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
        emit dataChanged(index);
        emitModified();
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

int AutoTypeAssociations::associationsSize() const
{
    int size = 0;
    for (const Association& association : m_associations) {
        size += association.sequence.toUtf8().size() + association.window.toUtf8().size();
    }
    return size;
}

void AutoTypeAssociations::clear()
{
    m_associations.clear();
}

bool AutoTypeAssociations::operator==(const AutoTypeAssociations& other) const
{
    if (m_associations.count() != other.m_associations.count()) {
        return false;
    }
    for (int i = 0; i < m_associations.count(); ++i) {
        if (m_associations[i] != other.m_associations[i]) {
            return false;
        }
    }
    return true;
}

bool AutoTypeAssociations::operator!=(const AutoTypeAssociations& other) const
{
    return !(*this == other);
}

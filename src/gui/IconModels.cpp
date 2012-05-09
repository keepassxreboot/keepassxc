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

#include "IconModels.h"

#include "core/DatabaseIcons.h"

DefaultIconModel::DefaultIconModel(QObject* parent) :
    QAbstractListModel(parent)
{
}

int DefaultIconModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return databaseIcons()->iconCount();
}

QVariant DefaultIconModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Q_ASSERT(index.row() < databaseIcons()->iconCount());

    if (role == Qt::DecorationRole) {
        return databaseIcons()->iconPixmap(index.row());
    }

    return QVariant();
}

CustomIconModel::CustomIconModel(QObject* parent) :
    QAbstractListModel(parent)
{
}

void CustomIconModel::setIcons(QHash<Uuid, QImage> icons)
{
    beginResetModel();

    m_icons = icons;
    m_uuids.clear();
    QHash<Uuid, QImage>::const_iterator iter;
    int i = 0;
    for (iter = m_icons.constBegin(); iter != m_icons.constEnd(); ++iter) {
        m_uuids.insert(i, iter.key());
        i++;
    }
    Q_ASSERT(m_uuids.count() == m_icons.size());

    endResetModel();
}

int CustomIconModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_icons.size();
}

QVariant CustomIconModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DecorationRole) {
        return m_icons.value(uuidFromIndex(index));
    }

    return QVariant();
}

Uuid CustomIconModel::uuidFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid());

    return m_uuids.value(index.row());
}

QModelIndex CustomIconModel::indexFromUuid(const Uuid& uuid) const
{
    QHash<int, Uuid>::const_iterator iter;
    for (iter = m_uuids.constBegin(); iter != m_uuids.constEnd(); ++iter) {
        if (iter.value() == uuid) {
            return createIndex(iter.key(), 0);
        }
    }
    return QModelIndex();
}

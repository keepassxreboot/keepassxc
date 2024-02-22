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

#include "AutoTypeAssociationsModel.h"

#include "core/Entry.h"

AutoTypeAssociationsModel::AutoTypeAssociationsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_autoTypeAssociations(nullptr)
    , m_entry(nullptr)
{
}

void AutoTypeAssociationsModel::setAutoTypeAssociations(AutoTypeAssociations* autoTypeAssociations)
{
    beginResetModel();

    if (m_autoTypeAssociations) {
        m_autoTypeAssociations->disconnect(this);
    }

    m_autoTypeAssociations = autoTypeAssociations;

    if (m_autoTypeAssociations) {
        connect(m_autoTypeAssociations, SIGNAL(dataChanged(int)), SLOT(associationChange(int)));
        connect(m_autoTypeAssociations, SIGNAL(aboutToAdd(int)), SLOT(associationAboutToAdd(int)));
        connect(m_autoTypeAssociations, SIGNAL(added(int)), SLOT(associationAdd()));
        connect(m_autoTypeAssociations, SIGNAL(aboutToRemove(int)), SLOT(associationAboutToRemove(int)));
        connect(m_autoTypeAssociations, SIGNAL(removed(int)), SLOT(associationRemove()));
        connect(m_autoTypeAssociations, SIGNAL(aboutToReset()), SLOT(aboutToReset()));
        connect(m_autoTypeAssociations, SIGNAL(reset()), SLOT(reset()));
    }

    endResetModel();
}

void AutoTypeAssociationsModel::setEntry(Entry* entry)
{
    m_entry = entry;
}

int AutoTypeAssociationsModel::rowCount(const QModelIndex& parent) const
{
    if (!m_autoTypeAssociations || parent.isValid()) {
        return 0;
    } else {
        return m_autoTypeAssociations->size();
    }
}

int AutoTypeAssociationsModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 2;
}

QVariant AutoTypeAssociationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        if (section == 0) {
            return tr("Window");
        } else {
            return tr("Sequence");
        }
    } else {
        return {};
    }
}

QVariant AutoTypeAssociationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            QString window = m_autoTypeAssociations->get(index.row()).window;
            if (window.isEmpty()) {
                return tr("(empty)");
            }
            if (m_entry) {
                window = m_entry->maskPasswordPlaceholders(window);
                window = m_entry->resolveMultiplePlaceholders(window);
            }
            return window;
        } else {
            QString sequence = m_autoTypeAssociations->get(index.row()).sequence;
            if (sequence.isEmpty()) {
                sequence = tr("Default sequence");
            }
            return sequence;
        }
    } else {
        return {};
    }
}

void AutoTypeAssociationsModel::associationChange(int i)
{
    emit dataChanged(index(i, 0), index(i, columnCount() - 1));
}

void AutoTypeAssociationsModel::associationAboutToAdd(int i)
{
    beginInsertRows(QModelIndex(), i, i);
}

void AutoTypeAssociationsModel::associationAdd()
{
    endInsertRows();
}

void AutoTypeAssociationsModel::associationAboutToRemove(int i)
{
    beginRemoveRows(QModelIndex(), i, i);
}

void AutoTypeAssociationsModel::associationRemove()
{
    endRemoveRows();
}

void AutoTypeAssociationsModel::aboutToReset()
{
    beginResetModel();
}

void AutoTypeAssociationsModel::reset()
{
    endResetModel();
}

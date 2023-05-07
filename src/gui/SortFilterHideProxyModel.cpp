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

#include "SortFilterHideProxyModel.h"

SortFilterHideProxyModel::SortFilterHideProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    m_collator.setNumericMode(true);
}

Qt::DropActions SortFilterHideProxyModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

void SortFilterHideProxyModel::hideColumn(int column, bool hide)
{
    m_hiddenColumns.resize(column + 1);
    m_hiddenColumns[column] = hide;

    invalidateFilter();
}

bool SortFilterHideProxyModel::filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent)

    return sourceColumn >= m_hiddenColumns.size() || !m_hiddenColumns.at(sourceColumn);
}

bool SortFilterHideProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    auto leftData = sourceModel()->data(left, sortRole());
    auto rightData = sourceModel()->data(right, sortRole());
    if (leftData.type() == QVariant::String) {
        return m_collator.compare(leftData.toString(), rightData.toString()) < 0;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

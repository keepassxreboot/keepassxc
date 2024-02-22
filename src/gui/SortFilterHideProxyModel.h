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

#ifndef KEEPASSX_SORTFILTERHIDEPROXYMODEL_H
#define KEEPASSX_SORTFILTERHIDEPROXYMODEL_H

#include <QBitArray>
#include <QCollator>
#include <QSortFilterProxyModel>

class SortFilterHideProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SortFilterHideProxyModel(QObject* parent = nullptr);
    Qt::DropActions supportedDragActions() const override;
    void hideColumn(int column, bool hide);

protected:
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    QBitArray m_hiddenColumns;
    QCollator m_collator;
};

#endif // KEEPASSX_SORTFILTERHIDEPROXYMODEL_H

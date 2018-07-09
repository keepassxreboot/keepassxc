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

#ifndef KEEPASSX_ICONMODELS_H
#define KEEPASSX_ICONMODELS_H

#include <QAbstractListModel>
#include <QPixmap>

class DefaultIconModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DefaultIconModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
};

class CustomIconModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CustomIconModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void setIcons(const QHash<QUuid, QPixmap>& icons, const QList<QUuid>& iconsOrder);
    QUuid uuidFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromUuid(const QUuid& uuid) const;

private:
    QHash<QUuid, QPixmap> m_icons;
    QList<QUuid> m_iconsOrder;
};

#endif // KEEPASSX_ICONMODELS_H

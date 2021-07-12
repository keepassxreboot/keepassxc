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

#ifndef KEEPASSX_AUTOTYPEASSOCIATIONSMODEL_H
#define KEEPASSX_AUTOTYPEASSOCIATIONSMODEL_H

#include <QAbstractListModel>
#include <QPointer>

class AutoTypeAssociations;

class Entry;

class AutoTypeAssociationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AutoTypeAssociationsModel(QObject* parent = nullptr);
    void setAutoTypeAssociations(AutoTypeAssociations* autoTypeAssociations);
    void setEntry(Entry* entry);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public slots:
    void associationChange(int i);
    void associationAboutToAdd(int i);
    void associationAdd();
    void associationAboutToRemove(int i);
    void associationRemove();
    void aboutToReset();
    void reset();

private:
    AutoTypeAssociations* m_autoTypeAssociations;
    QPointer<const Entry> m_entry;
};

#endif // KEEPASSX_AUTOTYPEASSOCIATIONSMODEL_H

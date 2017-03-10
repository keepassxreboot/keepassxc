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

#ifndef KEEPASSX_ENTRYATTRIBUTESMODEL_H
#define KEEPASSX_ENTRYATTRIBUTESMODEL_H

#include <QAbstractListModel>

class EntryAttributes;

class EntryAttributesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit EntryAttributesModel(QObject* parent = nullptr);
    void setEntryAttributes(EntryAttributes* entryAttributes);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex indexByKey(const QString& key) const;
    QString keyByIndex(const QModelIndex& index) const;

private slots:
    void attributeChange(const QString& key);
    void attributeAboutToAdd(const QString& key);
    void attributeAdd();
    void attributeAboutToRemove(const QString& key);
    void attributeRemove();
    void attributeAboutToRename(const QString& oldKey, const QString& newKey);
    void attributeRename(const QString& oldKey, const QString& newKey);
    void aboutToReset();
    void reset();

private:
    void updateAttributes();

    EntryAttributes* m_entryAttributes;
    QList<QString> m_attributes;
    bool m_nextRenameDataChange;
};

#endif // KEEPASSX_ENTRYATTRIBUTESMODEL_H

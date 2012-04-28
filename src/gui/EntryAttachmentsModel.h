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

#ifndef KEEPASSX_ENTRYATTACHMENTSMODEL_H
#define KEEPASSX_ENTRYATTACHMENTSMODEL_H

#include <QtCore/QAbstractTableModel>

class EntryAttachments;

class EntryAttachmentsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EntryAttachmentsModel(QObject* parent = 0);
    void setEntryAttachments(EntryAttachments* entry);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

private Q_SLOTS:
    void attachmentChange(const QString& key);
    void attachmentAboutToAdd(const QString& key);
    void attachmentAdd();
    void attachmentAboutToRemove(const QString& key);
    void attachmentRemove();
    void aboutToReset();
    void reset();

private:
    EntryAttachments* m_entryAttachments;
};

#endif // KEEPASSX_ENTRYATTACHMENTSMODEL_H

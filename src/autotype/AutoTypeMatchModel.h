/*
 *  Copyright (C) 2015 David Wu <lightvector@gmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AUTOTYPEMATCHMODEL_H
#define KEEPASSX_AUTOTYPEMATCHMODEL_H

#include <QAbstractTableModel>

#include "autotype/AutoTypeMatch.h"

class Entry;
class Group;

class AutoTypeMatchModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum ModelColumn
    {
        ParentGroup = 0,
        Title = 1,
        Username = 2,
        Sequence = 3
    };

    explicit AutoTypeMatchModel(QObject* parent = nullptr);
    AutoTypeMatch matchFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromMatch(const AutoTypeMatch& match) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setMatchList(const QList<AutoTypeMatch>& matches);

private slots:
    void entryAboutToRemove(Entry* entry);
    void entryRemoved();
    void entryDataChanged(Entry* entry);

private:
    void severConnections();
    void makeConnections(const Group* group);

    QList<AutoTypeMatch> m_matches;
    QList<const Group*> m_allGroups;
};

#endif // KEEPASSX_AUTOTYPEMATCHMODEL_H

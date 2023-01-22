/*
 * Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_TAGVIEW_H
#define KEEPASSXC_TAGVIEW_H

#include <QListView>
#include <QPointer>
#include <QSharedPointer>

class Database;
class QAbstractListModel;
class TagModel;

class TagView : public QListView
{
    Q_OBJECT

public:
    explicit TagView(QWidget* parent = nullptr);
    void setDatabase(QSharedPointer<Database> db);

signals:

private slots:
    void contextMenuRequested(const QPoint& pos);

private:
    QSharedPointer<Database> m_db;
    QPointer<TagModel> m_model;
};

#endif // KEEPASSX_ENTRYVIEW_H

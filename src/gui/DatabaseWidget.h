/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DATABASEWIDGET_H
#define KEEPASSX_DATABASEWIDGET_H

#include <QtGui/QStackedWidget>

class ChangeMasterKeyWidget;
class Database;
class EditEntryWidget;
class EditGroupWidget;
class Entry;
class EntryView;
class Group;
class GroupView;

class DatabaseWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit DatabaseWidget(Database* db, QWidget* parent = 0);
    GroupView* groupView();
    EntryView* entryView();
    bool dbHasKey();

Q_SIGNALS:
    void closeRequest();
    void currentIndexChanged(int index);

public Q_SLOTS:
    void createEntry();
    void createGroup();
    void switchToEntryEdit();
    void switchToGroupEdit();
    void switchToMasterKeyChange();
    void setCurrentIndex(int index);

private Q_SLOTS:
    void switchToView(bool accepted);
    void switchToEntryEdit(Entry* entry);
    void switchToEntryEdit(Entry* entry, bool create);
    void switchToGroupEdit(Group* entry, bool create);
    void updateMasterKey(bool accepted);

private:
    Database* m_db;
    QWidget* m_mainWidget;
    EditEntryWidget* m_editEntryWidget;
    EditGroupWidget* m_editGroupWidget;
    ChangeMasterKeyWidget* m_changeMasterKeyWidget;
    GroupView* m_groupView;
    EntryView* m_entryView;
    Group* m_newGroup;
    Entry* m_newEntry;
    Group* m_newParent;
};

#endif // KEEPASSX_DATABASEWIDGET_H

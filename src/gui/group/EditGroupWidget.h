/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_EDITGROUPWIDGET_H
#define KEEPASSX_EDITGROUPWIDGET_H

#include <QScopedPointer>

#include "gui/EditWidget.h"

class Group;
class Database;
class EditWidgetIcons;
class EditWidgetAutoType;
class EditWidgetProperties;

namespace Ui {
    class EditGroupWidgetMain;
    class EditWidget;
}

class EditGroupWidget : public EditWidget
{
    Q_OBJECT

public:
    explicit EditGroupWidget(QWidget* parent = nullptr);
    ~EditGroupWidget();

    void loadGroup(Group* group, bool create, Database* database);
    void clear();

signals:
    void editFinished(bool accepted);
    void messageEditEntry(QString, MessageWidget::MessageType);
    void messageEditEntryDismiss();

private slots:
    void apply();
    void save();
    void cancel();

private:
    const QScopedPointer<Ui::EditGroupWidgetMain> m_mainUi;
    QWidget* const m_editGroupWidgetMain;
    EditWidgetIcons* const m_editGroupWidgetIcons;
    EditWidgetAutoType* const m_editWidgetAutoType;
    EditWidgetProperties* const m_editWidgetProperties;
    Group* m_group;
    Database* m_database;

    Q_DISABLE_COPY(EditGroupWidget)
};

#endif // KEEPASSX_EDITGROUPWIDGET_H

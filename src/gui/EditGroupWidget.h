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

#include <QtCore/QScopedPointer>
#include <QtGui/QWidget>

#include "core/Group.h"

namespace Ui {
    class EditGroupWidget;
}

class EditGroupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditGroupWidget(QWidget* parent = 0);
    ~EditGroupWidget();

    void loadGroup(Group* group, bool create);

Q_SIGNALS:
    void editFinished(bool accepted);

private Q_SLOTS:
    void save();
    void cancel();

private:
    QScopedPointer<Ui::EditGroupWidget> m_ui;
    Group* m_group;

    Q_DISABLE_COPY(EditGroupWidget)
};

#endif // KEEPASSX_EDITGROUPWIDGET_H

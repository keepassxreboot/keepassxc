/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_EDITGROUPPAGEKEESHARE_H
#define KEEPASSXC_EDITGROUPPAGEKEESHARE_H

#include "gui/group/EditGroupWidget.h"

class Group;
class Database;

class EditGroupPageKeeShare : public IEditGroupPage
{
public:
    EditGroupPageKeeShare(EditGroupWidget* widget);
    QString name() override;
    QIcon icon() override;
    QWidget* createWidget() override;
    void set(QWidget* widget, Group* temporaryGroup, QSharedPointer<Database> database) override;
    void assign(QWidget* widget) override;
};

#endif // KEEPASSXC_EDITGROUPPAGEKEESHARE_H

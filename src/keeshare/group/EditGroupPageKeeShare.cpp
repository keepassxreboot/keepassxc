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

#include "EditGroupPageKeeShare.h"

#include "core/FilePath.h"
#include "keeshare/group/EditGroupWidgetKeeShare.h"

#include <QApplication>

EditGroupPageKeeShare::EditGroupPageKeeShare(EditGroupWidget* widget)
{
    Q_UNUSED(widget);
}

QString EditGroupPageKeeShare::name()
{
    return QApplication::tr("KeeShare");
}

QIcon EditGroupPageKeeShare::icon()
{
    return FilePath::instance()->icon("apps", "preferences-system-network-sharing");
}

QWidget* EditGroupPageKeeShare::createWidget()
{
    return new EditGroupWidgetKeeShare();
}

void EditGroupPageKeeShare::set(QWidget* widget, Group* temporaryGroup)
{
    EditGroupWidgetKeeShare* settingsWidget = reinterpret_cast<EditGroupWidgetKeeShare*>(widget);
    settingsWidget->setGroup(temporaryGroup);
}

void EditGroupPageKeeShare::assign(QWidget* widget)
{
    Q_UNUSED(widget);
    // everything is saved directly
}

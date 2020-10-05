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

#include "SettingsPageKeeShare.h"

#include "core/Database.h"
#include "core/Group.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/Icons.h"
#include "gui/MessageWidget.h"
#include "keeshare/KeeShare.h"
#include "keeshare/SettingsWidgetKeeShare.h"
#include <QApplication>
#include <QObject>

SettingsPageKeeShare::SettingsPageKeeShare(DatabaseTabWidget* tabWidget)
    : m_tabWidget(tabWidget)
{
}

QString SettingsPageKeeShare::name()
{
    return QApplication::tr("KeeShare");
}

QIcon SettingsPageKeeShare::icon()
{
    return icons()->icon("preferences-system-network-sharing");
}

QWidget* SettingsPageKeeShare::createWidget()
{
    auto* widget = new SettingsWidgetKeeShare();
    QObject::connect(widget,
                     SIGNAL(settingsMessage(QString, MessageWidget::MessageType)),
                     m_tabWidget,
                     SIGNAL(messageGlobal(QString, MessageWidget::MessageType)));
    return widget;
}

void SettingsPageKeeShare::loadSettings(QWidget* widget)
{
    Q_UNUSED(widget);
    SettingsWidgetKeeShare* settingsWidget = reinterpret_cast<SettingsWidgetKeeShare*>(widget);
    settingsWidget->loadSettings();
}

void SettingsPageKeeShare::saveSettings(QWidget* widget)
{
    Q_UNUSED(widget);
    SettingsWidgetKeeShare* settingsWidget = reinterpret_cast<SettingsWidgetKeeShare*>(widget);
    return settingsWidget->saveSettings();
}

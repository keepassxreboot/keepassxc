/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
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

#include "AgentSettingsPage.h"
#include "AgentSettingsWidget.h"
#include "gui/Icons.h"

QString AgentSettingsPage::name()
{
    return QObject::tr("SSH Agent");
}

QIcon AgentSettingsPage::icon()
{
    return icons()->icon("utilities-terminal");
}

QWidget* AgentSettingsPage::createWidget()
{
    return new AgentSettingsWidget();
}

void AgentSettingsPage::loadSettings(QWidget* widget)
{
    AgentSettingsWidget* agentWidget = reinterpret_cast<AgentSettingsWidget*>(widget);
    agentWidget->loadSettings();
}

void AgentSettingsPage::saveSettings(QWidget* widget)
{
    AgentSettingsWidget* agentWidget = reinterpret_cast<AgentSettingsWidget*>(widget);
    agentWidget->saveSettings();
}

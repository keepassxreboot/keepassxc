/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsPageHibp.h"

#include "ReportsWidgetHibp.h"
#include "gui/Icons.h"

ReportsPageHibp::ReportsPageHibp()
    : m_hibpWidget(new ReportsWidgetHibp())
{
}

QString ReportsPageHibp::name()
{
    return QObject::tr("HIBP");
}

QIcon ReportsPageHibp::icon()
{
    return icons()->icon("hibp");
}

QWidget* ReportsPageHibp::createWidget()
{
    return m_hibpWidget;
}

void ReportsPageHibp::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetHibp*>(widget);
    settingsWidget->loadSettings(db);
}

void ReportsPageHibp::saveSettings(QWidget* widget)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetHibp*>(widget);
    settingsWidget->saveSettings();
}

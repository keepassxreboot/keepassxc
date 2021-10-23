/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsPageBrowserStatistics.h"

#include "ReportsWidgetBrowserStatistics.h"
#include "gui/Icons.h"

ReportsPageBrowserStatistics::ReportsPageBrowserStatistics()
    : m_browserWidget(new ReportsWidgetBrowserStatistics())
{
}

QString ReportsPageBrowserStatistics::name()
{
    return QObject::tr("Browser Statistics");
}

QIcon ReportsPageBrowserStatistics::icon()
{
    return icons()->icon("internet-web-browser");
}

QWidget* ReportsPageBrowserStatistics::createWidget()
{
    return m_browserWidget;
}

void ReportsPageBrowserStatistics::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetBrowserStatistics*>(widget);
    settingsWidget->loadSettings(db);
}

void ReportsPageBrowserStatistics::saveSettings(QWidget* widget)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetBrowserStatistics*>(widget);
    settingsWidget->saveSettings();
}

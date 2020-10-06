/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsPageHealthcheck.h"

#include "ReportsWidgetHealthcheck.h"
#include "gui/Icons.h"

#include <QApplication>

ReportsPageHealthcheck::ReportsPageHealthcheck()
    : m_healthWidget(new ReportsWidgetHealthcheck())
{
}

QString ReportsPageHealthcheck::name()
{
    return QApplication::tr("Health Check");
}

QIcon ReportsPageHealthcheck::icon()
{
    return icons()->icon("health");
}

QWidget* ReportsPageHealthcheck::createWidget()
{
    return m_healthWidget;
}

void ReportsPageHealthcheck::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetHealthcheck*>(widget);
    settingsWidget->loadSettings(db);
}

void ReportsPageHealthcheck::saveSettings(QWidget* widget)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetHealthcheck*>(widget);
    settingsWidget->saveSettings();
}

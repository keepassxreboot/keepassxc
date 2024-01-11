/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsPagePasskeys.h"
#include "ReportsWidgetPasskeys.h"
#include "gui/Icons.h"

ReportsPagePasskeys::ReportsPagePasskeys()
    : m_passkeysWidget(new ReportsWidgetPasskeys())
{
}

QString ReportsPagePasskeys::name()
{
    return QObject::tr("Passkeys");
}

QIcon ReportsPagePasskeys::icon()
{
    return icons()->icon("passkey");
}

QWidget* ReportsPagePasskeys::createWidget()
{
    return m_passkeysWidget;
}

void ReportsPagePasskeys::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetPasskeys*>(widget);
    settingsWidget->loadSettings(db);
}

void ReportsPagePasskeys::saveSettings(QWidget* widget)
{
    const auto settingsWidget = reinterpret_cast<ReportsWidgetPasskeys*>(widget);
    settingsWidget->saveSettings();
}

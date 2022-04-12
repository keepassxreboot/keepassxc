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

#include "NewDatabaseWizardPageMetaData.h"
#include "gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.h"

NewDatabaseWizardPageMetaData::NewDatabaseWizardPageMetaData(QWidget* parent)
    : NewDatabaseWizardPage(parent)
{
    setPageWidget(new DatabaseSettingWidgetMetaData());

    setTitle(tr("General Database Information"));
    setSubTitle(tr("Please fill in the display name and an optional description for your new database:"));
}

NewDatabaseWizardPageMetaData::~NewDatabaseWizardPageMetaData() = default;
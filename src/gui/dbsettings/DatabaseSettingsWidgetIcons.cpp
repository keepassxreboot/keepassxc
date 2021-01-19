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

#include "DatabaseSettingsWidgetIcons.h"
#include "ui_DatabaseSettingsWidgetIcons.h"

#include <QProgressDialog>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/MessageBox.h"

DatabaseSettingsWidgetIcons::DatabaseSettingsWidgetIcons(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetIcons())
{
    m_ui->setupUi(this);

    connect(m_ui->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));
    connect(m_ui->purgeButton, SIGNAL(clicked()), SLOT(purgeUnusedCustomIcons()));
}

DatabaseSettingsWidgetIcons::~DatabaseSettingsWidgetIcons()
{
}

void DatabaseSettingsWidgetIcons::initialize()
{
}

void DatabaseSettingsWidgetIcons::removeCustomIcon()
{
}

void DatabaseSettingsWidgetIcons::purgeUnusedCustomIcons()
{
}

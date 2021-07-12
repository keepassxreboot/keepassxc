
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

#include "DatabaseSettingsWidgetKeeShare.h"
#include "ui_DatabaseSettingsWidgetKeeShare.h"

#include "core/Group.h"
#include "keeshare/KeeShare.h"
#include "keeshare/KeeShareSettings.h"

#include <QStandardItemModel>

DatabaseSettingsWidgetKeeShare::DatabaseSettingsWidgetKeeShare(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetKeeShare())
{
    m_ui->setupUi(this);
}

DatabaseSettingsWidgetKeeShare::~DatabaseSettingsWidgetKeeShare()
{
}

void DatabaseSettingsWidgetKeeShare::loadSettings(QSharedPointer<Database> db)
{
    m_db = db;

    m_referencesModel.reset(new QStandardItemModel());

    m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Breadcrumb") << tr("Type") << tr("Path")
                                                               << tr("Last Signer") << tr("Certificates"));
    const QList<Group*> groups = db->rootGroup()->groupsRecursive(true);
    for (const Group* group : groups) {
        if (!KeeShare::isShared(group)) {
            continue;
        }
        const KeeShareSettings::Reference reference = KeeShare::referenceOf(group);

        QStringList hierarchy = group->hierarchy();
        hierarchy.removeFirst();
        QList<QStandardItem*> row = QList<QStandardItem*>();
        row << new QStandardItem(hierarchy.join(tr(" > ", "Breadcrumb separator")));
        row << new QStandardItem(KeeShare::referenceTypeLabel(reference));
        row << new QStandardItem(reference.path);
        m_referencesModel->appendRow(row);
    }

    m_ui->sharedGroupsView->setModel(m_referencesModel.data());
}

void DatabaseSettingsWidgetKeeShare::saveSettings()
{
    // nothing to do - the tab is passive
}

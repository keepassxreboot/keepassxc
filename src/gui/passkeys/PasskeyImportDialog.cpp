/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PasskeyImportDialog.h"
#include "ui_PasskeyImportDialog.h"

#include "browser/BrowserService.h"
#include "core/Metadata.h"
#include "gui/MainWindow.h"
#include <QCloseEvent>
#include <QFileInfo>

PasskeyImportDialog::PasskeyImportDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::PasskeyImportDialog())
    , m_useDefaultGroup(true)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);
    m_ui->useDefaultGroupCheckbox->setChecked(true);
    m_ui->selectGroupComboBox->setEnabled(false);

    connect(m_ui->importButton, SIGNAL(clicked()), SLOT(accept()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), SLOT(reject()));
    connect(m_ui->selectDatabaseButton, SIGNAL(clicked()), SLOT(selectDatabase()));
    connect(m_ui->selectGroupComboBox, SIGNAL(currentIndexChanged(int)), SLOT(changeGroup(int)));
    connect(m_ui->useDefaultGroupCheckbox, SIGNAL(stateChanged(int)), SLOT(useDefaultGroupChanged()));
}

PasskeyImportDialog::~PasskeyImportDialog()
{
}

void PasskeyImportDialog::setInfo(const QString& url, const QString& username, const QSharedPointer<Database>& database)
{
    m_ui->urlLabel->setText(tr("URL: %1").arg(url));
    m_ui->usernameLabel->setText(tr("Username: %1").arg(username));
    m_ui->selectDatabaseLabel->setText(tr("Database: %1").arg(getDatabaseName(database)));
    m_ui->selectGroupLabel->setText(tr("Group:"));

    addGroups(database);

    auto openDatabaseCount = 0;
    for (auto dbWidget : getMainWindow()->getOpenDatabases()) {
        if (dbWidget && !dbWidget->isLocked()) {
            openDatabaseCount++;
        }
    }
    m_ui->selectDatabaseButton->setEnabled(openDatabaseCount > 1);
}

QSharedPointer<Database> PasskeyImportDialog::getSelectedDatabase()
{
    return m_selectedDatabase;
}

QUuid PasskeyImportDialog::getSelectedGroupUuid()
{
    return m_selectedGroupUuid;
}

bool PasskeyImportDialog::useDefaultGroup()
{
    return m_useDefaultGroup;
}

QString PasskeyImportDialog::getDatabaseName(const QSharedPointer<Database>& database) const
{
    return QFileInfo(database->filePath()).fileName();
}

void PasskeyImportDialog::addGroups(const QSharedPointer<Database>& database)
{
    m_ui->selectGroupComboBox->clear();
    for (const auto& group : database->rootGroup()->groupsRecursive(true)) {
        if (!group || group->isRecycled() || group == database->metadata()->recycleBin()) {
            continue;
        }

        m_ui->selectGroupComboBox->addItem(group->fullPath(), group->uuid());
    }
}

void PasskeyImportDialog::selectDatabase()
{
    auto selectedDatabase = browserService()->selectedDatabase();
    if (!selectedDatabase) {
        return;
    }

    m_selectedDatabase = selectedDatabase;
    m_ui->selectDatabaseLabel->setText(QString("Database: %1").arg(getDatabaseName(m_selectedDatabase)));

    addGroups(m_selectedDatabase);
}

void PasskeyImportDialog::changeGroup(int index)
{
    m_selectedGroupUuid = m_ui->selectGroupComboBox->itemData(index).value<QUuid>();
}

void PasskeyImportDialog::useDefaultGroupChanged()
{
    m_ui->selectGroupComboBox->setEnabled(!m_ui->useDefaultGroupCheckbox->isChecked());
    m_useDefaultGroup = m_ui->useDefaultGroupCheckbox->isChecked();
}

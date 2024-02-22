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
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);

    connect(this, SIGNAL(updateGroups()), this, SLOT(addGroups()));
    connect(this, SIGNAL(updateEntries()), this, SLOT(addEntries()));
    connect(m_ui->importButton, SIGNAL(clicked()), SLOT(accept()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), SLOT(reject()));
    connect(m_ui->selectDatabaseCombobBox, SIGNAL(currentIndexChanged(int)), SLOT(changeDatabase(int)));
    connect(m_ui->selectEntryComboBox, SIGNAL(currentIndexChanged(int)), SLOT(changeEntry(int)));
    connect(m_ui->selectGroupComboBox, SIGNAL(currentIndexChanged(int)), SLOT(changeGroup(int)));
}

PasskeyImportDialog::~PasskeyImportDialog()
{
}

void PasskeyImportDialog::setInfo(const QString& relyingParty,
                                  const QString& username,
                                  const QSharedPointer<Database>& database,
                                  bool isEntry)
{
    m_ui->relyingPartyLabel->setText(tr("Relying Party: %1").arg(relyingParty));
    m_ui->usernameLabel->setText(tr("Username: %1").arg(username));

    if (isEntry) {
        m_ui->verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        m_ui->infoLabel->setText(tr("Import the following Passkey to this entry:"));
        m_ui->groupBox->setVisible(false);
    }

    m_selectedDatabase = database;
    addDatabases();
    addGroups();

    auto openDatabaseCount = 0;
    for (auto dbWidget : getMainWindow()->getOpenDatabases()) {
        if (dbWidget && !dbWidget->isLocked()) {
            openDatabaseCount++;
        }
    }
    m_ui->selectDatabaseCombobBox->setEnabled(openDatabaseCount > 1);
}

QSharedPointer<Database> PasskeyImportDialog::getSelectedDatabase() const
{
    return m_selectedDatabase;
}

QUuid PasskeyImportDialog::getSelectedEntryUuid() const
{
    return m_selectedEntryUuid;
}

QUuid PasskeyImportDialog::getSelectedGroupUuid() const
{
    return m_selectedGroupUuid;
}

bool PasskeyImportDialog::useDefaultGroup() const
{
    return m_selectedGroupUuid.isNull();
}

bool PasskeyImportDialog::createNewEntry() const
{
    return m_selectedEntryUuid.isNull();
}

void PasskeyImportDialog::addDatabases()
{
    auto currentDatabaseIndex = 0;
    const auto openDatabases = browserService()->getOpenDatabases();
    const auto currentDatabase = browserService()->getDatabase();

    m_ui->selectDatabaseCombobBox->clear();
    for (const auto& db : openDatabases) {
        m_ui->selectDatabaseCombobBox->addItem(db->metadata()->name(), db->rootGroup()->uuid());
        if (db->rootGroup()->uuid() == currentDatabase->rootGroup()->uuid()) {
            currentDatabaseIndex = m_ui->selectDatabaseCombobBox->count() - 1;
        }
    }

    m_ui->selectDatabaseCombobBox->setCurrentIndex(currentDatabaseIndex);
}

void PasskeyImportDialog::addEntries()
{
    if (!m_selectedDatabase || !m_selectedDatabase->rootGroup()) {
        return;
    }

    m_ui->selectEntryComboBox->clear();
    m_ui->selectEntryComboBox->addItem(tr("Create new entry"), {});

    const auto group = m_selectedDatabase->rootGroup()->findGroupByUuid(m_selectedGroupUuid);
    if (!group) {
        return;
    }

    // Collect all entries in the group and resolve the title
    QList<QPair<QString, QUuid>> entries;
    for (const auto entry : group->entries()) {
        if (!entry || entry->isRecycled()) {
            continue;
        }
        entries.append({entry->resolveMultiplePlaceholders(entry->title()), entry->uuid()});
    }

    // Sort entries by title
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    // Add sorted entries to the combobox
    for (const auto& pair : entries) {
        m_ui->selectEntryComboBox->addItem(pair.first, pair.second);
    }
}

void PasskeyImportDialog::addGroups()
{
    if (!m_selectedDatabase) {
        return;
    }

    m_ui->selectGroupComboBox->clear();
    m_ui->selectGroupComboBox->addItem(tr("Default Passkeys group (Imported Passkeys)"), {});

    for (const auto& group : m_selectedDatabase->rootGroup()->groupsRecursive(true)) {
        if (!group || group->isRecycled() || group == m_selectedDatabase->metadata()->recycleBin()) {
            continue;
        }

        m_ui->selectGroupComboBox->addItem(group->fullPath(), group->uuid());
    }
}

void PasskeyImportDialog::changeDatabase(int index)
{
    m_selectedDatabaseUuid = m_ui->selectDatabaseCombobBox->itemData(index).value<QUuid>();
    m_selectedDatabase = browserService()->getDatabase(m_selectedDatabaseUuid);
    emit updateGroups();
}

void PasskeyImportDialog::changeEntry(int index)
{
    m_selectedEntryUuid = m_ui->selectEntryComboBox->itemData(index).value<QUuid>();
}

void PasskeyImportDialog::changeGroup(int index)
{
    m_selectedGroupUuid = m_ui->selectGroupComboBox->itemData(index).value<QUuid>();
    emit updateEntries();
}

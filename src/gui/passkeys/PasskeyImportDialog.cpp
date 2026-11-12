/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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
#include "core/EntrySearcher.h"
#include "core/Metadata.h"
#include "gui/MainWindow.h"
#include <QCloseEvent>
#include <QFileInfo>
#include <QKeyEvent>

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
    connect(m_ui->selectGroupComboBox, SIGNAL(currentIndexChanged(int)), SLOT(changeGroup(int)));
    connect(m_ui->entryTreeWidget,
            SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            SLOT(changeEntry(QTreeWidgetItem*)));
    connect(m_ui->entryTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(accept()));
    connect(m_ui->searchEntryLineEdit, SIGNAL(textChanged(QString)), SLOT(filterEntries(QString)));

    auto header = m_ui->entryTreeWidget->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // Allow navigating the entry list while typing in the search field
    m_ui->searchEntryLineEdit->installEventFilter(this);
}

PasskeyImportDialog::~PasskeyImportDialog()
{
}

void PasskeyImportDialog::setInfo(const QString& relyingParty,
                                  const QString& username,
                                  const QSharedPointer<Database>& database,
                                  bool isEntry,
                                  const QString& titleText,
                                  const QString& infoText,
                                  const QString& importButtonText)
{
    m_ui->relyingPartyLabel->setText(tr("Relying Party: %1").arg(relyingParty));
    m_ui->usernameLabel->setText(tr("Username: %1").arg(username));

    if (isEntry) {
        m_ui->verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        m_ui->infoLabel->setText(tr("Import the following passkey to this entry:"));
        m_ui->groupBox->setVisible(false);
    }

    m_selectedDatabase = database;
    addDatabases();
    addGroups();
    addEntries();

    auto openDatabaseCount = 0;
    for (auto dbWidget : getMainWindow()->getOpenDatabases()) {
        if (dbWidget && !dbWidget->isLocked()) {
            openDatabaseCount++;
        }
    }
    m_ui->selectDatabaseCombobBox->setEnabled(openDatabaseCount > 1);

    if (!titleText.isEmpty()) {
        setWindowTitle(titleText);
    }

    if (!infoText.isEmpty()) {
        m_ui->infoLabel->setText(infoText);
    }

    if (!importButtonText.isEmpty()) {
        m_ui->importButton->setText(importButtonText);
    }

    if (!isEntry) {
        m_ui->searchEntryLineEdit->setFocus();
    }
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
    m_entries.clear();
    m_ui->entryTreeWidget->clear();

    auto createNewItem = new QTreeWidgetItem(m_ui->entryTreeWidget, {tr("Create new entry")});
    createNewItem->setFirstColumnSpanned(true);
    auto font = createNewItem->font(0);
    font.setItalic(true);
    createNewItem->setFont(0, font);
    m_ui->entryTreeWidget->setCurrentItem(createNewItem);

    if (!m_selectedDatabase || !m_selectedDatabase->rootGroup()) {
        return;
    }

    // Collect all entries in the database and resolve the title
    QList<QPair<QString, Entry*>> entries;
    for (const auto entry : m_selectedDatabase->rootGroup()->entriesRecursive()) {
        if (!entry || entry->isRecycled()) {
            continue;
        }
        entries.append({entry->resolveMultiplePlaceholders(entry->title()), entry});
    }

    // Sort entries by title
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    for (const auto& pair : entries) {
        const auto entry = pair.second;
        auto item = new QTreeWidgetItem(
            m_ui->entryTreeWidget,
            {pair.first, entry->resolveMultiplePlaceholders(entry->username()), entry->group()->fullPath()});
        item->setData(0, Qt::UserRole, entry->uuid());
        m_entries.append(entry);
    }

    filterEntries(m_ui->searchEntryLineEdit->text());
}

void PasskeyImportDialog::addGroups()
{
    if (!m_selectedDatabase) {
        return;
    }

    m_ui->selectGroupComboBox->clear();
    m_ui->selectGroupComboBox->addItem(tr("Default passkeys group (Imported Passkeys)"), {});

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
    emit updateEntries();
}

void PasskeyImportDialog::changeEntry(QTreeWidgetItem* current)
{
    m_selectedEntryUuid = current ? current->data(0, Qt::UserRole).value<QUuid>() : QUuid();

    // The group is only used when a new entry is created
    m_ui->selectGroupComboBox->setEnabled(m_selectedEntryUuid.isNull());
}

void PasskeyImportDialog::changeGroup(int index)
{
    m_selectedGroupUuid = m_ui->selectGroupComboBox->itemData(index).value<QUuid>();
}

void PasskeyImportDialog::filterEntries(const QString& searchString)
{
    QList<Entry*> entries;
    for (const auto& entry : m_entries) {
        if (entry) {
            entries.append(entry);
        }
    }

    QSet<QUuid> matches;
    if (!searchString.isEmpty()) {
        EntrySearcher searcher;
        for (const auto entry : searcher.searchEntries(searchString, entries)) {
            matches.insert(entry->uuid());
        }
    }

    // The first item ("Create new entry") is always shown
    for (int i = 1; i < m_ui->entryTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_ui->entryTreeWidget->topLevelItem(i);
        item->setHidden(!searchString.isEmpty() && !matches.contains(item->data(0, Qt::UserRole).value<QUuid>()));
    }

    // Select the best match while searching
    auto current = m_ui->entryTreeWidget->currentItem();
    if (!searchString.isEmpty() || !current || current->isHidden()) {
        selectFirstVisibleEntry();
    }
}

void PasskeyImportDialog::selectFirstVisibleEntry()
{
    for (int i = 1; i < m_ui->entryTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_ui->entryTreeWidget->topLevelItem(i);
        if (!item->isHidden()) {
            m_ui->entryTreeWidget->setCurrentItem(item);
            return;
        }
    }

    m_ui->entryTreeWidget->setCurrentItem(m_ui->entryTreeWidget->topLevelItem(0));
}

bool PasskeyImportDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->searchEntryLineEdit && event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
            QCoreApplication::sendEvent(m_ui->entryTreeWidget, event);
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

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
#include "core/Metadata.h"
#include "gui/MainWindow.h"
#include "gui/group/GroupModel.h"
#include <QCloseEvent>
#include <QFileInfo>
#include <QSortFilterProxyModel>

// ---------------------------------------------------------------------------
// GroupModelNoRecycle
// Thin QSortFilterProxyModel that hides the recycle bin group (same pattern
// as DatabaseSettingsWidgetFdoSecrets::GroupModelNoRecycle).
// ---------------------------------------------------------------------------
class PasskeyImportDialog::GroupModelNoRecycle : public QSortFilterProxyModel
{
    Q_OBJECT

    Database* m_db;

public:
    explicit GroupModelNoRecycle(Database* db, QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_db(db)
    {
        Q_ASSERT(db);
        setSourceModel(new GroupModel(m_db, this));
    }

    Group* groupFromIndex(const QModelIndex& index) const
    {
        auto* groupModel = qobject_cast<GroupModel*>(sourceModel());
        Q_ASSERT(groupModel);
        return groupModel->groupFromIndex(mapToSource(index));
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const auto sourceIdx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!sourceIdx.isValid()) {
            return false;
        }
        auto* groupModel = qobject_cast<GroupModel*>(sourceModel());
        Q_ASSERT(groupModel);
        const auto* group = groupModel->groupFromIndex(sourceIdx);
        const auto* recycleBin = m_db->metadata()->recycleBin();
        return group && !group->isRecycled()
               && (!recycleBin || group->uuid() != recycleBin->uuid());
    }
};

// ---------------------------------------------------------------------------

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

    // When the user toggles between "default group" and "select group",
    // re-evaluate which group UUID is current.
    connect(m_ui->useDefaultGroupRadio, &QRadioButton::toggled,
            this, &PasskeyImportDialog::onGroupSelectionChanged);

    // When "Select group" becomes checked and nothing is selected yet,
    // auto-select the root item so the user always has a valid destination.
    connect(m_ui->selectCustomGroupRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked && m_groupModel
                && !m_ui->selectGroupTreeView->selectionModel()->hasSelection()) {
            const auto rootIdx = m_groupModel->index(0, 0);
            if (rootIdx.isValid()) {
                m_ui->selectGroupTreeView->selectionModel()->select(
                    rootIdx, QItemSelectionModel::SelectCurrent);
                m_ui->selectGroupTreeView->setCurrentIndex(rootIdx);
            }
        }
        onGroupSelectionChanged();
    });
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

    // Disconnect any signal still bound to the old selection model.
    if (auto* sm = m_ui->selectGroupTreeView->selectionModel()) {
        disconnect(sm, nullptr, this, nullptr);
    }

    // Reset to the default group and uncheck any custom selection.
    m_selectedGroupUuid = QUuid();
    {
        QSignalBlocker b(m_ui->useDefaultGroupRadio);
        m_ui->useDefaultGroupRadio->setChecked(true);
    }

    // (Re-)build the tree model, filtering out the recycle bin.
    m_groupModel.reset(new GroupModelNoRecycle(m_selectedDatabase.data(), this));
    m_ui->selectGroupTreeView->setModel(m_groupModel.data());
    m_ui->selectGroupTreeView->expandAll();

    connect(m_ui->selectGroupTreeView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &PasskeyImportDialog::onGroupSelectionChanged);

    emit updateEntries();
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

void PasskeyImportDialog::onGroupSelectionChanged()
{
    if (m_ui->useDefaultGroupRadio->isChecked()) {
        m_selectedGroupUuid = QUuid();
    } else if (m_groupModel) {
        const auto indexes = m_ui->selectGroupTreeView->selectionModel()->selectedIndexes();
        if (!indexes.isEmpty()) {
            const auto* group = m_groupModel->groupFromIndex(indexes.first());
            m_selectedGroupUuid = group ? group->uuid() : QUuid();
        }
    }
    emit updateEntries();
}

#include "PasskeyImportDialog.moc"

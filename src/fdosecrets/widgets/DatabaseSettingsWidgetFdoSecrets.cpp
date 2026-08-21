/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "DatabaseSettingsWidgetFdoSecrets.h"
#include "ui_DatabaseSettingsWidgetFdoSecrets.h"

#include "fdosecrets/ClientAuth.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/widgets/ClientAuthModels.h"
#include "fdosecrets/widgets/ClientRecordDialog.h"

#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/group/GroupModel.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace
{
    enum class ExposedGroup
    {
        None,
        Expose
    };
} // namespace

class DatabaseSettingsWidgetFdoSecrets::GroupModelNoRecycle : public QSortFilterProxyModel
{
    Q_OBJECT

    Database* m_db;

public:
    explicit GroupModelNoRecycle(Database* db)
        : m_db(db)
    {
        Q_ASSERT(db);
        setSourceModel(new GroupModel(m_db, this));
    }

    Group* groupFromIndex(const QModelIndex& index) const
    {
        return groupFromSourceIndex(mapToSource(index));
    }

    Group* groupFromSourceIndex(const QModelIndex& index) const
    {
        auto groupModel = qobject_cast<GroupModel*>(sourceModel());
        Q_ASSERT(groupModel);
        return groupModel->groupFromIndex(index);
    }

    QModelIndex indexFromGroup(Group* group) const
    {
        auto groupModel = qobject_cast<GroupModel*>(sourceModel());
        Q_ASSERT(groupModel);
        return mapFromSource(groupModel->index(group));
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        auto source_idx = sourceModel()->index(source_row, 0, source_parent);
        if (!source_idx.isValid()) {
            return false;
        }

        auto recycleBin = m_db->metadata()->recycleBin();
        if (!recycleBin) {
            return true;
        }

        // can not call mapFromSource, which internally calls filterAcceptsRow
        auto group = groupFromSourceIndex(source_idx);

        return group && !group->isRecycled() && group->uuid() != recycleBin->uuid();
    }
};

DatabaseSettingsWidgetFdoSecrets::DatabaseSettingsWidgetFdoSecrets(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetFdoSecrets)
{
    m_ui->setupUi(this);
    m_ui->buttonGroup->setId(m_ui->radioDonotExpose, static_cast<int>(ExposedGroup::None));
    m_ui->buttonGroup->setId(m_ui->radioExpose, static_cast<int>(ExposedGroup::Expose));

    // make sure there is at least a selection
    connect(m_ui->radioExpose, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked && !m_ui->selectGroup->selectionModel()->hasSelection()) {
            auto model = m_ui->selectGroup->model();
            if (model) {
                auto idx = model->index(0, 0);
                m_ui->selectGroup->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);
            }
        }
    });

    m_ui->authzWarning->setHidden(true);
    m_ui->authzWarning->setCloseButtonVisible(false);
    m_ui->authzWarning->setAutoHideTimeout(MessageWidget::DisableAutoHide);

    m_recordsModel = new FdoSecrets::ClientRecordsModel(this);
    m_ui->recordsView->setModel(m_recordsModel);
    // the name and the decisions get the width they need, the rules the rest
    using Model = FdoSecrets::ClientRecordsModel;
    m_ui->recordsView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->recordsView->horizontalHeader()->setSectionResizeMode(Model::ColumnRules, QHeaderView::Stretch);

    connect(m_ui->recordAddButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetFdoSecrets::addClientRecord);
    connect(m_ui->recordEditButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetFdoSecrets::editClientRecord);
    connect(
        m_ui->recordRemoveButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetFdoSecrets::removeClientRecord);
    connect(m_ui->recordsView, &QTableView::doubleClicked, this, &DatabaseSettingsWidgetFdoSecrets::editClientRecord);
    connect(m_ui->recordsView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &DatabaseSettingsWidgetFdoSecrets::updateRecordButtons);
    connect(
        m_recordsModel, &QAbstractItemModel::modelReset, this, &DatabaseSettingsWidgetFdoSecrets::updateOverlapWarning);
    connect(
        m_recordsModel, &QAbstractItemModel::modelReset, this, &DatabaseSettingsWidgetFdoSecrets::updateRecordButtons);
}

DatabaseSettingsWidgetFdoSecrets::~DatabaseSettingsWidgetFdoSecrets() = default;

void DatabaseSettingsWidgetFdoSecrets::loadSettings(QSharedPointer<Database> db)
{
    m_db = std::move(db);

    m_model.reset(new GroupModelNoRecycle(m_db.data()));
    m_ui->selectGroup->setModel(m_model.data());

    Group* recycleBin = nullptr;
    if (m_db->metadata() && m_db->metadata()->recycleBin()) {
        recycleBin = m_db->metadata()->recycleBin();
    }

    auto group = m_db->rootGroup()->findGroupByUuid(FdoSecrets::settings()->exposedGroup(m_db));
    if (!group || group->isRecycled() || (recycleBin && group->uuid() == recycleBin->uuid())) {
        m_ui->radioDonotExpose->setChecked(true);
    } else {
        auto idx = m_model->indexFromGroup(group);
        m_ui->selectGroup->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);
        // expand all its parents
        idx = idx.parent();
        while (idx.isValid()) {
            m_ui->selectGroup->expand(idx);
            idx = idx.parent();
        }
        m_ui->radioExpose->setChecked(true);
    }

    // the records are edited on a copy and only written when the settings are
    // saved, like every other page of this dialog
    m_records = FdoSecrets::loadClientRecords(m_db.data());
    m_removedRecords.clear();
    m_removedDecisions.clear();
    // the initial warning belongs to the page as it appears; only warnings
    // appearing later, as records are edited, are worth animating
    m_ui->authzWarning->setAnimate(false);
    refreshRecords();
    m_ui->authzWarning->setAnimate(true);

    settingsWarning();
    updateRecordButtons();
}

void DatabaseSettingsWidgetFdoSecrets::saveSettings()
{
    Q_ASSERT(m_db);
    Q_ASSERT(m_model);

    QUuid exposedGroup;
    switch (static_cast<ExposedGroup>(m_ui->buttonGroup->checkedId())) {
    case ExposedGroup::None:
        break;
    case ExposedGroup::Expose: {
        auto idx = m_ui->selectGroup->selectionModel()->selectedIndexes().takeFirst();
        Q_ASSERT(idx.isValid());
        exposedGroup = m_model->groupFromIndex(idx)->uuid();
        break;
    }
    }

    FdoSecrets::settings()->setExposedGroup(m_db, exposedGroup);

    // removals first: a record removed and re-added keeps the newer definition
    for (const auto& id : asConst(m_removedRecords)) {
        FdoSecrets::removeClientRecord(m_db.data(), id);
    }
    for (const auto& removed : asConst(m_removedDecisions)) {
        if (removed.first) {
            FdoSecrets::setEntryClientDecision(removed.first, removed.second, AuthDecision::Undecided);
        }
    }
    for (const auto& record : asConst(m_records)) {
        // unchanged records write the same value, which customData ignores
        FdoSecrets::saveClientRecord(m_db.data(), record);
    }
    m_removedRecords.clear();
    m_removedDecisions.clear();
}

void DatabaseSettingsWidgetFdoSecrets::addClientRecord()
{
    FdoSecrets::ClientRecordDialog dlg(m_db, {}, m_records, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_records << dlg.record();
        refreshRecords();
    }
}

void DatabaseSettingsWidgetFdoSecrets::editClientRecord()
{
    const auto row = m_ui->recordsView->currentIndex().row();
    const auto record = m_recordsModel->recordAt(row);
    if (!record.isValid()) {
        return;
    }
    FdoSecrets::ClientRecordDialog dlg(m_db, record, m_records, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    for (auto& staged : m_records) {
        if (staged.id == record.id) {
            staged = dlg.record();
            break;
        }
    }
    for (auto entry : dlg.removedEntries()) {
        m_removedDecisions << qMakePair(QPointer<Entry>(entry), record.id);
    }
    refreshRecords();
}

void DatabaseSettingsWidgetFdoSecrets::removeClientRecord()
{
    const auto record = m_recordsModel->recordAt(m_ui->recordsView->currentIndex().row());
    if (!record.isValid()) {
        return;
    }
    // no confirmation: nothing is written before the settings are saved, and
    // the dialog can still be cancelled
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records.at(i).id == record.id) {
            m_records.removeAt(i);
            break;
        }
    }
    // a record that was never saved has nothing to remove from the database
    if (FdoSecrets::loadClientRecord(m_db.data(), record.id).isValid()) {
        m_removedRecords << record.id;
    }
    refreshRecords();
}

void DatabaseSettingsWidgetFdoSecrets::refreshRecords()
{
    // decision counts come from the database, minus what is staged for removal
    QHash<FdoSecrets::DBusClientId, QPair<int, int>> counts;
    for (const auto* entry : m_db->rootGroup()->entriesRecursive()) {
        const auto decisions = FdoSecrets::entryClientDecisions(entry);
        for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
            if (m_removedDecisions.contains(qMakePair(QPointer<Entry>(const_cast<Entry*>(entry)), it.key()))) {
                continue;
            }
            auto& count = counts[it.key()];
            (it.value() == AuthDecision::Allowed ? count.first : count.second) += 1;
        }
    }
    m_recordsModel->setRecords(m_records, counts);
}

void DatabaseSettingsWidgetFdoSecrets::updateRecordButtons()
{
    const auto hasSelection = m_recordsModel->recordAt(m_ui->recordsView->currentIndex().row()).isValid();
    m_ui->recordEditButton->setEnabled(hasSelection);
    m_ui->recordRemoveButton->setEnabled(hasSelection);
}

void DatabaseSettingsWidgetFdoSecrets::updateOverlapWarning()
{
    const auto& records = m_recordsModel->records();
    QStringList overlapping;
    for (int i = 0; i < records.size(); ++i) {
        for (int j = i + 1; j < records.size(); ++j) {
            if (FdoSecrets::recordsOverlap(records.at(i), records.at(j))) {
                overlapping << tr("\"%1\" and \"%2\"").arg(records.at(i).name, records.at(j).name);
            }
        }
    }
    if (overlapping.isEmpty()) {
        m_ui->authzWarning->hideMessage();
    } else {
        m_ui->authzWarning->showMessage(tr("%1 can match the same client. Overlaps resolve to the denying record "
                                           "first, then the earliest created one.")
                                            .arg(overlapping.join(tr(", "))),
                                        MessageWidget::Warning);
    }
}

void DatabaseSettingsWidgetFdoSecrets::settingsWarning()
{
    if (FdoSecrets::settings()->isEnabled()) {
        m_ui->groupBox->setEnabled(true);
        m_ui->authzBox->setEnabled(true);
        m_ui->warningWidget->hideMessage();
    } else {
        m_ui->groupBox->setEnabled(false);
        m_ui->authzBox->setEnabled(false);
        m_ui->warningWidget->showMessage(tr("Enable Secret Service to access these settings."), MessageWidget::Warning);
        m_ui->warningWidget->setCloseButtonVisible(false);
        m_ui->warningWidget->setAutoHideTimeout(-1);
    }
}

#include "DatabaseSettingsWidgetFdoSecrets.moc"

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

#include "fdosecrets/FdoSecretsSettings.h"

#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/group/GroupModel.h"

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

    settingsWarning();
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
}

void DatabaseSettingsWidgetFdoSecrets::settingsWarning()
{
    if (FdoSecrets::settings()->isEnabled()) {
        m_ui->groupBox->setEnabled(true);
        m_ui->warningWidget->hideMessage();
    } else {
        m_ui->groupBox->setEnabled(false);
        m_ui->warningWidget->showMessage(tr("Enable Secret Service to access these settings."), MessageWidget::Warning);
        m_ui->warningWidget->setCloseButtonVisible(false);
        m_ui->warningWidget->setAutoHideTimeout(-1);
    }
}

#include "DatabaseSettingsWidgetFdoSecrets.moc"

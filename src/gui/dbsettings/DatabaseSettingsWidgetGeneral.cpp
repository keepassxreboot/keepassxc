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

#include "DatabaseSettingsWidgetGeneral.h"
#include "ui_DatabaseSettingsWidgetGeneral.h"

#include "core/Clock.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/MessageBox.h"

DatabaseSettingsWidgetGeneral::DatabaseSettingsWidgetGeneral(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetGeneral())
{
    m_ui->setupUi(this);

    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(toggled(bool)), m_ui->historyMaxItemsSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(toggled(bool)), m_ui->historyMaxSizeSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->autosaveDelayCheckBox, SIGNAL(toggled(bool)), m_ui->autosaveDelaySpinBox, SLOT(setEnabled(bool)));
}

DatabaseSettingsWidgetGeneral::~DatabaseSettingsWidgetGeneral() = default;

void DatabaseSettingsWidgetGeneral::initialize()
{
    Metadata* meta = m_db->metadata();

    m_ui->dbNameEdit->setText(meta->name());
    m_ui->dbDescriptionEdit->setText(meta->description());
    m_ui->recycleBinEnabledCheckBox->setChecked(meta->recycleBinEnabled());
    m_ui->defaultUsernameEdit->setText(meta->defaultUserName());
    m_ui->compressionCheckbox->setChecked(m_db->compressionAlgorithm() != Database::CompressionNone);

    if (meta->historyMaxItems() > -1) {
        m_ui->historyMaxItemsSpinBox->setValue(meta->historyMaxItems());
        m_ui->historyMaxItemsCheckBox->setChecked(true);
    } else {
        m_ui->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_ui->historyMaxItemsSpinBox->setEnabled(false);
        m_ui->historyMaxItemsCheckBox->setChecked(false);
    }
    int historyMaxSizeMiB = qRound(meta->historyMaxSize() / qreal(1024 * 1024));
    if (historyMaxSizeMiB > 0) {
        m_ui->historyMaxSizeSpinBox->setValue(historyMaxSizeMiB);
        m_ui->historyMaxSizeCheckBox->setChecked(true);
    } else {
        m_ui->historyMaxSizeSpinBox->setValue(qRound(Metadata::DefaultHistoryMaxSize / qreal(1024 * 1024)));
        m_ui->historyMaxSizeSpinBox->setEnabled(false);
        m_ui->historyMaxSizeCheckBox->setChecked(false);
    }
    if (meta->autosaveDelayMin() > 0) {
        m_ui->autosaveDelaySpinBox->setValue(meta->autosaveDelayMin());
        m_ui->autosaveDelayCheckBox->setChecked(true);
    } else {
        m_ui->autosaveDelayCheckBox->setChecked(false);
        m_ui->autosaveDelaySpinBox->setEnabled(false);
    }
}

void DatabaseSettingsWidgetGeneral::uninitialize()
{
}

void DatabaseSettingsWidgetGeneral::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_ui->dbNameEdit->setFocus();
}

bool DatabaseSettingsWidgetGeneral::save()
{
    auto* meta = m_db->metadata();

    if (!m_ui->recycleBinEnabledCheckBox->isChecked() && meta->recycleBinEnabled()) {
        auto recycleBin = meta->recycleBin();
        if (recycleBin && !recycleBin->isEmpty()) {
            auto result = MessageBox::question(this,
                                               tr("Delete Recycle Bin"),
                                               tr("Do you want to delete the current recycle bin and all its "
                                                  "contents?\nThis action is not reversible."),
                                               MessageBox::Delete | MessageBox::No,
                                               MessageBox::No);

            if (result == MessageBox::Delete) {
                recycleBin->deleteLater();
            } else {
                recycleBin->setName(recycleBin->name().append(tr(" (old)")));
                recycleBin->setIcon(Group::DefaultIconNumber);
            }
        }

        meta->setRecycleBin(nullptr);
    }

    m_db->setCompressionAlgorithm(m_ui->compressionCheckbox->isChecked() ? Database::CompressionGZip
                                                                         : Database::CompressionNone);

    meta->setName(m_ui->dbNameEdit->text());
    meta->setDescription(m_ui->dbDescriptionEdit->text());
    meta->setDefaultUserName(m_ui->defaultUsernameEdit->text());
    meta->setRecycleBinEnabled(m_ui->recycleBinEnabledCheckBox->isChecked());
    meta->setSettingsChanged(Clock::currentDateTimeUtc());

    bool truncate = false;

    int historyMaxItems;
    if (m_ui->historyMaxItemsCheckBox->isChecked()) {
        historyMaxItems = m_ui->historyMaxItemsSpinBox->value();
    } else {
        historyMaxItems = -1;
    }
    if (historyMaxItems != meta->historyMaxItems()) {
        meta->setHistoryMaxItems(historyMaxItems);
        truncate = true;
    }

    int historyMaxSize;
    if (m_ui->historyMaxSizeCheckBox->isChecked()) {
        historyMaxSize = m_ui->historyMaxSizeSpinBox->value() * 1048576;
    } else {
        historyMaxSize = -1;
    }
    if (historyMaxSize != meta->historyMaxSize()) {
        meta->setHistoryMaxSize(historyMaxSize);
        truncate = true;
    }

    int autosaveDelayMin = 0;
    if (m_ui->autosaveDelayCheckBox->isChecked()) {
        autosaveDelayMin = m_ui->autosaveDelaySpinBox->value();
    }
    meta->setAutosaveDelayMin(autosaveDelayMin);

    if (truncate) {
        const QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(false);
        for (Entry* entry : allEntries) {
            entry->truncateHistory();
        }
    }

    return true;
}

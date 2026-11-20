/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QListView>

#include "core/Clock.h"
#include "core/CustomData.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/OfflineCache.h"
#include "gui/DatabaseIcons.h"
#include "gui/DatabaseWidget.h"
#include "gui/IconModels.h"
#include "gui/MessageBox.h"

DatabaseSettingsWidgetGeneral::DatabaseSettingsWidgetGeneral(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetGeneral())
{
    m_ui->setupUi(this);

    connect(m_ui->dbPublicColorButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetGeneral::pickPublicColor);
    connect(m_ui->dbPublicColorClearButton, &QPushButton::clicked, this, [this] { setupPublicColorButton({}); });
    connect(m_ui->dbPublicIconButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetGeneral::pickPublicIcon);
    connect(m_ui->dbPublicIconClearButton, &QPushButton::clicked, this, [this] { setupPublicIconButton(-1); });

    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(toggled(bool)), m_ui->historyMaxItemsSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(toggled(bool)), m_ui->historyMaxSizeSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->autosaveDelayCheckBox, SIGNAL(toggled(bool)), m_ui->autosaveDelaySpinBox, SLOT(setEnabled(bool)));

    connect(m_ui->cloudFileCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && !m_ui->cloudFileSuggestCheckBox->isChecked() && m_db
            && OfflineCache::localOverride(m_db->filePath()) == OfflineCache::Override::Unset) {
            m_ui->cloudFileSuggestCheckBox->setChecked(true);
        }
        updateCloudStatusLabel();
    });
    connect(m_ui->cloudSyncNowButton, &QPushButton::clicked, this, [this] {
        for (QWidget* w = parentWidget(); w; w = w->parentWidget()) {
            if (auto* dbWidget = qobject_cast<DatabaseWidget*>(w)) {
                dbWidget->syncOfflineCacheNow();
                updateCloudStatusLabel();
                break;
            }
        }
    });
    connect(m_ui->cloudShowVersionsButton, &QPushButton::clicked, this, [this] {
        for (QWidget* w = parentWidget(); w; w = w->parentWidget()) {
            if (auto* dbWidget = qobject_cast<DatabaseWidget*>(w)) {
                dbWidget->showOfflineBackupVersions();
                updateCloudStatusLabel();
                break;
            }
        }
    });
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

    m_ui->dbPublicName->setText(m_db->publicName());
    setupPublicColorButton(m_db->publicColor());
    setupPublicIconButton(m_db->publicIcon());

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

    const bool hasFilePath = !m_db->filePath().isEmpty() && !m_db->isTemporaryDatabase();
    m_ui->cloudOfflineGroupBox->setEnabled(hasFilePath);
    const bool suggestedInDb =
        meta->customData() && meta->customData()->value(CustomData::CloudFile) == QStringLiteral("true");
    m_ui->cloudFileSuggestCheckBox->blockSignals(true);
    m_ui->cloudFileCheckBox->blockSignals(true);
    m_ui->cloudFileSuggestCheckBox->setChecked(suggestedInDb);
    m_ui->cloudFileCheckBox->setChecked(m_db->isCloudModeEnabled());
    m_ui->cloudFileCheckBox->blockSignals(false);
    m_ui->cloudFileSuggestCheckBox->blockSignals(false);
    updateCloudStatusLabel();
}

void DatabaseSettingsWidgetGeneral::uninitialize()
{
}

void DatabaseSettingsWidgetGeneral::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_ui->dbNameEdit->setFocus();
}

bool DatabaseSettingsWidgetGeneral::saveSettings()
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

    m_db->setPublicName(m_ui->dbPublicName->text());
    m_db->setPublicColor(m_ui->dbPublicColorButton->property("color").toString());
    m_db->setPublicIcon(m_ui->dbPublicIconButton->property("iconIndex").toInt());

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

    if (!m_db->filePath().isEmpty() && !m_db->isTemporaryDatabase()) {
        const bool wantCloud = m_ui->cloudFileCheckBox->isChecked();
        const bool wantSuggest = m_ui->cloudFileSuggestCheckBox->isChecked();
        const bool wasSuggest =
            meta->customData()->value(CustomData::CloudFile) == QStringLiteral("true");
        if (wantSuggest != wasSuggest) {
            if (wantSuggest) {
                meta->customData()->set(CustomData::CloudFile, QStringLiteral("true"));
            } else {
                meta->customData()->remove(CustomData::CloudFile);
            }
        }

        auto cache = OfflineCache::forDatabase(m_db->filePath());
        cache.setSuggestedByDatabase(wantSuggest);

        if (wantCloud) {
            OfflineCache::setLocalOverride(m_db->filePath(), OfflineCache::Override::Enabled);
            m_db->refreshOfflineCache(false);
        } else {
            if (cache.hasCacheFile() || !cache.versions().isEmpty()) {
                auto res = MessageBox::question(
                    this,
                    tr("Delete Offline Copy and Backups?"),
                    tr("Do you want to delete the local offline copy and backup versions for this database from this "
                       "computer?"),
                    MessageBox::Delete | MessageBox::No,
                    MessageBox::No);
                if (res == MessageBox::Delete) {
                    cache.removeAll();
                }
            }
            OfflineCache::setLocalOverride(m_db->filePath(), OfflineCache::Override::Disabled);
            m_db->setWorkingOffline(false);
        }
        emit m_db->cloudModeChanged();
    }

    return true;
}

void DatabaseSettingsWidgetGeneral::updateCloudStatusLabel()
{
    if (!m_db || m_db->filePath().isEmpty()) {
        m_ui->cloudStatusLabel->setText(tr("Save the database to a file first to enable an offline copy."));
        m_ui->cloudSyncNowButton->setEnabled(false);
        m_ui->cloudShowVersionsButton->setEnabled(false);
        return;
    }

    const auto cache = OfflineCache::forDatabase(m_db->filePath());
    const bool enabled = m_ui->cloudFileCheckBox->isChecked();
    m_ui->cloudSyncNowButton->setEnabled(enabled);
    m_ui->cloudShowVersionsButton->setEnabled(enabled || !cache.versions().isEmpty());

    if (!enabled) {
        m_ui->cloudStatusLabel->setText(tr("Offline copy is disabled on this computer."));
    } else if (m_db->isWorkingOffline()) {
        m_ui->cloudStatusLabel->setText(tr("Working offline. Changes are saved locally."));
    } else if (cache.hasCacheFile()) {
        m_ui->cloudStatusLabel->setText(tr("Offline copy is up to date."));
    } else {
        m_ui->cloudStatusLabel->setText(tr("Offline copy will be created when settings are saved."));
    }
}

void DatabaseSettingsWidgetGeneral::pickPublicColor()
{
    auto oldColor = QColor(m_ui->dbPublicColorButton->property("color").toString());
    auto newColor = QColorDialog::getColor(oldColor);
    if (newColor.isValid()) {
        setupPublicColorButton(newColor);
    }
}

void DatabaseSettingsWidgetGeneral::setupPublicColorButton(const QColor& color)
{
    m_ui->dbPublicColorClearButton->setVisible(color.isValid());
    if (color.isValid()) {
        m_ui->dbPublicColorButton->setStyleSheet(QString("background-color:%1").arg(color.name()));
        m_ui->dbPublicColorButton->setProperty("color", color.name());
    } else {
        m_ui->dbPublicColorButton->setStyleSheet("");
        m_ui->dbPublicColorButton->setProperty("color", {});
    }
}

void DatabaseSettingsWidgetGeneral::pickPublicIcon()
{
    QDialog dialog(this);
    dialog.setSizeGripEnabled(false);
    dialog.setWindowTitle(tr("Select Database Icon"));

    auto iconList = new QListView;
    iconList->setFlow(QListView::LeftToRight);
    iconList->setMovement(QListView::Static);
    iconList->setResizeMode(QListView::Adjust);
    iconList->setWrapping(true);
    iconList->setSpacing(4);

    auto iconModel = new DefaultIconModel;
    iconList->setModel(iconModel);
    if (m_ui->dbPublicIconButton->property("iconIndex").toInt() >= 0) {
        iconList->setCurrentIndex(iconModel->index(m_ui->dbPublicIconButton->property("iconIndex").toInt(), 0));
    } else {
        iconList->setCurrentIndex(iconModel->index(0, 0));
    }

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto layout = new QVBoxLayout(&dialog);
    layout->addWidget(iconList);
    layout->addWidget(buttonBox);

    // Resize the dialog to fit the default icon list
    auto cellSize = iconList->sizeHintForIndex(iconModel->index(0, 0));
    auto spacing = iconList->spacing() * 2;
    dialog.resize((cellSize.width() + spacing) * 15, (cellSize.height() + spacing) * 6 + 16);

    connect(iconList, &QListView::doubleClicked, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(
        &dialog, &QDialog::accepted, this, [this, iconList] { setupPublicIconButton(iconList->currentIndex().row()); });

    dialog.exec();
}

void DatabaseSettingsWidgetGeneral::setupPublicIconButton(int iconIndex)
{
    auto valid = iconIndex >= 0 && iconIndex < databaseIcons()->count();
    m_ui->dbPublicIconClearButton->setVisible(valid);
    if (valid) {
        m_ui->dbPublicIconButton->setIcon(databaseIcons()->icon(iconIndex));
        m_ui->dbPublicIconButton->setProperty("iconIndex", iconIndex);
        m_ui->dbPublicIconClearButton->setVisible(true);
    } else {
        m_ui->dbPublicIconButton->setIcon(QIcon());
        m_ui->dbPublicIconButton->setProperty("iconIndex", -1);
        m_ui->dbPublicIconClearButton->setVisible(false);
    }
}

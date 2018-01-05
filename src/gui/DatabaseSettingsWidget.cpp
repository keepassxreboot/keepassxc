/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseSettingsWidget.h"
#include "ui_DatabaseSettingsWidget.h"

#include <QMessageBox>

#include "core/Global.h"
#include "core/AsyncTask.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "MessageBox.h"

DatabaseSettingsWidget::DatabaseSettingsWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidget())
    , m_db(nullptr)
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxItemsSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxSizeSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->transformBenchmarkButton, SIGNAL(clicked()), SLOT(transformRoundsBenchmark()));
    connect(m_ui->kdfComboBox, SIGNAL(currentIndexChanged(int)), SLOT(kdfChanged(int)));
}

DatabaseSettingsWidget::~DatabaseSettingsWidget()
{
}

void DatabaseSettingsWidget::load(Database* db)
{
    m_db = db;

    Metadata* meta = m_db->metadata();

    m_ui->dbNameEdit->setText(meta->name());
    m_ui->dbDescriptionEdit->setText(meta->description());
    m_ui->recycleBinEnabledCheckBox->setChecked(meta->recycleBinEnabled());
    m_ui->defaultUsernameEdit->setText(meta->defaultUserName());
    if (meta->historyMaxItems() > -1) {
        m_ui->historyMaxItemsSpinBox->setValue(meta->historyMaxItems());
        m_ui->historyMaxItemsCheckBox->setChecked(true);
    } else {
        m_ui->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_ui->historyMaxItemsCheckBox->setChecked(false);
    }
    int historyMaxSizeMiB = qRound(meta->historyMaxSize() / qreal(1048576));
    if (historyMaxSizeMiB > 0) {
        m_ui->historyMaxSizeSpinBox->setValue(historyMaxSizeMiB);
        m_ui->historyMaxSizeCheckBox->setChecked(true);
    } else {
        m_ui->historyMaxSizeSpinBox->setValue(Metadata::DefaultHistoryMaxSize);
        m_ui->historyMaxSizeCheckBox->setChecked(false);
    }

    m_ui->algorithmComboBox->clear();
    for (auto& cipher: asConst(KeePass2::CIPHERS)) {
        m_ui->algorithmComboBox->addItem(cipher.second, cipher.first.toByteArray());
    }
    int cipherIndex = m_ui->algorithmComboBox->findData(m_db->cipher().toByteArray());
    if (cipherIndex > -1) {
        m_ui->algorithmComboBox->setCurrentIndex(cipherIndex);
    }

    // Setup kdf combo box
    m_ui->kdfComboBox->blockSignals(true);
    m_ui->kdfComboBox->clear();
    for (auto& kdf: asConst(KeePass2::KDFS)) {
        m_ui->kdfComboBox->addItem(kdf.second, kdf.first.toByteArray());
    }
    m_ui->kdfComboBox->blockSignals(false);

    auto kdfUuid = m_db->kdf()->uuid();
    int kdfIndex = m_ui->kdfComboBox->findData(kdfUuid.toByteArray());
    if (kdfIndex > -1) {
        m_ui->kdfComboBox->setCurrentIndex(kdfIndex);
        kdfChanged(kdfIndex);
    }

    // Setup kdf parameters
    auto kdf = m_db->kdf();
    m_ui->transformRoundsSpinBox->setValue(kdf->rounds());
    if (kdfUuid == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        m_ui->memorySpinBox->setValue(argon2Kdf->memory() / (1<<10));
        m_ui->parallelismSpinBox->setValue(argon2Kdf->parallelism());
    }

    m_ui->dbNameEdit->setFocus();
}

void DatabaseSettingsWidget::save()
{
    Metadata* meta = m_db->metadata();

    meta->setName(m_ui->dbNameEdit->text());
    meta->setDescription(m_ui->dbDescriptionEdit->text());
    meta->setDefaultUserName(m_ui->defaultUsernameEdit->text());
    meta->setRecycleBinEnabled(m_ui->recycleBinEnabledCheckBox->isChecked());
    meta->setSettingsChanged(QDateTime::currentDateTimeUtc());

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

    if (truncate) {
        truncateHistories();
    }

    m_db->setCipher(Uuid(m_ui->algorithmComboBox->currentData().toByteArray()));

    // Save kdf parameters
    auto kdf = KeePass2::uuidToKdf(Uuid(m_ui->kdfComboBox->currentData().toByteArray()));
    kdf->setRounds(m_ui->transformRoundsSpinBox->value());
    if (kdf->uuid() == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        argon2Kdf->setMemory(m_ui->memorySpinBox->value() * (1<<10));
        argon2Kdf->setParallelism(m_ui->parallelismSpinBox->value());
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    // TODO: we should probably use AsyncTask::runAndWaitForFuture() here,
    //       but not without making Database thread-safe
    bool ok = m_db->changeKdf(kdf);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        MessageBox::warning(this, tr("KDF unchanged"),
                            tr("Failed to transform key with new KDF parameters; KDF unchanged."),
                            QMessageBox::Ok);
    }
    emit editFinished(true);
}

void DatabaseSettingsWidget::reject()
{
    emit editFinished(false);
}

void DatabaseSettingsWidget::transformRoundsBenchmark()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_ui->transformBenchmarkButton->setEnabled(false);

    // Create a new kdf with the current parameters
    auto kdf = KeePass2::uuidToKdf(Uuid(m_ui->kdfComboBox->currentData().toByteArray()));
    kdf->setRounds(m_ui->transformRoundsSpinBox->value());
    if (kdf->uuid() == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        if (!argon2Kdf->setMemory(m_ui->memorySpinBox->value() * (1<<10))) {
            m_ui->memorySpinBox->setValue(argon2Kdf->memory() / (1<<10));
        }
        if (!argon2Kdf->setParallelism(m_ui->parallelismSpinBox->value())) {
            m_ui->parallelismSpinBox->setValue(argon2Kdf->parallelism());
        }
    }

    // Determine the number of rounds required to meet 1 second delay
    int rounds = AsyncTask::runAndWaitForFuture([this, kdf]() {
        return kdf->benchmark(1000);
    });

    m_ui->transformRoundsSpinBox->setValue(rounds);

    QApplication::restoreOverrideCursor();
    m_ui->transformBenchmarkButton->setEnabled(true);
}

void DatabaseSettingsWidget::truncateHistories()
{
    const QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(false);
    for (Entry* entry : allEntries) {
        entry->truncateHistory();
    }
}

void DatabaseSettingsWidget::kdfChanged(int index)
{
    Uuid id(m_ui->kdfComboBox->itemData(index).toByteArray());
    if (id == KeePass2::KDF_ARGON2) {
        m_ui->memorySpinBox->setEnabled(true);
        m_ui->parallelismSpinBox->setEnabled(true);
    } else {
        m_ui->memorySpinBox->setEnabled(false);
        m_ui->parallelismSpinBox->setEnabled(false);
    }
}

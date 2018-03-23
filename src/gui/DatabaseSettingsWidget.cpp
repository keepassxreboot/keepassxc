/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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
#include "ui_DatabaseSettingsWidgetEncryption.h"
#include "ui_DatabaseSettingsWidgetGeneral.h"

#include <QMessageBox>
#include <QPushButton>
#include <QThread>

#include "MessageBox.h"
#include "core/AsyncTask.h"
#include "core/Database.h"
#include "core/FilePath.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/kdf/Argon2Kdf.h"

DatabaseSettingsWidget::DatabaseSettingsWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidget())
    , m_uiGeneral(new Ui::DatabaseSettingsWidgetGeneral())
    , m_uiEncryption(new Ui::DatabaseSettingsWidgetEncryption())
    , m_uiGeneralPage(new QWidget())
    , m_uiEncryptionPage(new QWidget())
    , m_db(nullptr)
{
    m_ui->setupUi(this);
    m_uiGeneral->setupUi(m_uiGeneralPage);
    m_uiEncryption->setupUi(m_uiEncryptionPage);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_uiGeneral->historyMaxItemsCheckBox,
            SIGNAL(toggled(bool)),
            m_uiGeneral->historyMaxItemsSpinBox,
            SLOT(setEnabled(bool)));
    connect(m_uiGeneral->historyMaxSizeCheckBox,
            SIGNAL(toggled(bool)),
            m_uiGeneral->historyMaxSizeSpinBox,
            SLOT(setEnabled(bool)));
    connect(m_uiEncryption->transformBenchmarkButton, SIGNAL(clicked()), SLOT(transformRoundsBenchmark()));
    connect(m_uiEncryption->kdfComboBox, SIGNAL(currentIndexChanged(int)), SLOT(kdfChanged(int)));

    connect(m_uiEncryption->memorySpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryChanged(int)));
    connect(m_uiEncryption->parallelismSpinBox, SIGNAL(valueChanged(int)), this, SLOT(parallelismChanged(int)));

    m_ui->categoryList->addCategory(tr("General"), FilePath::instance()->icon("categories", "preferences-other"));
    m_ui->categoryList->addCategory(tr("Encryption"), FilePath::instance()->icon("actions", "document-encrypt"));
    m_ui->stackedWidget->addWidget(m_uiGeneralPage);
    m_ui->stackedWidget->addWidget(m_uiEncryptionPage);

    connect(m_ui->categoryList, SIGNAL(categoryChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));
}

DatabaseSettingsWidget::~DatabaseSettingsWidget()
{
}

void DatabaseSettingsWidget::load(Database* db)
{
    m_db = db;

    Metadata* meta = m_db->metadata();

    m_uiGeneral->dbNameEdit->setText(meta->name());
    m_uiGeneral->dbDescriptionEdit->setText(meta->description());
    m_uiGeneral->recycleBinEnabledCheckBox->setChecked(meta->recycleBinEnabled());
    m_uiGeneral->defaultUsernameEdit->setText(meta->defaultUserName());
    m_uiGeneral->compressionCheckbox->setChecked(m_db->compressionAlgo() != Database::CompressionNone);

    if (meta->historyMaxItems() > -1) {
        m_uiGeneral->historyMaxItemsSpinBox->setValue(meta->historyMaxItems());
        m_uiGeneral->historyMaxItemsCheckBox->setChecked(true);
    } else {
        m_uiGeneral->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_uiGeneral->historyMaxItemsCheckBox->setChecked(false);
    }
    int historyMaxSizeMiB = qRound(meta->historyMaxSize() / qreal(1048576));
    if (historyMaxSizeMiB > 0) {
        m_uiGeneral->historyMaxSizeSpinBox->setValue(historyMaxSizeMiB);
        m_uiGeneral->historyMaxSizeCheckBox->setChecked(true);
    } else {
        m_uiGeneral->historyMaxSizeSpinBox->setValue(Metadata::DefaultHistoryMaxSize);
        m_uiGeneral->historyMaxSizeCheckBox->setChecked(false);
    }

    m_uiEncryption->algorithmComboBox->clear();
    for (auto& cipher: asConst(KeePass2::CIPHERS)) {
        m_uiEncryption->algorithmComboBox->addItem(QCoreApplication::translate("KeePass2", cipher.second.toUtf8()), cipher.first);
    }
    int cipherIndex = m_uiEncryption->algorithmComboBox->findData(m_db->cipher().toRfc4122());
    if (cipherIndex > -1) {
        m_uiEncryption->algorithmComboBox->setCurrentIndex(cipherIndex);
    }

    // Setup kdf combo box
    m_uiEncryption->kdfComboBox->blockSignals(true);
    m_uiEncryption->kdfComboBox->clear();
    for (auto& kdf: asConst(KeePass2::KDFS)) {
        m_uiEncryption->kdfComboBox->addItem(QCoreApplication::translate("KeePass2", kdf.second.toUtf8()), kdf.first);
    }
    m_uiEncryption->kdfComboBox->blockSignals(false);

    auto kdfUuid = m_db->kdf()->uuid();
    int kdfIndex = m_uiEncryption->kdfComboBox->findData(kdfUuid);
    if (kdfIndex > -1) {
        m_uiEncryption->kdfComboBox->setCurrentIndex(kdfIndex);
        kdfChanged(kdfIndex);
    }

    m_uiEncryption->memorySpinBox->setValue(64);
    m_uiEncryption->parallelismSpinBox->setValue(QThread::idealThreadCount());

    // Setup kdf parameters
    auto kdf = m_db->kdf();
    m_uiEncryption->transformRoundsSpinBox->setValue(kdf->rounds());
    if (kdfUuid == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        m_uiEncryption->memorySpinBox->setValue(static_cast<int>(argon2Kdf->memory()) / (1 << 10));
        m_uiEncryption->parallelismSpinBox->setValue(argon2Kdf->parallelism());
    }

    m_uiGeneral->dbNameEdit->setFocus();
    m_ui->categoryList->setCurrentCategory(0);
}

void DatabaseSettingsWidget::save()
{
    // first perform safety check for KDF rounds
    auto kdf = KeePass2::uuidToKdf(m_uiEncryption->kdfComboBox->currentData().value<QUuid>());
    if (kdf->uuid() == KeePass2::KDF_ARGON2 && m_uiEncryption->transformRoundsSpinBox->value() > 10000) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(tr("Number of rounds too high", "Key transformation rounds"));
        warning.setText(tr("You are using a very high number of key transform rounds with Argon2.\n\n"
                           "If you keep this number, your database may take hours or days (or even longer) to open!"));
        auto ok = warning.addButton(tr("Understood, keep number"), QMessageBox::ButtonRole::AcceptRole);
        auto cancel = warning.addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);
        warning.setDefaultButton(cancel);
        warning.exec();
        if (warning.clickedButton() != ok) {
            return;
        }
    } else if ((kdf->uuid() == KeePass2::KDF_AES_KDBX3 || kdf->uuid() == KeePass2::KDF_AES_KDBX4)
               && m_uiEncryption->transformRoundsSpinBox->value() < 100000) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(tr("Number of rounds too low", "Key transformation rounds"));
        warning.setText(tr("You are using a very low number of key transform rounds with AES-KDF.\n\n"
                           "If you keep this number, your database may be too easy to crack!"));
        auto ok = warning.addButton(tr("Understood, keep number"), QMessageBox::ButtonRole::AcceptRole);
        auto cancel = warning.addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);
        warning.setDefaultButton(cancel);
        warning.exec();
        if (warning.clickedButton() != ok) {
            return;
        }
    }

    m_db->setCompressionAlgo(m_uiGeneral->compressionCheckbox->isChecked() ? Database::CompressionGZip
                                                                           : Database::CompressionNone);

    Metadata* meta = m_db->metadata();

    meta->setName(m_uiGeneral->dbNameEdit->text());
    meta->setDescription(m_uiGeneral->dbDescriptionEdit->text());
    meta->setDefaultUserName(m_uiGeneral->defaultUsernameEdit->text());
    meta->setRecycleBinEnabled(m_uiGeneral->recycleBinEnabledCheckBox->isChecked());
    meta->setSettingsChanged(QDateTime::currentDateTimeUtc());

    bool truncate = false;

    int historyMaxItems;
    if (m_uiGeneral->historyMaxItemsCheckBox->isChecked()) {
        historyMaxItems = m_uiGeneral->historyMaxItemsSpinBox->value();
    } else {
        historyMaxItems = -1;
    }
    if (historyMaxItems != meta->historyMaxItems()) {
        meta->setHistoryMaxItems(historyMaxItems);
        truncate = true;
    }

    int historyMaxSize;
    if (m_uiGeneral->historyMaxSizeCheckBox->isChecked()) {
        historyMaxSize = m_uiGeneral->historyMaxSizeSpinBox->value() * 1048576;
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

    m_db->setCipher(m_uiEncryption->algorithmComboBox->currentData().value<QUuid>());

    // Save kdf parameters
    kdf->setRounds(m_uiEncryption->transformRoundsSpinBox->value());
    if (kdf->uuid() == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        argon2Kdf->setMemory(static_cast<quint64>(m_uiEncryption->memorySpinBox->value()) * (1 << 10));
        argon2Kdf->setParallelism(static_cast<quint32>(m_uiEncryption->parallelismSpinBox->value()));
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    // TODO: we should probably use AsyncTask::runAndWaitForFuture() here,
    //       but not without making Database thread-safe
    bool ok = m_db->changeKdf(kdf);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        MessageBox::warning(this,
                            tr("KDF unchanged"),
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
    m_uiEncryption->transformBenchmarkButton->setEnabled(false);
    m_uiEncryption->transformRoundsSpinBox->setFocus();

    // Create a new kdf with the current parameters
    auto kdf = KeePass2::uuidToKdf(m_uiEncryption->kdfComboBox->currentData().value<QUuid>());
    kdf->setRounds(m_uiEncryption->transformRoundsSpinBox->value());
    if (kdf->uuid() == KeePass2::KDF_ARGON2) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        if (!argon2Kdf->setMemory(static_cast<quint64>(m_uiEncryption->memorySpinBox->value()) * (1 << 10))) {
            m_uiEncryption->memorySpinBox->setValue(static_cast<int>(argon2Kdf->memory() / (1 << 10)));
        }
        if (!argon2Kdf->setParallelism(static_cast<quint32>(m_uiEncryption->parallelismSpinBox->value()))) {
            m_uiEncryption->parallelismSpinBox->setValue(argon2Kdf->parallelism());
        }
    }

    // Determine the number of rounds required to meet 1 second delay
    int rounds = AsyncTask::runAndWaitForFuture([&kdf]() { return kdf->benchmark(1000); });

    m_uiEncryption->transformRoundsSpinBox->setValue(rounds);
    m_uiEncryption->transformBenchmarkButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
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
    QUuid id(m_uiEncryption->kdfComboBox->itemData(index).value<QUuid>());

    bool memoryEnabled = id == KeePass2::KDF_ARGON2;
    m_uiEncryption->memoryUsageLabel->setEnabled(memoryEnabled);
    m_uiEncryption->memorySpinBox->setEnabled(memoryEnabled);

    bool parallelismEnabled = id == KeePass2::KDF_ARGON2;
    m_uiEncryption->parallelismLabel->setEnabled(parallelismEnabled);
    m_uiEncryption->parallelismSpinBox->setEnabled(parallelismEnabled);

    transformRoundsBenchmark();
}

/**
 * Update memory spin box suffix on value change.
 */
void DatabaseSettingsWidget::memoryChanged(int value)
{
    m_uiEncryption->memorySpinBox->setSuffix(tr(" MiB", "Abbreviation for Mebibytes (KDF settings)", value));
}

/**
 * Update parallelism spin box suffix on value change.
 */
void DatabaseSettingsWidget::parallelismChanged(int value)
{
    m_uiEncryption->parallelismSpinBox->setSuffix(
        tr(" thread(s)", "Threads for parallel execution (KDF settings)", value));
}

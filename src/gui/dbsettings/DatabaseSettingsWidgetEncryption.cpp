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

#include "DatabaseSettingsWidgetEncryption.h"
#include "ui_DatabaseSettingsWidgetEncryption.h"

#include "core/AsyncTask.h"
#include "core/Database.h"
#include "core/Global.h"
#include "core/Metadata.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "format/KeePass2.h"
#include "format/KeePass2Writer.h"
#include "gui/MessageBox.h"

#include <QPushButton>

const char* DatabaseSettingsWidgetEncryption::CD_DECRYPTION_TIME_PREFERENCE_KEY = "KPXC_DECRYPTION_TIME_PREFERENCE";

#define IS_ARGON2(uuid) (uuid == KeePass2::KDF_ARGON2D || uuid == KeePass2::KDF_ARGON2ID)
#define IS_AES_KDF(uuid) (uuid == KeePass2::KDF_AES_KDBX3 || uuid == KeePass2::KDF_AES_KDBX4)

namespace
{
    QString getTextualEncryptionTime(int millisecs)
    {
        if (millisecs < 1000) {
            return QObject::tr("%1 ms", "milliseconds", millisecs).arg(millisecs);
        }
        return QObject::tr("%1 s", "seconds", millisecs / 1000).arg(millisecs / 1000.0, 0, 'f', 1);
    }
} // namespace

DatabaseSettingsWidgetEncryption::DatabaseSettingsWidgetEncryption(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetEncryption())
{
    m_ui->setupUi(this);

    connect(m_ui->transformBenchmarkButton, SIGNAL(clicked()), SLOT(benchmarkTransformRounds()));
    connect(m_ui->kdfComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateKdfFields()));
    connect(m_ui->compatibilitySelection, SIGNAL(currentIndexChanged(int)), SLOT(loadKdfAlgorithms()));
    m_ui->formatCannotBeChanged->setVisible(false);

    connect(m_ui->memorySpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryChanged(int)));
    connect(m_ui->parallelismSpinBox, SIGNAL(valueChanged(int)), this, SLOT(parallelismChanged(int)));

    m_ui->compatibilitySelection->addItem(tr("KDBX 4 (recommended)"), KeePass2::KDF_ARGON2D);
    m_ui->compatibilitySelection->addItem(tr("KDBX 3"), KeePass2::KDF_AES_KDBX3);
    m_ui->decryptionTimeSlider->setMinimum(Kdf::MIN_ENCRYPTION_TIME / 100);
    m_ui->decryptionTimeSlider->setMaximum(Kdf::MAX_ENCRYPTION_TIME / 100);
    m_ui->decryptionTimeSlider->setValue(Kdf::DEFAULT_ENCRYPTION_TIME / 100);
    updateDecryptionTime(m_ui->decryptionTimeSlider->value());

    m_ui->transformBenchmarkButton->setText(
        QObject::tr("Benchmark %1 delay").arg(getTextualEncryptionTime(Kdf::DEFAULT_ENCRYPTION_TIME)));
    m_ui->minTimeLabel->setText(getTextualEncryptionTime(Kdf::MIN_ENCRYPTION_TIME));
    m_ui->maxTimeLabel->setText(getTextualEncryptionTime(Kdf::MAX_ENCRYPTION_TIME));

    connect(m_ui->decryptionTimeSlider, SIGNAL(valueChanged(int)), SLOT(updateDecryptionTime(int)));

    // conditions under which a key re-transformation is needed
    connect(m_ui->decryptionTimeSlider, SIGNAL(valueChanged(int)), SLOT(markDirty()));
    connect(m_ui->compatibilitySelection, SIGNAL(currentIndexChanged(int)), SLOT(markDirty()));
    connect(m_ui->algorithmComboBox, SIGNAL(currentIndexChanged(int)), SLOT(markDirty()));
    connect(m_ui->kdfComboBox, SIGNAL(currentIndexChanged(int)), SLOT(markDirty()));
    connect(m_ui->transformRoundsSpinBox, SIGNAL(valueChanged(int)), SLOT(markDirty()));
    connect(m_ui->memorySpinBox, SIGNAL(valueChanged(int)), SLOT(markDirty()));
    connect(m_ui->parallelismSpinBox, SIGNAL(valueChanged(int)), SLOT(markDirty()));
}

DatabaseSettingsWidgetEncryption::~DatabaseSettingsWidgetEncryption() = default;

void DatabaseSettingsWidgetEncryption::showBasicEncryption(int decryptionMillisecs)
{
    // Show the basic encryption settings tab and set the slider to the stored values
    m_ui->decryptionTimeSlider->setValue(decryptionMillisecs / 100);
    m_ui->encryptionSettingsTabWidget->setCurrentWidget(m_ui->basicTab);
    m_initWithAdvanced = false;
}

void DatabaseSettingsWidgetEncryption::initialize()
{
    Q_ASSERT(m_db);
    if (!m_db) {
        return;
    }

    bool isNewDatabase = false;

    // Check for uninitialized database parameters and set initial values accordingly
    if (!m_db->key()) {
        m_db->setKey(QSharedPointer<CompositeKey>::create(), true, false, false);
        m_db->setKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D));
        m_db->setCipher(KeePass2::CIPHER_AES256);
        isNewDatabase = true;
    } else if (!m_db->kdf()) {
        m_db->setKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D));
        isNewDatabase = true;
    }

    // Initialize the basic settings tab

    // check if the DB's custom data has a decryption time setting stored
    // and set the slider to it, otherwise just state that the time is unchanged
    // (we cannot infer the time from the raw KDF settings)
    auto* cd = m_db->metadata()->customData();
    if (cd->hasKey(CD_DECRYPTION_TIME_PREFERENCE_KEY)) {
        int decryptionTime = qMax(100, cd->value(CD_DECRYPTION_TIME_PREFERENCE_KEY).toInt());
        showBasicEncryption(decryptionTime);
    } else if (isNewDatabase) {
        showBasicEncryption();
    } else {
        // Set default basic decryption time
        m_ui->decryptionTimeSlider->setValue(Kdf::DEFAULT_ENCRYPTION_TIME / 100);
        // Show the advanced encryption settings tab
        m_ui->encryptionSettingsTabWidget->setCurrentWidget(m_ui->advancedTab);
        m_initWithAdvanced = true;
    }

    // Initialize the advanced settings tab

    // Set up the KDBX version selector
    bool isKdbx3 = m_db->formatVersion() <= KeePass2::FILE_VERSION_3_1;
    m_ui->compatibilitySelection->blockSignals(true);
    m_ui->compatibilitySelection->setCurrentIndex(isKdbx3 ? KDBX3 : KDBX4);
    m_ui->compatibilitySelection->blockSignals(false);

    // Disable KDBX selector if downgrading would lose data
    if (!isKdbx3 && KeePass2Writer::kdbxVersionRequired(m_db.data(), true, true) >= KeePass2::FILE_VERSION_4) {
        m_ui->compatibilitySelection->setEnabled(false);
        m_ui->formatCannotBeChanged->setVisible(true);
    }

    // Set up encryption ciphers
    m_ui->algorithmComboBox->clear();
    for (auto& cipher : asConst(KeePass2::CIPHERS)) {
        m_ui->algorithmComboBox->addItem(KeePass2::cipherToString(cipher), cipher);
    }
    int cipherIndex = m_ui->algorithmComboBox->findData(m_db->cipher());
    if (cipherIndex > -1) {
        m_ui->algorithmComboBox->setCurrentIndex(cipherIndex);
    }

    // Set up KDF algorithms
    loadKdfAlgorithms();

    // Perform Benchmark if requested
    if (isNewDatabase) {
        if (IS_ARGON2(m_ui->kdfComboBox->currentData())) {
            m_ui->memorySpinBox->setValue(16);
            m_ui->parallelismSpinBox->setValue(2);
        }
        benchmarkTransformRounds();
    }

    // New databases always require saving
    m_isDirty = isNewDatabase;
}

void DatabaseSettingsWidgetEncryption::uninitialize()
{
}

void DatabaseSettingsWidgetEncryption::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (m_ui->decryptionTimeSlider->isVisible()) {
        m_ui->decryptionTimeSlider->setFocus();
    } else {
        m_ui->transformRoundsSpinBox->setFocus();
    }
}

void DatabaseSettingsWidgetEncryption::loadKdfAlgorithms()
{
    bool isKdbx3 = m_ui->compatibilitySelection->currentIndex() == KDBX3;

    m_ui->kdfComboBox->blockSignals(true);
    m_ui->kdfComboBox->clear();
    const auto& kdfs = isKdbx3 ? KeePass2::KDBX3_KDFS : KeePass2::KDBX4_KDFS;
    for (auto& kdf : kdfs) {
        m_ui->kdfComboBox->addItem(KeePass2::kdfToString(kdf), kdf);
        // Set current index to the current database KDF if it matches
        if (m_db && m_db->kdf() && m_db->kdf()->uuid() == kdf) {
            m_ui->kdfComboBox->setCurrentIndex(m_ui->kdfComboBox->count() - 1);
        }
    }
    m_ui->kdfComboBox->blockSignals(false);

    // Ensure consistency with current index
    updateKdfFields();
}

void DatabaseSettingsWidgetEncryption::loadKdfParameters()
{
    if (!m_db) {
        return;
    }

    auto kdf = m_db->kdf();
    if (!kdf) {
        return;
    }

    // Load database KDF parameters if equal to current choice
    bool dbIsArgon2 = IS_ARGON2(kdf->uuid());
    bool kdfIsArgon2 = IS_ARGON2(m_ui->kdfComboBox->currentData().toUuid());
    if (dbIsArgon2 && kdfIsArgon2) {
        // Set Argon2 parameters
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        m_ui->transformRoundsSpinBox->setValue(argon2Kdf->rounds());
        m_ui->memorySpinBox->setValue(static_cast<int>(argon2Kdf->memory()) / (1 << 10));
        m_ui->parallelismSpinBox->setValue(argon2Kdf->parallelism());
    } else if (!dbIsArgon2 && !kdfIsArgon2) {
        // Set AES KDF parameters
        m_ui->transformRoundsSpinBox->setValue(kdf->rounds());
    } else {
        // Set reasonable defaults and then benchmark
        if (kdfIsArgon2) {
            m_ui->memorySpinBox->setValue(16);
            m_ui->parallelismSpinBox->setValue(2);
        }
        benchmarkTransformRounds();
    }
}

void DatabaseSettingsWidgetEncryption::updateKdfFields()
{
    bool isArgon2 = IS_ARGON2(m_ui->kdfComboBox->currentData().toUuid());
    m_ui->memoryUsageLabel->setVisible(isArgon2);
    m_ui->memorySpinBox->setVisible(isArgon2);
    m_ui->parallelismLabel->setVisible(isArgon2);
    m_ui->parallelismSpinBox->setVisible(isArgon2);

    loadKdfParameters();
}

void DatabaseSettingsWidgetEncryption::markDirty()
{
    m_isDirty = true;
}

bool DatabaseSettingsWidgetEncryption::save()
{
    Q_ASSERT(m_db);
    if (!m_db) {
        return false;
    }

    if (m_initWithAdvanced != isAdvancedMode()) {
        // Switched from basic <-> advanced mode, need to recalculate everything
        m_isDirty = true;
    }

    if (m_db->key() && !m_db->key()->keys().isEmpty() && !m_isDirty) {
        // nothing has changed, don't re-transform
        return true;
    }

    if (!isAdvancedMode()) {
        // Basic mode maintains current database KDF
        auto kdf = m_db->kdf();
        Q_ASSERT(kdf);
        if (kdf && !m_isDirty && !m_ui->decryptionTimeSettings->isVisible()) {
            return true;
        }

        QApplication::setOverrideCursor(Qt::BusyCursor);

        int time = m_ui->decryptionTimeSlider->value() * 100;
        int rounds = AsyncTask::runAndWaitForFuture([&kdf, time]() { return kdf->benchmark(time); });
        kdf->setRounds(rounds);

        // TODO: we should probably use AsyncTask::runAndWaitForFuture() here,
        //       but not without making Database thread-safe
        bool ok = m_db->changeKdf(kdf);

        QApplication::restoreOverrideCursor();

        m_db->metadata()->customData()->set(CD_DECRYPTION_TIME_PREFERENCE_KEY, QString("%1").arg(time));

        return ok;
    }

    // Advanced mode sets KDF
    auto kdfChoice = m_ui->kdfComboBox->currentData().toUuid();

    // first perform safety check for KDF rounds
    if (IS_ARGON2(kdfChoice) && m_ui->transformRoundsSpinBox->value() > 10000) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(tr("Number of rounds too high", "Key transformation rounds"));
        warning.setText(tr("You are using a very high number of key transform rounds with Argon2.\n\n"
                           "If you keep this number, your database may take hours, days, or even longer to open."));
        auto ok = warning.addButton(tr("Understood, keep number"), QMessageBox::ButtonRole::AcceptRole);
        auto cancel = warning.addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);
        warning.setDefaultButton(cancel);
        warning.layout()->setSizeConstraint(QLayout::SetMinimumSize);
        warning.exec();
        if (warning.clickedButton() != ok) {
            return false;
        }
    } else if (IS_AES_KDF(kdfChoice) && m_ui->transformRoundsSpinBox->value() < 100000) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(tr("Number of rounds too low", "Key transformation rounds"));
        warning.setText(tr("You are using a very low number of key transform rounds with AES-KDF.\n\n"
                           "If you keep this number, your database will not be protected from brute force attacks."));
        auto ok = warning.addButton(tr("Understood, keep number"), QMessageBox::ButtonRole::AcceptRole);
        auto cancel = warning.addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);
        warning.setDefaultButton(cancel);
        warning.layout()->setSizeConstraint(QLayout::SetMinimumSize);
        warning.exec();
        if (warning.clickedButton() != ok) {
            return false;
        }
    }

    m_db->setCipher(m_ui->algorithmComboBox->currentData().toUuid());

    // remove a stored decryption time from custom data when advanced settings are used
    // we don't know it until we actually run the KDF
    m_db->metadata()->customData()->remove(CD_DECRYPTION_TIME_PREFERENCE_KEY);

    // Save kdf parameters
    auto kdf = KeePass2::uuidToKdf(kdfChoice);
    kdf->setRounds(m_ui->transformRoundsSpinBox->value());
    if (IS_ARGON2(kdf->uuid())) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        argon2Kdf->setMemory(static_cast<quint64>(m_ui->memorySpinBox->value()) * (1 << 10));
        argon2Kdf->setParallelism(static_cast<quint32>(m_ui->parallelismSpinBox->value()));
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
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

    return ok;
}

void DatabaseSettingsWidgetEncryption::benchmarkTransformRounds(int millisecs)
{
    QApplication::setOverrideCursor(Qt::BusyCursor);
    m_ui->transformBenchmarkButton->setEnabled(false);
    m_ui->transformRoundsSpinBox->setEnabled(false);
    m_ui->transformRoundsSpinBox->clear();

    // Create a new kdf with the current parameters
    auto kdf = KeePass2::uuidToKdf(m_ui->kdfComboBox->currentData().toUuid());
    kdf->setRounds(m_ui->transformRoundsSpinBox->value());
    if (IS_ARGON2(kdf->uuid())) {
        auto argon2Kdf = kdf.staticCast<Argon2Kdf>();
        // Set a small static number of rounds for the benchmark
        argon2Kdf->setRounds(4);
        if (!argon2Kdf->setMemory(static_cast<quint64>(m_ui->memorySpinBox->value()) * (1 << 10))) {
            m_ui->memorySpinBox->setValue(static_cast<int>(argon2Kdf->memory() / (1 << 10)));
        }
        if (!argon2Kdf->setParallelism(static_cast<quint32>(m_ui->parallelismSpinBox->value()))) {
            m_ui->parallelismSpinBox->setValue(argon2Kdf->parallelism());
        }
    }

    // Determine the number of rounds required to meet 1 second delay
    int rounds = AsyncTask::runAndWaitForFuture([&kdf, millisecs]() { return kdf->benchmark(millisecs); });

    m_ui->transformRoundsSpinBox->setValue(rounds);
    m_ui->transformBenchmarkButton->setEnabled(true);
    m_ui->transformRoundsSpinBox->setEnabled(true);
    m_ui->transformRoundsSpinBox->setFocus();
    m_ui->decryptionTimeSlider->setValue(millisecs / 100);
    QApplication::restoreOverrideCursor();
}

/**
 * Update memory spin box suffix on value change.
 */
void DatabaseSettingsWidgetEncryption::memoryChanged(int value)
{
    m_ui->memorySpinBox->setSuffix(tr(" MiB", "Abbreviation for Mebibytes (KDF settings)", value));
}

/**
 * Update parallelism spin box suffix on value change.
 */
void DatabaseSettingsWidgetEncryption::parallelismChanged(int value)
{
    m_ui->parallelismSpinBox->setSuffix(tr(" thread(s)", "Threads for parallel execution (KDF settings)", value));
}

bool DatabaseSettingsWidgetEncryption::isAdvancedMode()
{
    return m_ui->encryptionSettingsTabWidget->currentWidget() == m_ui->advancedTab;
}

void DatabaseSettingsWidgetEncryption::updateDecryptionTime(int value)
{
    m_ui->decryptionTimeValueLabel->setText(getTextualEncryptionTime(value * 100));
}

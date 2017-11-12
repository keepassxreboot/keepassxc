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

#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QMessageBox>
#include <QDialogButtonBox>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/SymmetricCipher.h"
#include "format/KeePass2.h"
#include "keys/CompositeKey.h"
#include "format/KeePass2.h"
#include "crypto/kdf/Kdf.h"
#include "MessageBox.h"

DatabaseSettingsWidget::DatabaseSettingsWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidget())
    , m_benchmarkField(nullptr)
    , m_db(nullptr)
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxItemsSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxSizeSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->kdfComboBox, SIGNAL(currentIndexChanged(int)), SLOT(changeKdf(int)));
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
    m_ui->AlgorithmComboBox->setCurrentIndex(SymmetricCipher::cipherToAlgorithm(m_db->cipher()));
    if (meta->historyMaxItems() > -1) {
        m_ui->historyMaxItemsSpinBox->setValue(meta->historyMaxItems());
        m_ui->historyMaxItemsCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_ui->historyMaxItemsCheckBox->setChecked(false);
    }
    int historyMaxSizeMiB = qRound(meta->historyMaxSize() / qreal(1048576));
    if (historyMaxSizeMiB > 0) {
        m_ui->historyMaxSizeSpinBox->setValue(historyMaxSizeMiB);
        m_ui->historyMaxSizeCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxSizeSpinBox->setValue(Metadata::DefaultHistoryMaxSize);
        m_ui->historyMaxSizeCheckBox->setChecked(false);
    }

    bool blockSignals = m_ui->kdfComboBox->signalsBlocked();
    m_ui->kdfComboBox->blockSignals(true);
    m_kdf.reset(m_db->kdf()->clone());
    m_ui->kdfComboBox->clear();
    for (QList<KeePass2::UuidNamePair>::const_iterator kdfs = KeePass2::KDFS.constBegin();
         kdfs != KeePass2::KDFS.constEnd(); ++kdfs) {
        KeePass2::UuidNamePair kdf = *kdfs;
        m_ui->kdfComboBox->addItem(kdf.name(), kdf.uuid().toByteArray());
    }
    int kdfIndex = m_ui->kdfComboBox->findData(KeePass2::kdfToUuid(*m_kdf).toByteArray());
    if (kdfIndex > -1) {
        m_ui->kdfComboBox->setCurrentIndex(kdfIndex);
    }
    displayKdf(*m_kdf);
    m_ui->kdfComboBox->blockSignals(blockSignals);

    m_ui->dbNameEdit->setFocus();
}

void DatabaseSettingsWidget::save()
{
    Metadata* meta = m_db->metadata();

    meta->setName(m_ui->dbNameEdit->text());
    meta->setDescription(m_ui->dbDescriptionEdit->text());
    meta->setDefaultUserName(m_ui->defaultUsernameEdit->text());
    m_db->setCipher(SymmetricCipher::algorithmToCipher(static_cast<SymmetricCipher::Algorithm>
                                                       (m_ui->AlgorithmComboBox->currentIndex())));
    meta->setRecycleBinEnabled(m_ui->recycleBinEnabledCheckBox->isChecked());

    bool truncate = false;

    int historyMaxItems;
    if (m_ui->historyMaxItemsCheckBox->isChecked()) {
        historyMaxItems = m_ui->historyMaxItemsSpinBox->value();
    }
    else {
        historyMaxItems = -1;
    }
    if (historyMaxItems != meta->historyMaxItems()) {
        meta->setHistoryMaxItems(historyMaxItems);
        truncate = true;
    }

    int historyMaxSize;
    if (m_ui->historyMaxSizeCheckBox->isChecked()) {
        historyMaxSize = m_ui->historyMaxSizeSpinBox->value() * 1048576;
    }
    else {
        historyMaxSize = -1;
    }
    if (historyMaxSize != meta->historyMaxSize()) {
        meta->setHistoryMaxSize(historyMaxSize);
        truncate = true;
    }

    if (truncate) {
        truncateHistories();
    }

    bool kdfValid = true;
    for (int i = 0; i < m_kdfFields.size(); ++i) {
        QPair<quint32, QSpinBox*> field = m_kdfFields.at(i);
        kdfValid &= m_kdf->setField(field.first, static_cast<quint64>(qMax(0, field.second->value())));
        if (!kdfValid) {
            break;
        }
    }

    if (kdfValid) {
        Kdf* kdf = m_kdf.take();
        bool ok = m_db->changeKdf(kdf);
        if (!ok) {
            MessageBox::warning(this, tr("KDF unchanged"),
                                tr("Failed to transform key with new KDF parameters; KDF unchanged."),
                                QMessageBox::Ok);
            delete kdf; // m_db has not taken ownership
        }
    } else {
        MessageBox::warning(this, tr("KDF unchanged"),
                            tr("Invalid KDF parameters; KDF unchanged."),
                            QMessageBox::Ok);
    }
    clearKdfWidgets();

    emit editFinished(true);
}

void DatabaseSettingsWidget::reject()
{
    emit editFinished(false);
}

void DatabaseSettingsWidget::transformRoundsBenchmark()
{
    if (m_benchmarkField == nullptr) {
        Q_ASSERT(false);
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_benchmarkField->setValue(m_kdf->benchmark(1000));
    QApplication::restoreOverrideCursor();
}

void DatabaseSettingsWidget::truncateHistories()
{
    const QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(false);
    for (Entry* entry : allEntries) {
        entry->truncateHistory();
    }
}

void DatabaseSettingsWidget::changeKdf(int index) {
    QByteArray uuidBytes = m_ui->kdfComboBox->itemData(index).toByteArray();
    if (uuidBytes.size() != Uuid::Length) {
        return;
    }
    Kdf* newKdf = KeePass2::uuidToKdf(Uuid(uuidBytes));
    if (newKdf != nullptr) {
        m_kdf.reset(newKdf);
        displayKdf(*m_kdf);
    }
}

void DatabaseSettingsWidget::displayKdf(const Kdf& kdf)
{
    clearKdfWidgets();

    QWidget* lastWidget = nullptr;
    int columnStart = m_ui->gridLayout->columnCount();
    int rowStart = m_ui->gridLayout->rowCount();
    QList<Kdf::Field> fields = kdf.fields();
    for (int i = 0; i < fields.size(); i++) {
        const Kdf::Field& field = fields.at(i);
        QLabel* label = new QLabel(QString("%1:").arg(field.name()));
        QSpinBox* spinBox = new QSpinBox();
        m_kdfWidgets.append(label);
        m_kdfWidgets.append(spinBox);
        m_kdfFields.append(QPair<quint32, QSpinBox*>(field.id(), spinBox));
        spinBox->setMinimum(static_cast<qint32>(qMin(qMax(0ull, field.min()), 0x7FFFFFFFull)));
        spinBox->setMaximum(static_cast<qint32>(qMin(qMax(0ull, field.max()), 0x7FFFFFFFull)));
        spinBox->setValue(static_cast<qint32>(qMin(qMax(0ull, kdf.field(field.id())), 0x7FFFFFFFull)));
        spinBox->setObjectName(QString("kdfParams%1").arg(i));
        m_ui->gridLayout->addWidget(label, rowStart + i, columnStart - 3, Qt::AlignRight);
        if (field.benchmarked()) {
            Q_ASSERT(m_benchmarkField == nullptr);
            QPushButton* benchBtn = new QPushButton("Benchmark");
            connect(benchBtn, &QPushButton::clicked, this, &DatabaseSettingsWidget::transformRoundsBenchmark);
            m_kdfWidgets.append(benchBtn);
            m_ui->gridLayout->addWidget(spinBox, rowStart + i, columnStart - 2);
            m_ui->gridLayout->addWidget(benchBtn, rowStart + i, columnStart - 1);
            m_benchmarkField = spinBox;
            lastWidget = benchBtn;
        } else {
            m_ui->gridLayout->addWidget(spinBox, rowStart + i, columnStart - 2, 1, 2);
            lastWidget = spinBox;
        }
    }
    if (lastWidget != nullptr) {
        QWidget::setTabOrder(lastWidget, m_ui->buttonBox->button(QDialogButtonBox::StandardButton::Cancel));
        QWidget::setTabOrder(m_ui->buttonBox->button(QDialogButtonBox::StandardButton::Cancel), m_ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok));
    }
}

void DatabaseSettingsWidget::clearKdfWidgets()
{
    m_benchmarkField = nullptr;
    for (int i = 0; i < m_kdfWidgets.size(); ++i) {
        m_ui->gridLayout->removeWidget(m_kdfWidgets.at(i));
        m_kdfWidgets.at(i)->deleteLater();
    }
    m_kdfWidgets.clear();
    m_kdfFields.clear();
}

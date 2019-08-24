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

#include "SettingsWidgetKeeShare.h"
#include "ui_SettingsWidgetKeeShare.h"

#include "core/Config.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/FileDialog.h"
#include "keeshare/KeeShare.h"
#include "keeshare/KeeShareSettings.h"

#include <QMessageBox>
#include <QStandardItemModel>

SettingsWidgetKeeShare::SettingsWidgetKeeShare(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidgetKeeShare())
{
    m_ui->setupUi(this);

#if !defined(WITH_XC_KEESHARE_SECURE)
    // Setting does not help the user of Version without signed export
    m_ui->ownCertificateGroupBox->setVisible(false);
#endif

    connect(m_ui->ownCertificateSignerEdit, SIGNAL(textChanged(QString)), SLOT(setVerificationExporter(QString)));

    connect(m_ui->generateOwnCerticateButton, SIGNAL(clicked(bool)), SLOT(generateCertificate()));
    connect(m_ui->importOwnCertificateButton, SIGNAL(clicked(bool)), SLOT(importCertificate()));
    connect(m_ui->exportOwnCertificateButton, SIGNAL(clicked(bool)), SLOT(exportCertificate()));

    connect(m_ui->trustImportedCertificateButton, SIGNAL(clicked(bool)), SLOT(trustSelectedCertificates()));
    connect(m_ui->askImportedCertificateButton, SIGNAL(clicked(bool)), SLOT(askSelectedCertificates()));
    connect(m_ui->untrustImportedCertificateButton, SIGNAL(clicked(bool)), SLOT(untrustSelectedCertificates()));
    connect(m_ui->removeImportedCertificateButton, SIGNAL(clicked(bool)), SLOT(removeSelectedCertificates()));
}

SettingsWidgetKeeShare::~SettingsWidgetKeeShare()
{
}

void SettingsWidgetKeeShare::loadSettings()
{
    const auto active = KeeShare::active();
    m_ui->enableExportCheckBox->setChecked(active.out);
    m_ui->enableImportCheckBox->setChecked(active.in);

    m_own = KeeShare::own();
    updateOwnCertificate();

    m_foreign = KeeShare::foreign();
    updateForeignCertificates();
}

void SettingsWidgetKeeShare::updateForeignCertificates()
{
    auto headers = QStringList() << tr("Path") << tr("Status");
#if defined(WITH_XC_KEESHARE_SECURE)
    headers << tr("Signer") << tr("Fingerprint") << tr("Certificate");
#endif

    m_importedCertificateModel.reset(new QStandardItemModel());
    m_importedCertificateModel->setHorizontalHeaderLabels(headers);

    for (const auto& scopedCertificate : m_foreign.certificates) {
        const auto items = QList<QStandardItem*>()
                           << new QStandardItem(scopedCertificate.path)
                           << new QStandardItem(scopedCertificate.trust == KeeShareSettings::Trust::Ask
                                                    ? tr("Ask")
                                                    : (scopedCertificate.trust == KeeShareSettings::Trust::Trusted
                                                           ? tr("Trusted")
                                                           : tr("Untrusted")))
#if defined(WITH_XC_KEESHARE_SECURE)
                           << new QStandardItem(scopedCertificate.isKnown() ? scopedCertificate.certificate.signer
                                                                            : tr("Unknown"))
                           << new QStandardItem(scopedCertificate.certificate.fingerprint())
                           << new QStandardItem(scopedCertificate.certificate.publicKey())
#endif
            ;
        m_importedCertificateModel->appendRow(items);
    }

    m_ui->importedCertificateTableView->setModel(m_importedCertificateModel.data());
    m_ui->importedCertificateTableView->resizeColumnsToContents();
}

void SettingsWidgetKeeShare::updateOwnCertificate()
{
    m_ui->ownCertificateSignerEdit->setText(m_own.certificate.signer);
    m_ui->ownCertificatePublicKeyEdit->setText(m_own.certificate.publicKey());
    m_ui->ownCertificatePrivateKeyEdit->setText(m_own.key.privateKey());
    m_ui->ownCertificateFingerprintEdit->setText(m_own.certificate.fingerprint());
}

void SettingsWidgetKeeShare::saveSettings()
{
    KeeShareSettings::Active active;
    active.out = m_ui->enableExportCheckBox->isChecked();
    active.in = m_ui->enableImportCheckBox->isChecked();
    // TODO HNH: This depends on the order of saving new data - a better model would be to
    //           store changes to the settings in a temporary object and check on the final values
    //           of this object (similar scheme to Entry) - this way we could validate the settings before save
    KeeShare::setOwn(m_own);
    KeeShare::setForeign(m_foreign);
    KeeShare::setActive(active);

    config()->set("KeeShare/QuietSuccess", m_ui->quietSuccessCheckBox->isChecked());
}

void SettingsWidgetKeeShare::setVerificationExporter(const QString& signer)
{
    m_own.certificate.signer = signer;
    m_ui->ownCertificateSignerEdit->setText(m_own.certificate.signer);
}

void SettingsWidgetKeeShare::generateCertificate()
{
    m_own = KeeShareSettings::Own::generate();
    m_ui->ownCertificateSignerEdit->setText(m_own.certificate.signer);
    m_ui->ownCertificatePublicKeyEdit->setText(m_own.certificate.publicKey());
    m_ui->ownCertificatePrivateKeyEdit->setText(m_own.key.privateKey());
    m_ui->ownCertificateFingerprintEdit->setText(m_own.certificate.fingerprint());
}

void SettingsWidgetKeeShare::importCertificate()
{
    QString defaultDirPath = config()->get("KeeShare/LastKeyDir").toString();
    const bool dirExists = !defaultDirPath.isEmpty() && QDir(defaultDirPath).exists();
    if (!dirExists) {
        defaultDirPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
    }
    const auto filetype = tr("key.share", "Filetype for KeeShare key");
    const auto filters = QString("%1 (*." + filetype + ");;%2 (*)").arg(tr("KeeShare key file"), tr("All files"));
    QString filename = fileDialog()->getOpenFileName(
        this, tr("Select path"), defaultDirPath, filters, nullptr, QFileDialog::Options(0));
    if (filename.isEmpty()) {
        return;
    }
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    m_own = KeeShareSettings::Own::deserialize(stream.readAll());
    file.close();
    config()->set("KeeShare/LastKeyDir", QFileInfo(filename).absolutePath());

    updateOwnCertificate();
}

void SettingsWidgetKeeShare::exportCertificate()
{
    if (KeeShare::own() != m_own) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(tr("Exporting changed certificate"));
        warning.setText(tr("The exported certificate is not the same as the one in use. Do you want to export the "
                           "current certificate?"));
        auto yes = warning.addButton(QMessageBox::StandardButton::Yes);
        auto no = warning.addButton(QMessageBox::StandardButton::No);
        warning.setDefaultButton(no);
        warning.exec();
        if (warning.clickedButton() != yes) {
            return;
        }
    }
    QString defaultDirPath = config()->get("KeeShare/LastKeyDir").toString();
    const bool dirExists = !defaultDirPath.isEmpty() && QDir(defaultDirPath).exists();
    if (!dirExists) {
        defaultDirPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
    }
    const auto filetype = tr("key.share", "Filetype for KeeShare key");
    const auto filters = QString("%1 (*." + filetype + ");;%2 (*)").arg(tr("KeeShare key file"), tr("All files"));
    QString filename = QString("%1.%2").arg(m_own.certificate.signer).arg(filetype);
    filename = fileDialog()->getSaveFileName(
        this, tr("Select path"), defaultDirPath, filters, nullptr, QFileDialog::Options(0));
    if (filename.isEmpty()) {
        return;
    }
    QFile file(filename);
    file.open(QIODevice::Truncate | QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream << KeeShareSettings::Own::serialize(m_own);
    stream.flush();
    file.close();
    config()->set("KeeShare/LastKeyDir", QFileInfo(filename).absolutePath());
}

void SettingsWidgetKeeShare::trustSelectedCertificates()
{
    const auto* selectionModel = m_ui->importedCertificateTableView->selectionModel();
    Q_ASSERT(selectionModel);
    for (const auto& index : selectionModel->selectedRows()) {
        m_foreign.certificates[index.row()].trust = KeeShareSettings::Trust::Trusted;
    }

    updateForeignCertificates();
}

void SettingsWidgetKeeShare::askSelectedCertificates()
{
    const auto* selectionModel = m_ui->importedCertificateTableView->selectionModel();
    Q_ASSERT(selectionModel);
    for (const auto& index : selectionModel->selectedRows()) {
        m_foreign.certificates[index.row()].trust = KeeShareSettings::Trust::Ask;
    }

    updateForeignCertificates();
}

void SettingsWidgetKeeShare::untrustSelectedCertificates()
{
    const auto* selectionModel = m_ui->importedCertificateTableView->selectionModel();
    Q_ASSERT(selectionModel);
    for (const auto& index : selectionModel->selectedRows()) {
        m_foreign.certificates[index.row()].trust = KeeShareSettings::Trust::Untrusted;
    }

    updateForeignCertificates();
}

void SettingsWidgetKeeShare::removeSelectedCertificates()
{
    auto certificates = m_foreign.certificates;
    const auto* selectionModel = m_ui->importedCertificateTableView->selectionModel();
    Q_ASSERT(selectionModel);
    for (const auto& index : selectionModel->selectedRows()) {
        certificates.removeOne(m_foreign.certificates[index.row()]);
    }
    m_foreign.certificates = certificates;

    updateForeignCertificates();
}

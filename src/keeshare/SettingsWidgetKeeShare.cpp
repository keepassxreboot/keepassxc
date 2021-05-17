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

#include "config-keepassx.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "keeshare/KeeShare.h"

#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTextStream>

SettingsWidgetKeeShare::SettingsWidgetKeeShare(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidgetKeeShare())
{
    m_ui->setupUi(this);

    connect(m_ui->ownCertificateSignerEdit, SIGNAL(textChanged(QString)), SLOT(setVerificationExporter(QString)));
    connect(m_ui->generateOwnCerticateButton, SIGNAL(clicked(bool)), SLOT(generateCertificate()));
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
}

void SettingsWidgetKeeShare::updateOwnCertificate()
{
    m_ui->ownCertificateSignerEdit->setText(m_own.certificate.signer);
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
    KeeShare::setActive(active);

    config()->set(Config::KeeShare_QuietSuccess, m_ui->quietSuccessCheckBox->isChecked());
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
    m_ui->ownCertificateFingerprintEdit->setText(m_own.certificate.fingerprint());
}

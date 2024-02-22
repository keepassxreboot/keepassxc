/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "TotpSetupDialog.h"
#include "ui_TotpSetupDialog.h"

#include "core/Base32.h"
#include "core/Totp.h"
#include "gui/MessageBox.h"

TotpSetupDialog::TotpSetupDialog(QWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpSetupDialog())
    , m_entry(entry)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedSize(sizeHint());

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(m_ui->radioCustom, SIGNAL(toggled(bool)), SLOT(toggleCustom(bool)));

    init();
}

TotpSetupDialog::~TotpSetupDialog() = default;

void TotpSetupDialog::saveSettings()
{
    // Secret key sanity check
    // Convert user input to all uppercase and remove '='
    auto key = m_ui->seedEdit->text().toUpper().remove(" ").remove("=").trimmed().toLatin1();
    auto sanitizedKey = Base32::sanitizeInput(key);
    // Use startsWith to ignore added '=' for padding at the end
    if (!sanitizedKey.startsWith(key)) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid secret key. The key must be in Base32 format.\n"
                                   "Example: JBSWY3DPEHPK3PXP"));
        return;
    }

    QString encShortName;
    uint digits = Totp::DEFAULT_DIGITS;
    uint step = Totp::DEFAULT_STEP;
    Totp::Algorithm algorithm = Totp::DEFAULT_ALGORITHM;
    Totp::StorageFormat format = Totp::DEFAULT_FORMAT;

    if (m_ui->radioSteam->isChecked()) {
        digits = Totp::STEAM_DIGITS;
        encShortName = Totp::STEAM_SHORTNAME;
    } else if (m_ui->radioCustom->isChecked()) {
        algorithm = static_cast<Totp::Algorithm>(m_ui->algorithmComboBox->currentData().toInt());
        step = m_ui->stepSpinBox->value();
        digits = m_ui->digitsSpinBox->value();
    }

    auto settings = m_entry->totpSettings();
    if (settings) {
        if (key.isEmpty()) {
            auto answer = MessageBox::question(this,
                                               tr("Confirm Remove TOTP Settings"),
                                               tr("Are you sure you want to delete TOTP settings for this entry?"),
                                               MessageBox::Delete | MessageBox::Cancel);
            if (answer != MessageBox::Delete) {
                return;
            }
        }

        format = settings->format;
        if (format == Totp::StorageFormat::LEGACY && m_ui->radioCustom->isChecked()) {
            // Implicitly upgrade to the OTPURL format to allow for custom settings
            format = Totp::DEFAULT_FORMAT;
        }
    }

    m_entry->setTotp(Totp::createSettings(key, digits, step, format, encShortName, algorithm));
    emit totpUpdated();
    close();
}

void TotpSetupDialog::toggleCustom(bool status)
{
    m_ui->customSettingsGroup->setEnabled(status);
}

void TotpSetupDialog::init()
{
    // Add algorithm choices
    auto algorithms = Totp::supportedAlgorithms();
    for (const auto& item : algorithms) {
        m_ui->algorithmComboBox->addItem(item.first, item.second);
    }
    m_ui->algorithmComboBox->setCurrentIndex(0);

    // Read entry totp settings
    auto settings = m_entry->totpSettings();
    if (settings) {
        auto key = settings->key;
        m_ui->seedEdit->setText(key.remove("="));
        m_ui->seedEdit->setCursorPosition(0);
        m_ui->stepSpinBox->setValue(settings->step);

        if (settings->encoder.shortName == Totp::STEAM_SHORTNAME) {
            m_ui->radioSteam->setChecked(true);
        } else if (settings->custom) {
            m_ui->radioCustom->setChecked(true);
            m_ui->digitsSpinBox->setValue(settings->digits);
            int index = m_ui->algorithmComboBox->findData(settings->algorithm);
            if (index != -1) {
                m_ui->algorithmComboBox->setCurrentIndex(index);
            }
        }
    }
}

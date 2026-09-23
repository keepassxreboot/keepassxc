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

#include <QUrlQuery>

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
    connect(m_ui->radioUri, SIGNAL(toggled(bool)), SLOT(toggleUri(bool)));

    init();
}

TotpSetupDialog::~TotpSetupDialog() = default;

void TotpSetupDialog::init()
{
    // Add algorithm choices
    auto algorithms = Totp::supportedAlgorithms();
    for (const auto& item : algorithms) {
        m_ui->algorithmComboBox->addItem(item.first, item.second);
    }
    m_ui->algorithmComboBox->setCurrentIndex(0);
    m_ui->invalidKeyLabel->setVisible(false);

    // Read entry totp settings
    auto settings = m_entry->totpSettings();
    if (settings) {
        auto key = settings->key;
        m_ui->seedEdit->setText(key.remove("="));
        m_ui->seedEdit->setCursorPosition(0);
        m_ui->stepSpinBox->setValue(settings->step);

        if (settings->encoder.shortName == Totp::STEAM_SHORTNAME) {
            m_ui->radioSteam->setChecked(true);
        } else if (Totp::hasCustomSettings(settings)) {
            m_ui->radioCustom->setChecked(true);
            m_ui->digitsSpinBox->setValue(settings->digits);
            int index = m_ui->algorithmComboBox->findData(settings->algorithm);
            if (index != -1) {
                m_ui->algorithmComboBox->setCurrentIndex(index);
            }
        }

        auto error = Totp::checkValidSettings(settings);
        m_ui->invalidKeyLabel->setVisible(!error.isEmpty());
    }
}

void TotpSetupDialog::saveSettings()
{
    QSharedPointer<Totp::Settings> newSettings;
    if (m_ui->radioDefault->isChecked()) {
        newSettings = createFromRfc6238();
    } else if (m_ui->radioUri->isChecked()) {
        newSettings = createFromUri();
    } else if (m_ui->radioSteam->isChecked()) {
        newSettings = createFromSteam();
    } else if (m_ui->radioCustom->isChecked()) {
        newSettings = createFromCustom();
    }

    if (newSettings.isNull()) {
        return;
    }

    auto settings = m_entry->totpSettings();
    if (settings) {
        if (newSettings->key.isEmpty()) {
            auto answer = MessageBox::question(this,
                                               tr("Confirm Remove TOTP Settings"),
                                               tr("Are you sure you want to delete TOTP settings for this entry?"),
                                               MessageBox::Delete | MessageBox::Cancel);
            if (answer != MessageBox::Delete) {
                return;
            }
        }
    }

    m_entry->setTotp(newSettings);
    emit totpUpdated();
    close();
}

void TotpSetupDialog::toggleCustom(bool status)
{
    m_ui->customSettingsGroup->setEnabled(status);
}

void TotpSetupDialog::toggleUri(bool status)
{
    if (status) {
        m_ui->labelSecretKey->setText(tr("URI:"));
    } else {
        m_ui->labelSecretKey->setText(tr("Secret Key:"));
    }
}

QSharedPointer<Totp::Settings> TotpSetupDialog::createFromRfc6238()
{
    QString key = sanitizeSecretKey();
    if (key == QStringLiteral("err")) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid secret key. The key must be in Base32 format.\n"
                                   "Example: JBSWY3DPEHPK3PXP"));
        return nullptr;
    }

    QString encShortName;
    uint digits = Totp::DEFAULT_DIGITS;
    uint step = Totp::DEFAULT_STEP;
    Totp::Algorithm algorithm = Totp::DEFAULT_ALGORITHM;
    Totp::StorageFormat format = Totp::DEFAULT_FORMAT;

    auto settings = m_entry->totpSettings();
    if (settings) {
        format = settings->format;
    }

    return Totp::createSettings(key, digits, step, format, encShortName, algorithm);
}

QSharedPointer<Totp::Settings> TotpSetupDialog::createFromUri()
{
    auto uri = QUrl(m_ui->seedEdit->text());
    if (!uri.isValid() || uri.scheme() != "otpauth") {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid TOTP URI. The URI must start with otpauth://totp/"));
        return nullptr;
    }

    QString encShortName;
    uint digits = Totp::DEFAULT_DIGITS;
    uint step = Totp::DEFAULT_STEP;
    Totp::Algorithm algorithm = Totp::DEFAULT_ALGORITHM;
    Totp::StorageFormat format = Totp::DEFAULT_FORMAT;

    QUrlQuery query(uri);

    if (!query.hasQueryItem("secret")) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid TOTP URI. The URI must start with otpauth://totp/"));
        return nullptr;
    }
    QString key = sanitizeSecretKey(query.queryItemValue("secret"));
    if (key == QStringLiteral("err")) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid TOTP URI. The URI must start with otpauth://totp/"));
        return nullptr;
    }

    if (query.hasQueryItem("digits")) {
        digits = query.queryItemValue("digits").toUInt();
    }
    if (query.hasQueryItem("period")) {
        step = query.queryItemValue("period").toUInt();
    }
    if (query.hasQueryItem("algorithm")) {
        algorithm = Totp::getHashTypeByName(query.queryItemValue("algorithm"));
    }

    auto settings = m_entry->totpSettings();
    if (settings) {
        format = settings->format;
    }

    return Totp::createSettings(key, digits, step, format, encShortName, algorithm);
}

QSharedPointer<Totp::Settings> TotpSetupDialog::createFromSteam()
{
    QString key = sanitizeSecretKey();
    if (key == QStringLiteral("err")) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid secret key. The key must be in Base32 format.\n"
                                   "Example: JBSWY3DPEHPK3PXP"));
        return nullptr;
    }

    QString encShortName = Totp::STEAM_SHORTNAME;
    uint digits = Totp::STEAM_DIGITS;
    uint step = Totp::DEFAULT_STEP;
    Totp::Algorithm algorithm = Totp::DEFAULT_ALGORITHM;
    Totp::StorageFormat format = Totp::DEFAULT_FORMAT;

    auto settings = m_entry->totpSettings();
    if (settings) {
        format = settings->format;
    }

    return Totp::createSettings(key, digits, step, format, encShortName, algorithm);
}

QSharedPointer<Totp::Settings> TotpSetupDialog::createFromCustom()
{
    QString key = sanitizeSecretKey();
    if (key == QStringLiteral("err")) {
        MessageBox::information(this,
                                tr("Invalid TOTP Secret"),
                                tr("You have entered an invalid secret key. The key must be in Base32 format.\n"
                                   "Example: JBSWY3DPEHPK3PXP"));
        return nullptr;
    }

    QString encShortName;
    uint digits = m_ui->digitsSpinBox->value();
    uint step = m_ui->stepSpinBox->value();
    Totp::Algorithm algorithm = static_cast<Totp::Algorithm>(m_ui->algorithmComboBox->currentData().toInt());
    Totp::StorageFormat format = Totp::DEFAULT_FORMAT;

    auto settings = m_entry->totpSettings();
    if (settings) {
        format = settings->format;
        if (format == Totp::StorageFormat::LEGACY) {
            // Implicitly upgrade to the OTPURL format to allow for custom settings
            format = Totp::DEFAULT_FORMAT;
        }
    }

    return Totp::createSettings(key, digits, step, format, encShortName, algorithm);
}

QString TotpSetupDialog::sanitizeSecretKey()
{
    return sanitizeSecretKey(m_ui->seedEdit->text());
}

QString TotpSetupDialog::sanitizeSecretKey(const QString& key)
{
    // Secret key sanity check
    // Convert user input to all uppercase and remove '='
    auto keyCleaned = key.toUpper().remove(" ").remove("=").trimmed();
    auto keyBytes = keyCleaned.toLatin1();
    auto sanitizedKey = Base32::sanitizeInput(keyBytes);
    // Use startsWith to ignore added '=' for padding at the end
    if (!sanitizedKey.startsWith(keyBytes)) {
        return QStringLiteral("err");
    }
    return sanitizedKey;
}

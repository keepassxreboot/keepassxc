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
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/QrDecoder.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImageReader>
#include <QKeyEvent>
#include <QMimeData>
#include <QShortcut>
#include <QSignalBlocker>

TotpSetupDialog::TotpSetupDialog(QWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpSetupDialog())
    , m_entry(entry)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // Make the QR code area a square as wide as the buttons below it, and as tall as needed
    // to line up the buttons with the bottom of the settings column
    const auto buttonsSize = m_ui->qrButtonsLayout->sizeHint();
    const auto side =
        qMax(buttonsSize.width(),
             m_ui->settingsLayout->sizeHint().height() - buttonsSize.height() - m_ui->qrLayout->spacing());
    m_ui->qrDropArea->setFixedSize(side, side);
    setFixedSize(sizeHint());

    // Allow pasting an image of a QR code while the otpauth field has focus,
    // and anywhere else in the dialog where Ctrl+V is not used for text
    m_ui->otpauthEdit->installEventFilter(this);
    new QShortcut(QKeySequence::Paste, this, this, &TotpSetupDialog::pasteQrImage);

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(m_ui->radioCustom, SIGNAL(toggled(bool)), SLOT(toggleCustom(bool)));
    connect(m_ui->otpauthEdit, &QLineEdit::textChanged, this, &TotpSetupDialog::applyOtpauthUri);
    connect(m_ui->qrDropArea, &QrDropArea::imageDropped, this, &TotpSetupDialog::decodeQrImage);
    connect(m_ui->openImageButton, &QPushButton::clicked, this, &TotpSetupDialog::openQrImage);
    connect(m_ui->pasteImageButton, &QPushButton::clicked, this, &TotpSetupDialog::pasteQrImage);

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
    m_ui->invalidKeyLabel->setVisible(false);

    // Read entry totp settings
    auto settings = m_entry->totpSettings();
    if (settings) {
        loadSettings(settings);

        auto error = Totp::checkValidSettings(settings);
        m_ui->invalidKeyLabel->setVisible(!error.isEmpty());
    }
}

void TotpSetupDialog::loadSettings(const QSharedPointer<Totp::Settings>& settings)
{
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
    } else {
        m_ui->radioDefault->setChecked(true);
    }
}

void TotpSetupDialog::showError(const QString& message)
{
    m_ui->invalidKeyLabel->setText(message);
    m_ui->invalidKeyLabel->setVisible(!message.isEmpty());
}

void TotpSetupDialog::applyOtpauthUri(const QString& uri)
{
    const auto text = uri.trimmed();
    if (text.isEmpty()) {
        showError({});
        return;
    }

    QSharedPointer<Totp::Settings> settings;
    if (text.startsWith("otpauth://", Qt::CaseInsensitive)) {
        settings = Totp::parseSettings(text);
    }

    if (!settings || settings->key.isEmpty() || !Totp::checkValidSettings(settings).isEmpty()) {
        showError(tr("Error: invalid otpauth URI"));
        return;
    }

    showError({});
    loadSettings(settings);
}

void TotpSetupDialog::decodeQrImage(const QImage& image)
{
    m_ui->qrDropArea->setImage(image);

    const auto text = QrDecoder::decode(image);
    if (text.isEmpty()) {
        showError(tr("Error: no QR code found in the image"));
        return;
    }

    if (!text.startsWith("otpauth://", Qt::CaseInsensitive)) {
        showError(tr("Error: the QR code does not contain a TOTP setup URI"));
        return;
    }

    // Apply explicitly so the settings are reloaded even if the URI did not change
    {
        QSignalBlocker blocker(m_ui->otpauthEdit);
        m_ui->otpauthEdit->setText(text);
    }
    applyOtpauthUri(text);
}

void TotpSetupDialog::openQrImage()
{
    QStringList formats;
    for (const auto& format : QImageReader::supportedImageFormats()) {
        formats << QString("*.%1").arg(QString::fromLatin1(format));
    }
    const auto filters = QString("%1 (%2);;%3 (*)").arg(tr("Images"), formats.join(' '), tr("All files"));

    const auto fileName =
        fileDialog()->getOpenFileName(this, tr("Open QR code image"), FileDialog::getLastDir("totpqrcode"), filters);
    if (fileName.isEmpty()) {
        return;
    }
    FileDialog::saveLastDir("totpqrcode", fileName, true);

    QImage image(fileName);
    if (image.isNull()) {
        showError(tr("Error: unable to open the image"));
        return;
    }

    decodeQrImage(image);
}

bool TotpSetupDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->otpauthEdit && event->type() == QEvent::KeyPress
        && static_cast<QKeyEvent*>(event)->matches(QKeySequence::Paste)) {
        // Text is pasted by the line edit itself, images are decoded as QR codes
        const auto* mimeData = QGuiApplication::clipboard()->mimeData();
        if (mimeData && !mimeData->hasText() && (mimeData->hasImage() || mimeData->hasUrls())) {
            pasteQrImage();
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void TotpSetupDialog::pasteQrImage()
{
    const auto* mimeData = QGuiApplication::clipboard()->mimeData();
    if (!mimeData) {
        return;
    }

    // An otpauth URI copied as text is applied directly
    if (mimeData->hasText() && !mimeData->hasImage()) {
        const auto text = mimeData->text().trimmed();
        if (text.startsWith("otpauth://", Qt::CaseInsensitive)) {
            m_ui->otpauthEdit->setText(text);
            return;
        }
    }

    const auto image = QrDropArea::imageFromMimeData(mimeData);
    if (image.isNull()) {
        showError(tr("Error: the clipboard does not contain an image"));
        return;
    }

    decodeQrImage(image);
}

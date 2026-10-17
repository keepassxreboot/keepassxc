/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "TotpExportSettingsDialog.h"

#include "core/Totp.h"
#include "gui/Clipboard.h"
#include "gui/MainWindow.h"
#include "gui/SquareSvgWidget.h"
#include "qrcode/QrCode.h"

#include <QAccessible>
#include <QBoxLayout>
#include <QBuffer>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>

TotpExportSettingsDialog::TotpExportSettingsDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_timer(new QTimer(this))
    , m_verticalLayout(new QVBoxLayout())
    , m_totpSvgContainerWidget(new QStackedWidget())
    , m_totpSvgWidget(new SquareSvgWidget(m_totpSvgContainerWidget))
    , m_countDown(new QLabel())
    , m_warningLabel(new QLabel())
    , m_accessibilityText(new QLineEdit())
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Ok))
{
    setObjectName("entryQrCodeWidget");
    m_totpSvgContainerWidget->addWidget(m_totpSvgWidget);
    m_totpSvgWidget->setAccessibleName(tr("TOTP QR code"));
    m_totpSvgWidget->setFocusPolicy(Qt::StrongFocus);

    m_verticalLayout->addWidget(m_warningLabel);
    m_verticalLayout->addItem(new QSpacerItem(0, 0));
    m_verticalLayout->addWidget(m_totpSvgContainerWidget);
    m_accessibilityText->setReadOnly(true);
    m_accessibilityText->setFrame(false);
    m_accessibilityText->setAccessibleName(tr("TOTP QR code instructions"));
    m_accessibilityText->setText(tr("TOTP QR code. Scan this QR code with an authenticator app to configure TOTP."));
    m_verticalLayout->addWidget(m_accessibilityText);
    m_verticalLayout->addWidget(m_countDown);
    m_verticalLayout->addWidget(m_buttonBox);

    m_verticalLayout->setAlignment(m_buttonBox, Qt::AlignBottom);

    setLayout(m_verticalLayout);
    setAttribute(Qt::WA_DeleteOnClose);

    connect(m_buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
    connect(m_timer, SIGNAL(timeout()), SLOT(autoClose()));

    new QShortcut(QKeySequence(QKeySequence::Copy), this, SLOT(copyToClipboard()));

    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));
    m_buttonBox->setFocus();
    m_countDown->setAlignment(Qt::AlignCenter);
    setTabOrder(m_totpSvgWidget, m_accessibilityText);
    setTabOrder(m_accessibilityText, m_buttonBox->button(QDialogButtonBox::Cancel));
    setTabOrder(m_buttonBox->button(QDialogButtonBox::Cancel), m_buttonBox->button(QDialogButtonBox::Ok));

    m_secTillClose = 45;
    autoClose();
    m_timer->start(1000);

    const auto totpSettings = entry->totpSettings();
    if (Totp::hasCustomSettings(totpSettings) || !totpSettings->encoder.shortName.isEmpty()) {
        m_warningLabel->setWordWrap(true);
        m_warningLabel->setMargin(5);
        m_warningLabel->setText(tr("NOTE: These TOTP settings are custom and may not work with other authenticators.",
                                   "TOTP QR code dialog warning"));
    } else {
        m_warningLabel->hide();
    }

    m_totpUri = Totp::writeSettings(entry->totpSettings(), entry->title(), entry->username(), true);
    const QrCode qrc(m_totpUri);

    if (qrc.isValid()) {
        QBuffer buffer;
        qrc.writeSvg(&buffer, logicalDpiX());
        m_totpSvgWidget->load(buffer.data());
        m_totpSvgWidget->setAccessibleDescription(
            tr("Scan this QR code with an authenticator app to configure TOTP."));
        m_totpSvgWidget->setFocus(Qt::OtherFocusReason);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QAccessibleAnnouncementEvent announcementEvent(m_totpSvgWidget, tr("TOTP QR code"));
        QAccessible::updateAccessibility(&announcementEvent);
#endif
        const auto minsize = static_cast<int>(logicalDpiX() * 2.5);
        m_totpSvgWidget->setMinimumSize(minsize, minsize);
    } else {
        auto errorBox = new QMessageBox(parent);
        errorBox->setAttribute(Qt::WA_DeleteOnClose);
        errorBox->setIcon(QMessageBox::Warning);
        errorBox->setText(tr("There was an error creating the QR code."));
        errorBox->exec();
        close();
    }
}

void TotpExportSettingsDialog::copyToClipboard()
{
    clipboard()->setText(m_totpUri);
    if (config()->get(Config::HideWindowOnCopy).toBool()) {
        if (config()->get(Config::MinimizeOnCopy).toBool()) {
            getMainWindow()->minimizeOrHide();
        } else if (config()->get(Config::DropToBackgroundOnCopy).toBool()) {
            getMainWindow()->lower();
            window()->lower();
        }
    }
}

void TotpExportSettingsDialog::autoClose()
{
    if (--m_secTillClose > 0) {
        const auto countdownText = tr("Closing in %1 seconds.").arg(m_secTillClose);
        m_countDown->setText(countdownText);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        // The dialog closes automatically. Announce the final countdown milestones
        // so screen-reader users are not left unaware that the window is about to
        // disappear while focus remains on another control.
        if (m_secTillClose <= 5 || m_secTillClose == 10) {
            QAccessibleAnnouncementEvent announcementEvent(m_countDown, countdownText);
            announcementEvent.setPoliteness(QAccessible::AnnouncementPoliteness::Assertive);
            QAccessible::updateAccessibility(&announcementEvent);
        }
#endif
    } else {
        m_timer->stop();
        close();
    }
}

TotpExportSettingsDialog::~TotpExportSettingsDialog() = default;

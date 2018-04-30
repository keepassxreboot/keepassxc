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
#include "core/Config.h"
#include "core/Entry.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseWidget.h"
#include "gui/SquareSvgWidget.h"
#include "qrcode/QrCode.h"
#include "totp/totp.h"

#include <QBuffer>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

TotpExportSettingsDialog::TotpExportSettingsDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_entry(entry)
    , m_timer(new QTimer(this))
    , m_verticalLayout(new QVBoxLayout())
    , m_totpSvgWidget(new SquareSvgWidget())
    , m_countDown(new QLabel())
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Ok))
{
    m_verticalLayout->addItem(new QSpacerItem(0, 0));

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addItem(new QSpacerItem(0, 0));
    horizontalLayout->addWidget(m_totpSvgWidget);
    horizontalLayout->addItem(new QSpacerItem(0, 0));

    m_verticalLayout->addItem(horizontalLayout);
    m_verticalLayout->addItem(new QSpacerItem(0, 0));

    m_verticalLayout->addWidget(m_countDown);
    m_verticalLayout->addWidget(m_buttonBox);

    setLayout(m_verticalLayout);
    setAttribute(Qt::WA_DeleteOnClose);

    connect(m_buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(autoClose()));

    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));
    m_countDown->setAlignment(Qt::AlignCenter);

    m_secTillClose = 45;
    autoClose();
    m_timer->start(1000);

    const QUrl keyUri = totpKeyUri(entry);
    const QrCode qrc(keyUri.toEncoded());

    if (qrc.isValid()) {
        QBuffer buffer;
        qrc.writeSvg(&buffer, this->logicalDpiX());
        m_totpSvgWidget->load(buffer.data());

        const int threeInches = this->logicalDpiX() * 3;
        const int fourInches = this->logicalDpiX() * 4;
        const QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        m_totpSvgWidget->resize(QSize(threeInches, threeInches));
        m_totpSvgWidget->setSizePolicy(sizePolicy);
        setMinimumSize(QSize(threeInches, fourInches));
        setSizePolicy(sizePolicy);
    } else {
        auto errorBox = new QMessageBox(parent);
        errorBox->setAttribute(Qt::WA_DeleteOnClose);
        errorBox->setIcon(QMessageBox::Warning);
        errorBox->setText(tr("There was an error creating the QR code."));
        errorBox->exec();
        close();
    }

    show();
}

void TotpExportSettingsDialog::copyToClipboard()
{
    const QUrl keyUri = totpKeyUri(m_entry);
    clipboard()->setText(keyUri.toEncoded().data());
    if (config()->get("MinimizeOnCopy").toBool()) {
        static_cast<DatabaseWidget*>(parent())->window()->showMinimized();
    }
}

void TotpExportSettingsDialog::autoClose()
{
    if (--m_secTillClose > 0) {
        m_countDown->setText(tr("Closing in %1 seconds.").arg(m_secTillClose));
    } else {
        m_timer->stop();
        close();
    }
}

QUrl TotpExportSettingsDialog::totpKeyUri(const Entry* entry) const
{
    return Totp::writeSettings(entry->totpSettings(), true);
}

TotpExportSettingsDialog::~TotpExportSettingsDialog() = default;

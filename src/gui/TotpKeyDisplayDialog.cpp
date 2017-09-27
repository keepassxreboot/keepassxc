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

#include "TotpKeyDisplayDialog.h"
#include "ui_TotpKeyDisplayDialog.h"
#include "core/Config.h"
#include "core/Entry.h"
#include "core/QRCode.h"
#include "totp/totp.h"
#include "gui/DatabaseWidget.h"
#include "gui/Clipboard.h"
#include <QPushButton>

TotpKeyDisplayDialog::TotpKeyDisplayDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpKeyDisplayDialog())
{
    m_entry = entry;
    m_parent = parent;

    m_ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    const QUrl keyUri = totpKeyUri(entry);
    const QRCode qrcode(keyUri.toEncoded());
    m_ui->totpKeyLabel->setPixmap(QPixmap::fromImage(qrcode.toQImage(200, 4)));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
}

void TotpKeyDisplayDialog::copyToClipboard()
{
    const QUrl keyUri = totpKeyUri(m_entry);
    clipboard()->setText(keyUri.toEncoded().data());
    if (config()->get("MinimizeOnCopy").toBool()) {
        m_parent->window()->showMinimized();
    }
}

QUrl TotpKeyDisplayDialog::totpKeyUri(const Entry* entry) const
{
    return QTotp::generateOtpString(
        entry->totpSeed(),
        QString("totp"),
        QString(entry->title()),
        QString(entry->username()),
        QString("SHA1"),
        entry->totpDigits(),
        entry->totpStep()
    );
}

TotpKeyDisplayDialog::~TotpKeyDisplayDialog()
{
}

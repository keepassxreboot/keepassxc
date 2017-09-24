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

#include "TotpSeedDialog.h"
#include "ui_TotpSeedDialog.h"

#include "core/Config.h"
#include "core/Entry.h"
#include "gui/DatabaseWidget.h"
#include "gui/Clipboard.h"

#include "qqrencode.h"

#include <QPushButton>


TotpSeedDialog::TotpSeedDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpSeedDialog())
{
    m_entry = entry;
    m_parent = parent;

    m_ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    QQREncode encoder;
    encoder.setMargin(4);
    encoder.setVersion(20);

    /* TODO Implement using the _Key Uri Format_ so that 
       the key can be read from authenticator apps correctly.
       See: https://github.com/google/google-authenticator/wiki/Key-Uri-Format
    */

    const QString keyUri = m_entry->totpSeed();

    if(encoder.encode(keyUri)) {
        const QImage qrcode = encoder.toQImage();
        m_ui->totpSeedLabel->setPixmap(QPixmap::fromImage(qrcode.scaled(200,200)));
    } else {
        m_ui->totpSeedLabel->setText(QString("Couldn't generate the QR code."));
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
}

void TotpSeedDialog::copyToClipboard()
{
    clipboard()->setText(m_entry->totpSeed());
    if (config()->get("MinimizeOnCopy").toBool()) {
        m_parent->window()->showMinimized();
    }
}

TotpSeedDialog::~TotpSeedDialog()
{
}

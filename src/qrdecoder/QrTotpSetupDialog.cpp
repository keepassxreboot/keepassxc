/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QrTotpSetupDialog.h"

#include "QrTotpWidget.h"
#include "gui/TotpSetupDialog.h"

#include <QTabWidget>
#include <QVBoxLayout>

namespace QrDecoder
{
    QrTotpSetupDialog::QrTotpSetupDialog(QWidget* parent, Entry* entry)
        : QDialog(parent)
        , m_tabWidget(new QTabWidget(this))
        , m_totpSetupDialog(new TotpSetupDialog(this, entry))
        , m_qrTotpWidget(new QrTotpWidget(this))
    {
        Q_ASSERT(entry);
        auto layout = new QVBoxLayout(this);
        layout->addWidget(m_tabWidget);

        m_totpSetupDialog->setWindowFlags(Qt::Widget);

        m_tabWidget->addTab(m_totpSetupDialog, tr("Manual"));
        m_tabWidget->addTab(m_qrTotpWidget, tr("QR Code"));

        connect(
            m_qrTotpWidget, &QrTotpWidget::settingsReady, this, [this](const QSharedPointer<Totp::Settings>& settings) {
                if (!settings) {
                    return;
                }

                m_totpSetupDialog->setSettings(*settings);
                m_tabWidget->setCurrentWidget(m_totpSetupDialog);
            });

        connect(m_totpSetupDialog, &TotpSetupDialog::totpUpdated, this, &QrTotpSetupDialog::totpUpdated);

        connect(m_totpSetupDialog, &QDialog::accepted, this, &QDialog::accept);

        connect(m_totpSetupDialog, &QDialog::rejected, this, &QDialog::reject);
    }
} // namespace QrDecoder

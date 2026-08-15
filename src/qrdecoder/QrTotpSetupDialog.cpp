/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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


#include "QrTotpSetupDialog.h"

#include "gui/TotpSetupDialog.h"
#include "core/Totp.h"
#include "QrTotpWidget.h"

#include <QLineEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
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
        auto layout = new QVBoxLayout(this);
        layout->addWidget(m_tabWidget);

        m_totpSetupDialog->setWindowFlags(Qt::Widget);

        m_tabWidget->addTab(m_totpSetupDialog, tr("Manual"));
        m_tabWidget->addTab(m_qrTotpWidget, tr("QR Code"));

connect(m_qrTotpWidget,
        &QrTotpWidget::settingsReady,
        this,
        [this](const QSharedPointer<Totp::Settings>& settings) {
            if (!settings) {
                return;
            }

            auto seedEdit =
                m_totpSetupDialog->findChild<QLineEdit*>(QStringLiteral("seedEdit"));
            auto radioCustom =
                m_totpSetupDialog->findChild<QRadioButton*>(QStringLiteral("radioCustom"));
            auto customSettingsGroup =
                m_totpSetupDialog->findChild<QGroupBox*>(QStringLiteral("customSettingsGroup"));
            auto algorithmComboBox =
                m_totpSetupDialog->findChild<QComboBox*>(QStringLiteral("algorithmComboBox"));
            auto stepSpinBox =
                m_totpSetupDialog->findChild<QSpinBox*>(QStringLiteral("stepSpinBox"));
            auto digitsSpinBox =
                m_totpSetupDialog->findChild<QSpinBox*>(QStringLiteral("digitsSpinBox"));

            if (!seedEdit || !radioCustom || !customSettingsGroup || !algorithmComboBox
                || !stepSpinBox || !digitsSpinBox) {
                return;
            }

            // Secret
            seedEdit->setText(settings->key);

            // Parsed otpauth:// parameters
            radioCustom->setChecked(true);
            customSettingsGroup->setEnabled(true);

            const auto algorithmIndex = algorithmComboBox->findData(settings->algorithm);
            if (algorithmIndex >= 0) {
                algorithmComboBox->setCurrentIndex(algorithmIndex);
            }

            stepSpinBox->setValue(settings->step);
            digitsSpinBox->setValue(settings->digits);

            // Go back to the normal TOTP setup page.
            m_tabWidget->setCurrentWidget(m_totpSetupDialog);
            seedEdit->setFocus();
        });

        connect(m_totpSetupDialog,
                &TotpSetupDialog::totpUpdated,
                this,
                &QrTotpSetupDialog::totpUpdated);

        connect(m_totpSetupDialog, &QDialog::accepted, this, &QDialog::accept);
        connect(m_totpSetupDialog, &QDialog::rejected, this, &QDialog::reject);
    }
} // namespace QrDecoder
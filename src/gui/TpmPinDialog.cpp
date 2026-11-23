/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "TpmPinDialog.h"

#include "keys/drivers/YubiKey.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <QVBoxLayout>

TpmPinDialog::TpmPinDialog(const QString& keyName, const QString& message, bool keptForQuickUnlock, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("TPM2 PIN/password"));
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    // Previous failure / lockout state reported by the TPM, shown as plain bold text
    m_message = new QLabel(message, this);
    m_message->setTextFormat(Qt::PlainText);
    m_message->setWordWrap(true);
    auto messageFont = m_message->font();
    messageFont.setBold(true);
    m_message->setFont(messageFont);
    m_message->setVisible(!message.isEmpty());
    layout->addWidget(m_message);

    m_label = new QLabel(tr("Enter the PIN/password for TPM2 key %1:").arg(keyName), this);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setWordWrap(true);
    layout->addWidget(m_label);

    m_edit = new QLineEdit(this);
    m_edit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_edit);

    if (keptForQuickUnlock) {
        // The application setting keeps the PIN with the Quick Unlock data; make that visible instead of
        // offering a choice that would have no effect
        m_remember = new QCheckBox(tr("Remembered for Quick Unlock until the database is closed"), this);
        m_remember->setChecked(true);
        m_remember->setEnabled(false);
    } else {
        m_remember = new QCheckBox(tr("Remember until the database is locked or closed"), this);
        m_remember->setChecked(false);
    }
    layout->addWidget(m_remember);

    if (keptForQuickUnlock) {
        // A greyed-out box does not say why it cannot be changed: explain it right below, and also for
        // assistive technology and as a tooltip
        const auto reason = tr("This option cannot be disabled because \"Remember the TPM2 PIN/password for Quick "
                               "Unlock\" is enabled in Application Settings > Security > Hardware Keys. Turn that "
                               "setting off to choose for each unlock.");
        auto* why = new QLabel(reason, this);
        why->setTextFormat(Qt::PlainText);
        why->setWordWrap(true);
        auto whyFont = why->font();
        whyFont.setItalic(true);
        why->setFont(whyFont);
        // Align the text with the text of the check box
        const int indent =
            style()->pixelMetric(QStyle::PM_IndicatorWidth) + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing);
        why->setContentsMargins(indent, 0, 0, 0);
        layout->addWidget(why);
        m_remember->setToolTip(reason);
        m_remember->setAccessibleDescription(reason);
        // Wide enough that the explanation does not wrap into a tall, narrow column
        setMinimumWidth(470);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    m_edit->setFocus();
}

TpmPinDialog::~TpmPinDialog()
{
    m_edit->clear();
}

Botan::secure_vector<char> TpmPinDialog::pin() const
{
    return YubiKey::pinFromString(m_edit->text());
}

bool TpmPinDialog::remember() const
{
    return m_remember->isChecked();
}

bool TpmPinDialog::getPin(
    QWidget* parent,
    const QString& keyName,
    const QString& message,
    Botan::secure_vector<char>& pin,
    bool& remember,
    bool keptForQuickUnlock)
{
    TpmPinDialog dlg(keyName, message, keptForQuickUnlock, parent);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    pin = dlg.pin();
    remember = dlg.remember();
    return !pin.empty();
}

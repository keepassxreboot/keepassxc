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

#include "PreviewEntryAttachmentsDialog.h"
#include "ui_PreviewEntryAttachmentsDialog.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QMimeDatabase>
#include <QPushButton>

PreviewEntryAttachmentsDialog::PreviewEntryAttachmentsDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::PreviewEntryAttachmentsDialog)
{
    m_ui->setupUi(this);

    // Disable the help button in the title bar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Initialize dialog buttons
    m_ui->dialogButtons->setStandardButtons(QDialogButtonBox::Close | QDialogButtonBox::Open | QDialogButtonBox::Save);
    auto closeButton = m_ui->dialogButtons->button(QDialogButtonBox::Close);
    closeButton->setDefault(true);

    auto saveButton = m_ui->dialogButtons->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Save…"));

    connect(m_ui->dialogButtons, &QDialogButtonBox::rejected, this, &PreviewEntryAttachmentsDialog::reject);
    connect(m_ui->dialogButtons, &QDialogButtonBox::clicked, [this](QAbstractButton* button) {
        auto pressedButton = m_ui->dialogButtons->standardButton(button);

        const auto attachment = m_ui->attachmentWidget->getAttachment();
        if (pressedButton == QDialogButtonBox::Open) {
            emit openAttachment(attachment.name);
        } else if (pressedButton == QDialogButtonBox::Save) {
            emit saveAttachment(attachment.name);
        }
    });
}

PreviewEntryAttachmentsDialog::~PreviewEntryAttachmentsDialog() = default;

void PreviewEntryAttachmentsDialog::setAttachment(attachments::Attachment attachment)
{
    setWindowTitle(tr("Preview: %1").arg(attachment.name));

    m_ui->attachmentWidget->openAttachment(std::move(attachment), attachments::OpenMode::ReadOnly);
}

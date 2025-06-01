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

#include "EditEntryAttachmentsDialog.h"
#include "ui_EditEntryAttachmentsDialog.h"

#include <core/EntryAttachments.h>

#include <QDebug>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPushButton>

EditEntryAttachmentsDialog::EditEntryAttachmentsDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::EditEntryAttachmentsDialog)
{
    m_ui->setupUi(this);

    m_ui->dialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_ui->dialogButtons, &QDialogButtonBox::accepted, this, &EditEntryAttachmentsDialog::accept);
    connect(m_ui->dialogButtons, &QDialogButtonBox::rejected, this, &EditEntryAttachmentsDialog::reject);
}

EditEntryAttachmentsDialog::~EditEntryAttachmentsDialog() = default;

void EditEntryAttachmentsDialog::setAttachment(attachments::Attachment attachment)
{
    setWindowTitle(tr("Edit: %1").arg(attachment.name));

    m_ui->attachmentWidget->openAttachment(std::move(attachment), attachments::OpenMode::ReadWrite);
}

attachments::Attachment EditEntryAttachmentsDialog::getAttachment() const
{
    return m_ui->attachmentWidget->getAttachment();
}

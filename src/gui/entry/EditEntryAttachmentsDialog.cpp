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
#include "core/Tools.h"
#include "ui_EditEntryAttachmentsDialog.h"

#include <core/EntryAttachments.h>

#include <QDebug>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPushButton>

EditEntryAttachmentsDialog::EditEntryAttachmentsDialog(
    std::unique_ptr<attachments::AttachmentsWidgetFactory> widgetsFactory,
    QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::EditEntryAttachmentsDialog)
    , m_widgetsFactory(std::move(widgetsFactory))
{
    Q_ASSERT(m_widgetsFactory);

    m_ui->setupUi(this);

    m_ui->dialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_ui->dialogButtons, &QDialogButtonBox::accepted, this, &EditEntryAttachmentsDialog::accept);
    connect(m_ui->dialogButtons, &QDialogButtonBox::rejected, this, &EditEntryAttachmentsDialog::reject);
}

EditEntryAttachmentsDialog::~EditEntryAttachmentsDialog() = default;

void EditEntryAttachmentsDialog::setAttachment(attachments::Attachment attachment)
{
    setWindowTitle(tr("Edit: %1").arg(attachment.name));

    if (auto widget = m_widgetsFactory->createAttachmentWidget(Tools::getMimeType(attachment.data), this)) {
        widget->openAttachment(std::move(attachment), attachments::OpenMode::ReadWrite);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (auto lastWidget = std::exchange(m_attachmentWidget, widget)) {
            m_ui->verticalLayout->removeWidget(lastWidget);
        }

        m_ui->verticalLayout->insertWidget(0, m_attachmentWidget);
    } else {
        qWarning() << tr("Unable to create attachment widget for file %1").arg(attachment.name);
    }
}

attachments::Attachment EditEntryAttachmentsDialog::getAttachment() const
{
    if (m_attachmentWidget) {
        return m_attachmentWidget->getAttachment();
    } else {
        qWarning() << tr("Attachment not found");
    }

    return {};
}

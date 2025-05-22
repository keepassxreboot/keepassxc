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

PreviewEntryAttachmentsDialog::PreviewEntryAttachmentsDialog(
    std::shared_ptr<attachments::IAttachmentWidgetFactory> widgetsFactory,
    QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::PreviewEntryAttachmentsDialog)
    , m_widgetFactory(std::move(widgetsFactory))
    , m_attachmentWidget(nullptr)
{
    Q_ASSERT(m_widgetFactory);

    m_ui->setupUi(this);

    // Disable the help button in the title bar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Initialize dialog buttons
    m_ui->dialogButtons->setStandardButtons(QDialogButtonBox::Close | QDialogButtonBox::Open | QDialogButtonBox::Save);
    auto closeButton = m_ui->dialogButtons->button(QDialogButtonBox::Close);
    closeButton->setDefault(true);

    connect(m_ui->dialogButtons, &QDialogButtonBox::rejected, this, &PreviewEntryAttachmentsDialog::reject);
    connect(m_ui->dialogButtons, &QDialogButtonBox::clicked, [this](QAbstractButton* button) {
        auto pressedButton = m_ui->dialogButtons->standardButton(button);
        if (!m_attachmentWidget) {
            qWarning() << tr("Attachment not found");
            return;
        }

        const auto attachment = m_attachmentWidget->getAttachment();
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

    if (auto attachWidget = m_widgetFactory->createAttachmentWidget(Tools::getMimeType(attachment.data), this)) {
        attachWidget->openAttachment(attachment, attachments::OpenMode::ReadOnly);
        attachWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (auto lastWidget = std::exchange(m_attachmentWidget, attachWidget)) {
            m_ui->verticalLayout->removeWidget(lastWidget);
        }

        // Set the new attachment widget
        m_ui->verticalLayout->insertWidget(0, m_attachmentWidget);
    } else {
        qWarning() << tr("Unable to create attachment widget for file %1").arg(attachment.name);
    }
}

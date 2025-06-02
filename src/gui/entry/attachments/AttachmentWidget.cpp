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

#include "AttachmentWidget.h"

#include "ImageAttachmentsWidget.h"
#include "TextAttachmentsWidget.h"

#include <core/Tools.h>

#include <QLabel>
#include <QVBoxLayout>

AttachmentWidget::AttachmentWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Attachment Viewer"));

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(0);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setAlignment(Qt::AlignCenter);
}

AttachmentWidget::~AttachmentWidget() = default;

void AttachmentWidget::openAttachment(attachments::Attachment attachment, attachments::OpenMode mode)
{
    m_attachment = std::move(attachment);
    m_mode = mode;

    updateUi();
}

void AttachmentWidget::updateUi()
{
    auto type = Tools::getMimeType(m_attachment.data);

    if (m_attachmentWidget) {
        layout()->removeWidget(m_attachmentWidget);
        m_attachmentWidget->deleteLater();
    }

    if (Tools::isTextMimeType(type)) {
        auto widget = new TextAttachmentsWidget(this);
        widget->openAttachment(m_attachment, m_mode);

        m_attachmentWidget = widget;
    } else if (type == Tools::MimeType::Image) {
        auto widget = new ImageAttachmentsWidget(this);
        widget->openAttachment(m_attachment, m_mode);

        m_attachmentWidget = widget;
    } else {
        auto label = new QLabel(tr("Unknown attachment type"), this);
        label->setAlignment(Qt::AlignCenter);

        m_attachmentWidget = label;
    }

    Q_ASSERT(m_attachmentWidget);
    m_attachmentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout()->addWidget(m_attachmentWidget);
}

attachments::Attachment AttachmentWidget::getAttachment() const
{
    // Text attachments can be edited at this time so pass this call forward
    if (auto textWidget = qobject_cast<TextAttachmentsWidget*>(m_attachmentWidget)) {
        return textWidget->getAttachment();
    }

    return m_attachment;
}

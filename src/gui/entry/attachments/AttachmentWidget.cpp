#include "AttachmentWidget.h"

#include "ImageAttachmentsWidget.h"
#include "TextAttachmentsWidget.h"
#include "ui_AttachmentWidget.h"

#include <core/Tools.h>

#include <QLabel>

namespace
{
    constexpr const char* UnknownAttachmentType = "Unknown attachment type";
}

AttachmentWidget::AttachmentWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AttachmentWidget())
{
    m_ui->setupUi(this);

    m_ui->verticalLayout->setAlignment(Qt::AlignCenter);
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
        m_ui->verticalLayout->removeWidget(m_attachmentWidget);
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
        auto label = new QLabel(this);
        label->setText(tr(UnknownAttachmentType));
        label->setAlignment(Qt::AlignCenter);

        m_attachmentWidget = label;
    }

    Q_ASSERT(m_attachmentWidget);
    m_attachmentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_ui->verticalLayout->insertWidget(0, m_attachmentWidget);
}

attachments::Attachment AttachmentWidget::getAttachment() const
{
    return m_attachment;
}

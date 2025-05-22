#include "UnknownAttachmentTypeWidget.h"

#include "ui_UnknownAttachmentTypeWidget.h"

UnknownAttachmentTypeWidget::UnknownAttachmentTypeWidget(QWidget* parent)
    : AbstractAttachmentWidget(parent)
    , m_ui(new Ui::UnknownAttachmentTypeWidget())
{
    m_ui->setupUi(this);
}

UnknownAttachmentTypeWidget ::~UnknownAttachmentTypeWidget() = default;

void UnknownAttachmentTypeWidget::openAttachment(attachments::Attachment attachment,
                                                 [[maybe_unused]] attachments::OpenMode mode)
{
    m_attachment = std::move(attachment);
}

attachments::Attachment UnknownAttachmentTypeWidget::getAttachment() const
{
    return m_attachment;
}

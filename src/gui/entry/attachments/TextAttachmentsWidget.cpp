#include "TextAttachmentsWidget.h"

#include "ui_TextAttachmentsWidget.h"

TextAttachmentsWidget::TextAttachmentsWidget(QWidget* parent)
    : AbstractAttachmentWidget(parent)
    , m_ui(new Ui::TextAttachmentsWidget())
    , m_mode(attachments::OpenMode::ReadOnly)
{
    m_ui->setupUi(this);

    updateWidget();
}

TextAttachmentsWidget::~TextAttachmentsWidget() = default;

void TextAttachmentsWidget::openAttachment(attachments::Attachment attachment, attachments::OpenMode mode)
{
    m_attachment = std::move(attachment);
    m_mode = mode;

    updateWidget();
}

attachments::Attachment TextAttachmentsWidget::getAttachment() const
{
    return {
        m_attachment.name,
        m_ui->attachmentTextEdit->toPlainText().toUtf8(),
    };
}

void TextAttachmentsWidget::updateWidget()
{
    m_ui->attachmentTextEdit->setReadOnly(m_mode == attachments::OpenMode::ReadOnly);
    m_ui->attachmentTextEdit->setPlainText(QString::fromUtf8(m_attachment.data));
}

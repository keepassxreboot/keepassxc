#include "TextAttachmentsEditWidget.h"
#include "ui_TextAttachmentsEditWidget.h"

#include <QPushButton>
#include <QTextEdit>

TextAttachmentsEditWidget::TextAttachmentsEditWidget(QWidget* parent)
    : attachments::AbstractAttachmentWidget(parent)
    , m_ui(new Ui::TextAttachmentsEditWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->attachmentsTextEdit, &QTextEdit::textChanged, this, &TextAttachmentsEditWidget::textChanged);
    connect(m_ui->previewPushButton, &QPushButton::clicked, this, &TextAttachmentsEditWidget::previewButtonClicked);
}

TextAttachmentsEditWidget::~TextAttachmentsEditWidget() = default;

void TextAttachmentsEditWidget::openAttachment(attachments::Attachment attachments, attachments::OpenMode mode)
{
    m_attachment = std::move(attachments);
    m_mode = mode;

    updateUi();
}

attachments::Attachment TextAttachmentsEditWidget::getAttachment() const
{
    return {.name = m_attachment.name, .data = m_ui->attachmentsTextEdit->toPlainText().toUtf8()};
}

void TextAttachmentsEditWidget::updateUi()
{
    m_ui->attachmentsTextEdit->setPlainText(m_attachment.data);
    m_ui->attachmentsTextEdit->setReadOnly(m_mode == attachments::OpenMode::ReadOnly);
}

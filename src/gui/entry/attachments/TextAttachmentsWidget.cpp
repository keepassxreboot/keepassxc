#include "TextAttachmentsWidget.h"
#include "TextAttachmentsPreviewWidget.h"

#include "ui_TextAttachmentsWidget.h"

#include <QSplitter>
#include <QTextEdit>

TextAttachmentsWidget::TextAttachmentsWidget(QWidget* parent)
    : AbstractAttachmentWidget(parent)
    , m_ui(new Ui::TextAttachmentsWidget())
    , m_mode(attachments::OpenMode::ReadOnly)
{
    m_ui->setupUi(this);

    initWidget();
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
    return m_attachment;
}

void TextAttachmentsWidget::updateWidget()
{
    m_textEdit->setReadOnly(m_mode == attachments::OpenMode::ReadOnly);

    if (m_mode == attachments::OpenMode::ReadOnly) {
        m_splitter->setSizes({0, 1});
        m_textEdit->hide();
    } else {
        m_splitter->setSizes({1, 0});
        m_textEdit->show();
    }

    m_textEdit->setText(m_attachment.data);
    m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
}

void TextAttachmentsWidget::initWidget()
{
    m_splitter = new QSplitter(this);
    m_textEdit = new QTextEdit(this);

    m_previewWidget = new TextAttachmentsPreviewWidget(this);

    connect(m_textEdit, &QTextEdit::textChanged, [this]() {
        m_attachment.data = m_textEdit->toPlainText().toUtf8();
        m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
    });

    m_splitter->addWidget(m_textEdit);
    m_splitter->addWidget(m_previewWidget);

    m_ui->verticalLayout->addWidget(m_splitter);

    updateWidget();
}

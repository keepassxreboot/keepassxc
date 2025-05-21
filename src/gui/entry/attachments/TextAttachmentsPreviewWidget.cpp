#include "TextAttachmentsPreviewWidget.h"
#include "ui_TextAttachmentsPreviewWidget.h"

#include <QComboBox>
#include <QDebug>
#include <QMetaEnum>

TextAttachmentsPreviewWidget::TextAttachmentsPreviewWidget(QWidget* parent)
    : attachments::AbstractAttachmentWidget(parent)
    , m_ui(new Ui::TextAttachmentsPreviewWidget())
{
    m_ui->setupUi(this);

    initTypeCombobox();
}

TextAttachmentsPreviewWidget::~TextAttachmentsPreviewWidget() = default;

void TextAttachmentsPreviewWidget::openAttachment(attachments::Attachment attachments, attachments::OpenMode mode)
{
    if (mode == attachments::OpenMode::ReadWrite) {
        qWarning() << "Read-write mode is not supported for text preview attachments";
    }

    m_attachment = std::move(attachments);

    onTypeChanged(m_ui->typeComboBox->currentIndex());
}

attachments::Attachment TextAttachmentsPreviewWidget::getAttachment() const
{
    return m_attachment;
}

void TextAttachmentsPreviewWidget::initTypeCombobox()
{
    const auto metaEnum = QMetaEnum::fromType<TextAttachmentsPreviewWidget::PreviewTextType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_ui->typeComboBox->addItem(tr(metaEnum.key(i)), metaEnum.value(i));
    }

    connect(m_ui->typeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &TextAttachmentsPreviewWidget::onTypeChanged);

    m_ui->typeComboBox->setCurrentIndex(m_ui->typeComboBox->findData(PlainText));

    onTypeChanged(m_ui->typeComboBox->currentIndex());
}

void TextAttachmentsPreviewWidget::onTypeChanged(int index)
{
    if (index < 0) {
        qWarning() << "TextAttachmentsPreviewWidget: Unknown text format";
    }

    const auto fileType = m_ui->typeComboBox->itemData(index).toInt();
    if (fileType == TextAttachmentsPreviewWidget::PreviewTextType::PlainText) {
        m_ui->previewTextBrowser->setPlainText(m_attachment.data);
    }

    if (fileType == TextAttachmentsPreviewWidget::PreviewTextType::Html) {
        m_ui->previewTextBrowser->setHtml(m_attachment.data);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (fileType == TextAttachmentsPreviewWidget::PreviewTextType::Markdown) {
        m_ui->previewTextBrowser->setMarkdown(m_attachment.data);
    }
#endif
}

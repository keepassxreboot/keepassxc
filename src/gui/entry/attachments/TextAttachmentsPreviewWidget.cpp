#include "TextAttachmentsPreviewWidget.h"
#include "ui_TextAttachmentsPreviewWidget.h"

#include <core/Tools.h>

#include <QComboBox>
#include <QDebug>
#include <QMetaEnum>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace
{
    constexpr TextAttachmentsPreviewWidget::PreviewTextType ConvertToPreviewTextType(Tools::MimeType mimeType) noexcept
    {
        if (mimeType == Tools::MimeType::Html) {
            return TextAttachmentsPreviewWidget::Html;
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        if (mimeType == Tools::MimeType::Markdown) {
            return TextAttachmentsPreviewWidget::Markdown;
        }
#endif

        return TextAttachmentsPreviewWidget::PlainText;
    }

} // namespace

TextAttachmentsPreviewWidget::TextAttachmentsPreviewWidget(QWidget* parent)
    : QWidget(parent)
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

    updateUi();
}

attachments::Attachment TextAttachmentsPreviewWidget::getAttachment() const
{
    return m_attachment;
}

void TextAttachmentsPreviewWidget::initTypeCombobox()
{
    QStandardItemModel* model = new QStandardItemModel(this);

    const auto metaEnum = QMetaEnum::fromType<TextAttachmentsPreviewWidget::PreviewTextType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        QStandardItem* item = new QStandardItem(metaEnum.key(i));
        item->setData(metaEnum.value(i), Qt::UserRole);
        model->appendRow(item);
    }

    QSortFilterProxyModel* filterProxyMode = new QSortFilterProxyModel(this);
    filterProxyMode->setSourceModel(model);
    filterProxyMode->sort(0, Qt::SortOrder::DescendingOrder);
    m_ui->typeComboBox->setModel(filterProxyMode);

    connect(m_ui->typeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &TextAttachmentsPreviewWidget::onTypeChanged);

    m_ui->typeComboBox->setCurrentIndex(m_ui->typeComboBox->findData(PlainText));

    onTypeChanged(m_ui->typeComboBox->currentIndex());
}

void TextAttachmentsPreviewWidget::updateUi()
{
    if (!m_attachment.name.isEmpty()) {
        const auto mimeType = Tools::getMimeType(QFileInfo(m_attachment.name));

        auto index = m_ui->typeComboBox->findData(ConvertToPreviewTextType(mimeType));
        m_ui->typeComboBox->setCurrentIndex(index);
    }

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

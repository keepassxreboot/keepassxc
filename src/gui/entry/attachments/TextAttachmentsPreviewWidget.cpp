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

#include "TextAttachmentsPreviewWidget.h"
#include "ui_TextAttachmentsPreviewWidget.h"

#include <core/Tools.h>

#include <QComboBox>
#include <QDebug>
#include <QMetaEnum>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTimer>

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
    , m_userManuallySelectedType(false)
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

    connect(m_ui->typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_userManuallySelectedType = true;
        onTypeChanged(index);
    });

    // Configure text browser to open external links
    m_ui->previewTextBrowser->setOpenExternalLinks(true);

    m_ui->previewTextBrowser->setTabStopDistance(
        QFontMetrics(m_ui->previewTextBrowser->font()).horizontalAdvance(QString(4, ' ')));

    m_ui->typeComboBox->setCurrentIndex(m_ui->typeComboBox->findData(PlainText));

    onTypeChanged(m_ui->typeComboBox->currentIndex());
}

void TextAttachmentsPreviewWidget::updateUi()
{
    // Only auto-select format based on file extension if user hasn't manually chosen one
    if (!m_userManuallySelectedType && !m_attachment.name.isEmpty()) {
        const auto mimeType = Tools::getMimeType(QFileInfo(m_attachment.name));

        auto index = m_ui->typeComboBox->findData(ConvertToPreviewTextType(mimeType));
        if (index >= 0) {
            // Temporarily block signals to avoid triggering manual selection flag
            m_ui->typeComboBox->blockSignals(true);
            m_ui->typeComboBox->setCurrentIndex(index);
            m_ui->typeComboBox->blockSignals(false);
        }
    }

    onTypeChanged(m_ui->typeComboBox->currentIndex());
}

void TextAttachmentsPreviewWidget::onTypeChanged(int index)
{
    if (index < 0) {
        qWarning() << "TextAttachmentsPreviewWidget: Unknown text format";
    }

    const auto scrollPos = m_ui->previewTextBrowser->verticalScrollBar()->value();
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

    // Delay setting the scrollbar position to ensure the text is rendered first
    QTimer::singleShot(
        100, this, [this, scrollPos] { m_ui->previewTextBrowser->verticalScrollBar()->setValue(scrollPos); });
}

void TextAttachmentsPreviewWidget::matchScroll(double percent)
{
    // Match the scroll position of the text browser to the given percentage
    int maxScroll = m_ui->previewTextBrowser->verticalScrollBar()->maximum();
    int newScrollPos = static_cast<int>(maxScroll * percent);
    m_ui->previewTextBrowser->verticalScrollBar()->setValue(newScrollPos);
}

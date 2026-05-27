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

#include "TextAttachmentsWidget.h"
#include "TextAttachmentsEditWidget.h"
#include "TextAttachmentsPreviewWidget.h"

#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

TextAttachmentsWidget::TextAttachmentsWidget(QWidget* parent)
    : QWidget(parent)
    , m_previewUpdateTimer(new QTimer(this))
    , m_mode(attachments::OpenMode::ReadOnly)
{
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
    if (m_mode == attachments::OpenMode::ReadWrite) {
        return m_editWidget->getAttachment();
    }

    return m_attachment;
}

void TextAttachmentsWidget::updateWidget()
{
    if (m_mode == attachments::OpenMode::ReadOnly) {
        // Only show the preview widget in read-only mode
        m_splitter->setSizes({0, 1});
        m_editWidget->hide();
        m_previewWidget->openAttachment(m_attachment, m_mode);
    } else {
        // Show the edit widget and hide the preview by default in read-write mode
        m_splitter->setSizes({1, 0});
        m_editWidget->show();
        m_editWidget->openAttachment(m_attachment, m_mode);
    }
}

void TextAttachmentsWidget::updatePreviewWidget()
{
    m_previewVisible = isPreviewVisible();
    if (m_previewVisible) {
        m_attachment = m_editWidget->getAttachment();
        m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
    }
}

void TextAttachmentsWidget::initWidget()
{
    m_splitter = new QSplitter(this);
    m_editWidget = new TextAttachmentsEditWidget(this);
    m_previewWidget = new TextAttachmentsPreviewWidget(this);

    m_previewUpdateTimer->setSingleShot(true);
    m_previewUpdateTimer->setInterval(500);

    // Only update the preview after a set timeout and if it is visible
    connect(m_previewUpdateTimer, &QTimer::timeout, this, &TextAttachmentsWidget::updatePreviewWidget);
    connect(m_editWidget,
            &TextAttachmentsEditWidget::scrollChanged,
            m_previewWidget,
            &TextAttachmentsPreviewWidget::matchScroll);

    connect(
        m_editWidget, &TextAttachmentsEditWidget::textChanged, m_previewUpdateTimer, QOverload<>::of(&QTimer::start));

    connect(m_editWidget, &TextAttachmentsEditWidget::previewButtonClicked, [this] {
        // Split the display in half if showing the preview widget
        const auto previewSize = isPreviewVisible() ? 0 : m_splitter->width() / 2;
        const auto editSize = m_splitter->width() - previewSize;
        m_splitter->setSizes({editSize, previewSize});
        updatePreviewWidget();
    });

    // Check if the preview panel is manually collapsed or shown
    connect(m_splitter, &QSplitter::splitterMoved, this, [this](int, int) {
        // Trigger a preview update if it has become visible
        auto visible = isPreviewVisible();
        if (visible && !m_previewVisible) {
            updatePreviewWidget();
        }
        m_previewVisible = visible;
    });

    m_splitter->addWidget(m_editWidget);
    m_splitter->addWidget(m_previewWidget);
    // Prevent collapsing of the edit widget
    m_splitter->setCollapsible(0, false);

    // Setup this widget with the splitter
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
    setLayout(layout);
    setObjectName("TextAttachmentsWidget");

    updateWidget();
}

bool TextAttachmentsWidget::isPreviewVisible() const
{
    return m_previewWidget->width() > 0;
}

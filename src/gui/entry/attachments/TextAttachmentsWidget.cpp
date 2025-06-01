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

#include "ui_TextAttachmentsWidget.h"

#include <QSplitter>
#include <QTextEdit>

TextAttachmentsWidget::TextAttachmentsWidget(QWidget* parent)
    : QWidget(parent)
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
    if (m_mode == attachments::OpenMode::ReadOnly) {
        m_splitter->setSizes({0, 1});
        m_editWidget->hide();
    } else {
        m_splitter->setSizes({1, 0});
        m_editWidget->show();
    }

    m_editWidget->openAttachment(m_attachment, m_mode);
    m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
}

void TextAttachmentsWidget::initWidget()
{
    m_splitter = new QSplitter(this);
    m_editWidget = new TextAttachmentsEditWidget(this);
    m_previewWidget = new TextAttachmentsPreviewWidget(this);

    connect(editWidget, &TextAttachmentsEditWidget::textChanged, [this]() {
        m_attachment = m_editWidget->getAttachment();
        m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
    });

    connect(editWidget, &TextAttachmentsEditWidget::previewButtonClicked, [this]() {
        const auto sizes = m_splitter->sizes();
        const auto previewSize = sizes.value(1, 0) > 0 ? 0 : 1;
        m_splitter->setSizes({1, previewSize});
    });

    m_splitter->addWidget(m_editWidget);
    m_splitter->addWidget(m_previewWidget);
    // Prevent collapsing of the edit widget
    m_splitter->setCollapsible(0, false);

    m_ui->verticalLayout->addWidget(m_splitter);

    updateWidget();
}

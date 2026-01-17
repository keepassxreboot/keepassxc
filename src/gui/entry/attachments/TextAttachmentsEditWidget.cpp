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

#include "TextAttachmentsEditWidget.h"
#include "ui_TextAttachmentsEditWidget.h"

#include <QPushButton>
#include <QScrollBar>
#include <QTextEdit>

TextAttachmentsEditWidget::TextAttachmentsEditWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::TextAttachmentsEditWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->attachmentsTextEdit, &QTextEdit::textChanged, this, &TextAttachmentsEditWidget::textChanged);
    connect(m_ui->previewPushButton, &QPushButton::clicked, this, &TextAttachmentsEditWidget::previewButtonClicked);
    connect(m_ui->attachmentsTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        // Return a percentage of document scroll
        auto percent = static_cast<double>(value) / m_ui->attachmentsTextEdit->verticalScrollBar()->maximum();
        emit scrollChanged(percent);
    });
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
    return {m_attachment.name, m_ui->attachmentsTextEdit->toPlainText().toUtf8()};
}

void TextAttachmentsEditWidget::updateUi()
{
    m_ui->attachmentsTextEdit->setPlainText(m_attachment.data);
    m_ui->attachmentsTextEdit->setReadOnly(m_mode == attachments::OpenMode::ReadOnly);

    m_ui->attachmentsTextEdit->setTabStopDistance(
        QFontMetrics(m_ui->attachmentsTextEdit->font()).horizontalAdvance(QString(4, ' ')));
}

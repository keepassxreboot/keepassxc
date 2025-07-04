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

#pragma once

#include "AttachmentTypes.h"

#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class TextAttachmentsEditWidget;
}

class TextAttachmentsEditWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TextAttachmentsEditWidget(QWidget* parent = nullptr);
    ~TextAttachmentsEditWidget() override;

    void openAttachment(attachments::Attachment attachment, attachments::OpenMode mode);
    attachments::Attachment getAttachment() const;

signals:
    void textChanged();
    void scrollChanged(double percent);
    void previewButtonClicked(bool isChecked);

private:
    void updateUi();

    QScopedPointer<Ui::TextAttachmentsEditWidget> m_ui;

    attachments::Attachment m_attachment;
    attachments::OpenMode m_mode;
};

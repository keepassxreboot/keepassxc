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
    class TextAttachmentsPreviewWidget;
}

class TextAttachmentsPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TextAttachmentsPreviewWidget(QWidget* parent = nullptr);
    ~TextAttachmentsPreviewWidget() override;

    void openAttachment(attachments::Attachment attachment, attachments::OpenMode mode);
    attachments::Attachment getAttachment() const;

    enum PreviewTextType : int
    {
        Html,
        PlainText,
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        Markdown
#endif
    };

    Q_ENUM(PreviewTextType)

public slots:
    void matchScroll(double percent);

private slots:
    void onTypeChanged(int index);

private:
    void initTypeCombobox();
    void updateUi();

    QScopedPointer<Ui::TextAttachmentsPreviewWidget> m_ui;

    attachments::Attachment m_attachment;
    bool m_userManuallySelectedType;
};

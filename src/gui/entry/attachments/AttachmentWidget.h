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

#include <core/Tools.h>

#include <QPointer>
#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class AttachmentWidget;
}

/**
 * @brief The AttachmentWidget class provides a way to manage attachments in a GUI application.
 *
 */
class AttachmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttachmentWidget(QWidget* parent = nullptr);
    ~AttachmentWidget() override;

    /**
     * @brief Opens an attachment in the specified mode.
     *
     * @param attachment - The attachment to be opened.
     * @param mode - The mode in which to open the attachment (read-only or read-write).
     */
    void openAttachment(attachments::Attachment attachment, attachments::OpenMode mode);

    /**
     * @brief Get the current attachment.
     *
     * @return Attachment - The current attachment.
     */
    attachments::Attachment getAttachment() const;

private:
    void updateUi();

    QPointer<QWidget> m_attachmentWidget;

    attachments::Attachment m_attachment;
    attachments::OpenMode m_mode;
};

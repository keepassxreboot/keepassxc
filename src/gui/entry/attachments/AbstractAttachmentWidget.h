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

#include <QByteArray>
#include <QString>
#include <QWidget>

namespace attachments
{
    struct Attachment
    {
        QString name;
        QByteArray data;
    };

    enum class OpenMode
    {
        ReadOnly,
        ReadWrite
    };

    /**
     * @brief The AbstractAttachmentWidget abstract class provides a way to manage attachments in a GUI application.
     *
     * This abstract class allows for opening, reading, and writing attachments.
     */
    class AbstractAttachmentWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit AbstractAttachmentWidget(QWidget* parent = nullptr);

        /**
         * @brief Opens an attachment in the specified mode.
         *
         * @param attachment - The attachment to be opened.
         * @param mode - The mode in which to open the attachment (read-only or read-write).
         */
        virtual void openAttachment(Attachment attachment, OpenMode mode) = 0;

        /**
         * @brief Get the current attachment.
         *
         * @return Attachment - The current attachment.
         */
        virtual Attachment getAttachment() const = 0;
    };

} // namespace attachments

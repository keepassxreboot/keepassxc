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

#include "AbstractAttachmentWidget.h"
#include "core/Tools.h"

namespace attachments
{
    /**
     * @brief The IAttachmentWidgetFactory interface provides a way to create attachment widgets for different MIME
     * types.
     *
     * This interface allows for the creation of attachment widgets based on the specified MIME type.
     */
    class IAttachmentWidgetFactory
    {
    public:
        virtual ~IAttachmentWidgetFactory() = default;
        /**
         * @brief Creates an attachment widget for the specified MIME type.
         *
         * @param type The MIME type of the attachment.
         * @param parent The parent widget for the attachment widget.
         * @return A pointer to the created attachment widget.
         */
        virtual QPointer<AbstractAttachmentWidget> createAttachmentWidget(Tools::MimeType type, QWidget* parent) = 0;
    };

} // namespace attachments

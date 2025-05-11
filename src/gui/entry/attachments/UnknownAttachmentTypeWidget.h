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

#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class UnknownAttachmentTypeWidget;
}

class UnknownAttachmentTypeWidget : public attachments::AbstractAttachmentWidget
{
public:
    explicit UnknownAttachmentTypeWidget(QWidget* parent = nullptr);

    ~UnknownAttachmentTypeWidget() override = default;

    void openAttachment(attachments::Attachment attachment, [[maybe_unused]] attachments::OpenMode mode) override;
    attachments::Attachment getAttachment() const override;

private:
    QScopedPointer<Ui::UnknownAttachmentTypeWidget> m_ui{};

    attachments::Attachment m_attachment{};
};

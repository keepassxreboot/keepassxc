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

#include "attachments/AttachmentTypes.h"

#include <core/Tools.h>

#include <QDialog>
#include <QPointer>

namespace Ui
{
    class PreviewEntryAttachmentsDialog;
}

class PreviewEntryAttachmentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreviewEntryAttachmentsDialog(QWidget* parent = nullptr);
    ~PreviewEntryAttachmentsDialog() override;

    void setAttachment(attachments::Attachment attachment);

signals:
    void openAttachment(const QString& name);
    void saveAttachment(const QString& name);

private:
    QScopedPointer<Ui::PreviewEntryAttachmentsDialog> m_ui;
};

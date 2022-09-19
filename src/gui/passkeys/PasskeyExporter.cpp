/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PasskeyExporter.h"
#include "PasskeyExportDialog.h"

#include "browser/BrowserPasskeys.h"
#include "core/Entry.h"
#include "core/Tools.h"
#include "gui/MessageBox.h"
#include <QFile>
#include <QTextStream>

void PasskeyExporter::showExportDialog(const QList<Entry*>& items)
{
    if (items.isEmpty()) {
        return;
    }

    PasskeyExportDialog passkeyExportDialog;
    passkeyExportDialog.setEntries(items);
    auto ret = passkeyExportDialog.exec();

    if (ret == QDialog::Accepted) {
        // Select folder
        auto folder = passkeyExportDialog.selectExportFolder();
        if (folder.isEmpty()) {
            return;
        }

        const auto selectedItems = passkeyExportDialog.getSelectedItems();
        for (const auto& item : selectedItems) {
            auto entry = items[item->row()];
            exportSelectedEntry(entry, folder);
        }
    }
}

/**
 * Creates an export file for a Passkey credential
 *
 * File contents:
 * <URL>
 * <username>
 * <password / generated ID>
 * <key data>
 */
void PasskeyExporter::exportSelectedEntry(const Entry* entry, const QString& folder)
{
    const auto fullPath = QString("%1/%2.passkey").arg(folder, Tools::cleanFilename(entry->title()));
    if (QFile::exists(fullPath)) {
        auto dialogResult = MessageBox::warning(nullptr,
                                                tr("KeePassXC: Passkey Export"),
                                                tr("File \"%1.passkey\" already exists.\n"
                                                   "Do you want to overwrite it?\n")
                                                    .arg(entry->title()),
                                                MessageBox::Yes | MessageBox::No);

        if (dialogResult != MessageBox::Yes) {
            return;
        }
    }

    QFile passkeyFile(fullPath);
    if (!passkeyFile.open(QIODevice::WriteOnly)) {
        MessageBox::information(
            nullptr, tr("Cannot open file"), tr("Cannot open file \"%1\" for writing.").arg(fullPath));
        return;
    }

    QTextStream fileStream(&passkeyFile);
    fileStream << entry->attributes()->value(BrowserPasskeys::KPEX_PASSKEY_RELYING_PARTY) << "\n";
    fileStream << entry->url() << "\n";
    fileStream << entry->username() << "\n";
    fileStream << entry->attributes()->value(BrowserPasskeys::KPEX_PASSKEY_GENERATED_USER_ID) << "\n";
    fileStream << entry->attributes()->value(BrowserPasskeys::KPEX_PASSKEY_USER_HANDLE) << "\n";
    fileStream << entry->attributes()->value(BrowserPasskeys::KPEX_PASSKEY_PRIVATE_KEY_PEM);

    passkeyFile.close();
}

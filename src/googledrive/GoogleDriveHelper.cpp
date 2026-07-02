/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "GoogleDriveHelper.h"
#include "GoogleDriveBrowserDialog.h"
#include "GoogleDriveService.h"

GoogleDriveService* GoogleDriveHelper::createService(QObject* parent)
{
    return new GoogleDriveService(parent);
}

GoogleDriveOpenResult GoogleDriveHelper::pickOpenFile(QWidget* parent)
{
    auto* dialog = new GoogleDriveBrowserDialog(parent, GoogleDriveBrowserDialog::Open);
    GoogleDriveOpenResult result;
    if (dialog->exec() == QDialog::Accepted) {
        auto file = dialog->selectedFile();
        result.fileId = file.id;
        result.fileName = file.name;
    }
    dialog->deleteLater();
    return result;
}

GoogleDriveSaveResult GoogleDriveHelper::pickSaveDestination(QWidget* parent)
{
    auto* dialog = new GoogleDriveBrowserDialog(parent, GoogleDriveBrowserDialog::Save);
    GoogleDriveSaveResult result;
    if (dialog->exec() == QDialog::Accepted) {
        result.folderId = dialog->selectedFolderId();
        result.fileName = dialog->saveFileName();
    }
    dialog->deleteLater();
    return result;
}

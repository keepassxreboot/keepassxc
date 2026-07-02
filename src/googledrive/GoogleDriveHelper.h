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

#ifndef KEEPASSXC_GOOGLEDRIVEHELPER_H
#define KEEPASSXC_GOOGLEDRIVEHELPER_H

#include <QString>

class GoogleDriveService;
class GoogleDriveBrowserDialog;
class QWidget;

struct GoogleDriveOpenResult
{
    QString fileId;
    QString fileName;
    bool isValid() const
    {
        return !fileId.isEmpty();
    }
};

struct GoogleDriveSaveResult
{
    QString folderId;
    QString fileName;
    bool isValid() const
    {
        return !folderId.isEmpty() && !fileName.isEmpty();
    }
};

class GoogleDriveHelper
{
public:
    static GoogleDriveService* createService(QObject* parent);
    static GoogleDriveOpenResult pickOpenFile(QWidget* parent);
    static GoogleDriveSaveResult pickSaveDestination(QWidget* parent);
};

#endif // KEEPASSXC_GOOGLEDRIVEHELPER_H

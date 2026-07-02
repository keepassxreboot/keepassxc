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

#ifndef KEEPASSXC_GOOGLEDRIVEBROWSERDIALOG_H
#define KEEPASSXC_GOOGLEDRIVEBROWSERDIALOG_H

#include <QDialog>

#include "GoogleDriveService.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class GoogleDriveBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode { Open, Save };

    struct DriveFile
    {
        QString id;
        QString name;
        QString modifiedTime;
        qint64 size;
    };

    explicit GoogleDriveBrowserDialog(QWidget* parent, Mode mode = Open);

    DriveFile selectedFile() const;
    QString selectedFolderId() const;
    QString saveFileName() const;

private slots:
    void refreshFiles();
    void navigateToParent();
    void createNewFolder();
    void onFileListReady(const QList<GoogleDriveService::FileInfo>& files);
    void onFileListFailed(const QString& error);
    void onFolderCreated(const QString& folderId, const QString& name);
    void onFolderCreateFailed(const QString& error);
    void onAuthStatusChanged(bool authenticated);
    void onAuthFailed(const QString& error);
    void onSelectionChanged();
    void onCellDoubleClicked(int row, int column);
    void onFileNameChanged(const QString& text);

private:
    Mode m_mode;
    GoogleDriveService* m_service;
    QString m_currentFolderId;
    QString m_parentFolderId;
    QString m_selectedFolderId;
    QString m_selectedFolderName;
    QLabel* m_pathLabel;
    QLabel* m_statusLabel;
    QPushButton* m_connectButton;
    QPushButton* m_upButton;
    QPushButton* m_refreshButton;
    QPushButton* m_actionButton;
    QPushButton* m_newFolderButton;
    QLineEdit* m_fileNameEdit;
    QTableWidget* m_fileTable;
    DriveFile m_selectedFile;

    void updateActionButton();
};

#endif // KEEPASSXC_GOOGLEDRIVEBROWSERDIALOG_H

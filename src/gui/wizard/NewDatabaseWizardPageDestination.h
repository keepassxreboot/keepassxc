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

#ifndef KEEPASSXC_NEWDATABASEWIZARDPAGEDESTINATION_H
#define KEEPASSXC_NEWDATABASEWIZARDPAGEDESTINATION_H

#include "NewDatabaseWizardPage.h"

#include <QString>

#include "config-keepassx.h"

#ifdef KPXC_FEATURE_GOOGLEDRIVE
#include "googledrive/GoogleDriveService.h"
#endif

class GoogleDriveService;
class QFileSystemModel;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTreeView;

class NewDatabaseWizardPageDestination : public NewDatabaseWizardPage
{
    Q_OBJECT

public:
    explicit NewDatabaseWizardPageDestination(QWidget* parent = nullptr);
    Q_DISABLE_COPY(NewDatabaseWizardPageDestination);
    ~NewDatabaseWizardPageDestination() override;

    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

    QString localFilePath() const;
    QString driveFolderId() const;
    QString driveFileName() const;
    bool isDriveSelected() const;

private slots:
    void browseLocalFolder();

#ifdef KPXC_FEATURE_GOOGLEDRIVE
    void onDriveFileListReady(const QList<GoogleDriveService::FileInfo>& files);
    void onDriveFileListFailed(const QString& error);
    void onDriveFolderCreated(const QString& folderId, const QString& name);
    void onDriveFolderCreateFailed(const QString& error);
    void onDriveAuthStatusChanged(bool authenticated);
    void onDriveAuthFailed(const QString& error);
    void onDriveSelectionChanged();
    void onDriveCellDoubleClicked(int row, int column);
    void onDriveFileNameChanged(const QString& text);
    void navigateDriveToParent();
    void createDriveFolder();
    void refreshDriveFiles();
#endif

private:
    QStackedWidget* m_stack;

    // Local
    QLineEdit* m_localNameEdit;
    QLineEdit* m_localPathEdit;
    QPushButton* m_localBrowseButton;
    QTreeView* m_localTree;

    // Drive
#ifdef KPXC_FEATURE_GOOGLEDRIVE
    GoogleDriveService* m_driveService;
    QString m_driveCurrentFolderId;
    QString m_driveParentFolderId;
    QString m_driveSelectedFolderId;
    QString m_driveSelectedFolderName;
    QString m_driveFileName;
    QLabel* m_drivePathLabel;
    QLabel* m_driveStatusLabel;
    QPushButton* m_driveUpButton;
    QPushButton* m_driveNewFolderButton;
    QPushButton* m_driveRefreshButton;
    QLineEdit* m_driveFileNameEdit;
    QTableWidget* m_driveFileTable;
    QPushButton* m_driveConnectButton;
#endif

    void setupLocalPage();
    void setupDrivePage();
};

#endif // KEEPASSXC_NEWDATABASEWIZARDPAGEDESTINATION_H

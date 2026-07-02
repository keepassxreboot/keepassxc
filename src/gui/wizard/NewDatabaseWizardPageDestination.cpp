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

#include "NewDatabaseWizardPageDestination.h"

#include "config-keepassx.h"
#include "ui_NewDatabaseWizardPage.h"

#include "gui/dbsettings/DatabaseSettingsWidgetMetaDataSimple.h"
#include "gui/wizard/NewDatabaseWizardPageMetaData.h"

#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeView>
#include <QVBoxLayout>

NewDatabaseWizardPageDestination::NewDatabaseWizardPageDestination(QWidget* parent)
    : NewDatabaseWizardPage(parent)
{
    setTitle(tr("Save Location"));
    setSubTitle(tr("Choose where to save your new database:"));

    m_stack = new QStackedWidget();
    m_ui->pageContent->setWidget(m_stack);

    auto* localPage = new QWidget();
    m_stack->addWidget(localPage);
    setupLocalPage();

    auto* drivePage = new QWidget();
    m_stack->addWidget(drivePage);
    setupDrivePage();
}

NewDatabaseWizardPageDestination::~NewDatabaseWizardPageDestination() = default;

void NewDatabaseWizardPageDestination::setupLocalPage()
{
    auto* page = m_stack->widget(0);
    auto* layout = new QVBoxLayout(page);

    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel(tr("File name:"), page));
    m_localNameEdit = new QLineEdit(page);
    m_localNameEdit->setPlaceholderText(tr("MyPasswords"));
    nameLayout->addWidget(m_localNameEdit, 1);
    auto* extLabel = new QLabel(".kdbx", page);
    extLabel->setStyleSheet("color: gray;");
    nameLayout->addWidget(extLabel);
    layout->addLayout(nameLayout);

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel(tr("Folder:"), page));
    m_localPathEdit = new QLineEdit(page);
    m_localPathEdit->setPlaceholderText(QDir::homePath());
    pathLayout->addWidget(m_localPathEdit, 1);
    m_localBrowseButton = new QPushButton(tr("Browse..."), page);
    pathLayout->addWidget(m_localBrowseButton);
    layout->addLayout(pathLayout);

    m_localTree = new QTreeView(page);
    auto* fsModel = new QFileSystemModel(this);
    fsModel->setRootPath(QDir::homePath());
    fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    m_localTree->setModel(fsModel);
    m_localTree->setRootIndex(fsModel->index(QDir::homePath()));
    m_localTree->setAnimated(true);
    m_localTree->setIndentation(20);
    m_localTree->setSortingEnabled(true);
    m_localTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_localTree->setItemsExpandable(true);
    m_localTree->setDragEnabled(false);
    layout->addWidget(m_localTree);

    connect(m_localTree->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) {
        auto* model = qobject_cast<QFileSystemModel*>(m_localTree->model());
        if (model) {
            m_localPathEdit->setText(model->filePath(current));
        }
    });

    connect(m_localBrowseButton, &QPushButton::clicked, this, &NewDatabaseWizardPageDestination::browseLocalFolder);
}

void NewDatabaseWizardPageDestination::setupDrivePage()
{
#ifdef KPXC_FEATURE_GOOGLEDRIVE
    auto* page = m_stack->widget(1);
    auto* mainLayout = new QVBoxLayout(page);

    m_driveConnectButton = new QPushButton(tr("Connect to Google Drive"), page);
    mainLayout->addWidget(m_driveConnectButton);

    m_drivePathLabel = new QLabel(tr("My Drive"), page);
    mainLayout->addWidget(m_drivePathLabel);

    m_driveStatusLabel = new QLabel(page);
    mainLayout->addWidget(m_driveStatusLabel);

    m_driveFileTable = new QTableWidget(0, 3, page);
    m_driveFileTable->setHorizontalHeaderLabels({tr("Name"), tr("Modified"), tr("Size")});
    m_driveFileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_driveFileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_driveFileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_driveFileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_driveFileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_driveFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_driveFileTable->verticalHeader()->hide();
    mainLayout->addWidget(m_driveFileTable);

    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel(tr("File name:"), page));
    m_driveFileNameEdit = new QLineEdit(page);
    m_driveFileNameEdit->setPlaceholderText(tr("MyPasswords"));
    nameLayout->addWidget(m_driveFileNameEdit, 1);
    auto* extLabel = new QLabel(".kdbx", page);
    extLabel->setStyleSheet("color: gray;");
    nameLayout->addWidget(extLabel);
    mainLayout->addLayout(nameLayout);

    auto* buttonLayout = new QHBoxLayout();
    m_driveUpButton = new QPushButton(tr("Up"), page);
    m_driveUpButton->setEnabled(false);
    buttonLayout->addWidget(m_driveUpButton);

    m_driveNewFolderButton = new QPushButton(tr("New Folder"), page);
    m_driveNewFolderButton->setEnabled(false);
    buttonLayout->addWidget(m_driveNewFolderButton);

    m_driveRefreshButton = new QPushButton(tr("Refresh"), page);
    m_driveRefreshButton->setEnabled(false);
    buttonLayout->addWidget(m_driveRefreshButton);

    mainLayout->addLayout(buttonLayout);

    m_driveService = new GoogleDriveService(this);

    connect(m_driveConnectButton, &QPushButton::clicked, m_driveService, &GoogleDriveService::authenticate);
    connect(m_driveUpButton, &QPushButton::clicked, this, &NewDatabaseWizardPageDestination::navigateDriveToParent);
    connect(m_driveNewFolderButton, &QPushButton::clicked, this, &NewDatabaseWizardPageDestination::createDriveFolder);
    connect(m_driveRefreshButton, &QPushButton::clicked, this, &NewDatabaseWizardPageDestination::refreshDriveFiles);
    connect(m_driveFileTable, &QTableWidget::itemSelectionChanged, this, &NewDatabaseWizardPageDestination::onDriveSelectionChanged);
    connect(m_driveFileTable, &QTableWidget::cellDoubleClicked, this, &NewDatabaseWizardPageDestination::onDriveCellDoubleClicked);
    connect(m_driveFileNameEdit, &QLineEdit::textChanged, this, &NewDatabaseWizardPageDestination::onDriveFileNameChanged);

    connect(m_driveService, &GoogleDriveService::authStatusChanged, this, &NewDatabaseWizardPageDestination::onDriveAuthStatusChanged);
    connect(m_driveService, &GoogleDriveService::authFailed, this, &NewDatabaseWizardPageDestination::onDriveAuthFailed);
    connect(m_driveService, &GoogleDriveService::fileListReady, this, &NewDatabaseWizardPageDestination::onDriveFileListReady);
    connect(m_driveService, &GoogleDriveService::fileListFailed, this, &NewDatabaseWizardPageDestination::onDriveFileListFailed);
    connect(m_driveService, &GoogleDriveService::folderCreated, this, &NewDatabaseWizardPageDestination::onDriveFolderCreated);
    connect(m_driveService, &GoogleDriveService::folderCreateFailed, this, &NewDatabaseWizardPageDestination::onDriveFolderCreateFailed);
#else
    auto* page = m_stack->widget(1);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel(tr("Google Drive support is not available in this build."), page));
#endif
}

void NewDatabaseWizardPageDestination::initializePage()
{
    auto* metaPage = qobject_cast<NewDatabaseWizardPageMetaData*>(wizard()->page(0));
    bool isDrive = metaPage ? metaPage->isDriveSelected() : false;

    m_stack->setCurrentIndex(isDrive ? 1 : 0);

#ifdef KPXC_FEATURE_GOOGLEDRIVE
    if (isDrive) {
        bool auth = m_driveService->isAuthenticated();
        onDriveAuthStatusChanged(auth);
        if (auth) {
            refreshDriveFiles();
        }
    }
#else
    Q_UNUSED(isDrive);
#endif
}

bool NewDatabaseWizardPageDestination::validatePage()
{
    bool isDrive = m_stack->currentIndex() == 1;

    if (!isDrive) {
        QString name = m_localNameEdit->text().trimmed();
        if (name.isEmpty()) {
            return false;
        }
        QString path = m_localPathEdit->text().trimmed();
        if (path.isEmpty()) {
            path = QDir::homePath();
        }
        return QDir(path).exists();
    }

#ifdef KPXC_FEATURE_GOOGLEDRIVE
    QString folderId = m_driveSelectedFolderId.isEmpty() ? m_driveCurrentFolderId : m_driveSelectedFolderId;
    if (folderId.isEmpty()) {
        return false;
    }
    QString name = m_driveFileNameEdit ? m_driveFileNameEdit->text().trimmed() : QString();
    if (name.isEmpty()) {
        return false;
    }
    m_driveFileName = name;
    if (!m_driveFileName.endsWith(".kdbx")) {
        m_driveFileName += ".kdbx";
    }
    return true;
#else
    return false;
#endif
}

int NewDatabaseWizardPageDestination::nextId() const
{
    return 2;
}

QString NewDatabaseWizardPageDestination::localFilePath() const
{
    QString path = m_localPathEdit->text().trimmed();
    if (path.isEmpty()) {
        path = QDir::homePath();
    }
    QString name = m_localNameEdit->text().trimmed();
    if (name.isEmpty()) {
        name = "Passwords";
    }
    if (!name.endsWith(".kdbx")) {
        name += ".kdbx";
    }
    return QDir(path).absoluteFilePath(name);
}

QString NewDatabaseWizardPageDestination::driveFolderId() const
{
#ifdef KPXC_FEATURE_GOOGLEDRIVE
    return m_driveSelectedFolderId.isEmpty() ? m_driveCurrentFolderId : m_driveSelectedFolderId;
#else
    return {};
#endif
}

QString NewDatabaseWizardPageDestination::driveFileName() const
{
#ifdef KPXC_FEATURE_GOOGLEDRIVE
    return m_driveFileName;
#else
    return {};
#endif
}

bool NewDatabaseWizardPageDestination::isDriveSelected() const
{
    return m_stack->currentIndex() == 1;
}

void NewDatabaseWizardPageDestination::browseLocalFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Save Folder"),
        m_localPathEdit->text().isEmpty() ? QDir::homePath() : m_localPathEdit->text());
    if (!dir.isEmpty()) {
        m_localPathEdit->setText(dir);
    }
}

// --- Drive slots ---

#ifdef KPXC_FEATURE_GOOGLEDRIVE

void NewDatabaseWizardPageDestination::refreshDriveFiles()
{
    m_driveStatusLabel->setText(tr("Loading..."));
    m_driveRefreshButton->setEnabled(false);
    m_driveFileTable->setRowCount(0);
    m_driveSelectedFolderId.clear();
    m_driveSelectedFolderName.clear();
    m_driveService->listFiles(m_driveCurrentFolderId);
}

void NewDatabaseWizardPageDestination::navigateDriveToParent()
{
    if (m_driveParentFolderId.isEmpty()) {
        return;
    }
    m_driveCurrentFolderId = m_driveParentFolderId;
    m_driveParentFolderId.clear();
    m_driveSelectedFolderId.clear();
    m_driveSelectedFolderName.clear();
    m_drivePathLabel->setText(tr("My Drive"));
    m_driveUpButton->setEnabled(false);
    refreshDriveFiles();
}

void NewDatabaseWizardPageDestination::createDriveFolder()
{
    bool ok = false;
    QString name = QInputDialog::getText(this,
        tr("New Folder"),
        tr("Folder name:"),
        QLineEdit::Normal,
        {},
        &ok);

    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    m_driveStatusLabel->setText(tr("Creating folder..."));
    m_driveNewFolderButton->setEnabled(false);
    m_driveService->createFolder(m_driveCurrentFolderId, name.trimmed());
}

void NewDatabaseWizardPageDestination::onDriveFolderCreated(const QString& folderId, const QString& name)
{
    Q_UNUSED(folderId);
    m_driveStatusLabel->setText(tr("Folder \"%1\" created.").arg(name));
    m_driveNewFolderButton->setEnabled(true);
    m_driveSelectedFolderId = folderId;
    m_driveSelectedFolderName = name;
    refreshDriveFiles();
}

void NewDatabaseWizardPageDestination::onDriveFolderCreateFailed(const QString& error)
{
    Q_UNUSED(error);
    m_driveStatusLabel->setText(tr("Failed to create folder: %1").arg(error));
    m_driveNewFolderButton->setEnabled(true);
}

void NewDatabaseWizardPageDestination::onDriveFileListReady(const QList<GoogleDriveService::FileInfo>& files)
{
    m_driveFileTable->setRowCount(0);

    for (const auto& f : files) {
        int row = m_driveFileTable->rowCount();
        m_driveFileTable->insertRow(row);

        QString displayName = f.isFolder()
            ? QString::fromUtf8("\xF0\x9F\x93\x81 ") + f.name
            : f.name;
        m_driveFileTable->setItem(row, 0, new QTableWidgetItem(displayName));
        m_driveFileTable->setItem(row, 1, new QTableWidgetItem(
            f.modifiedTime.isValid()
                ? f.modifiedTime.toLocalTime().toString("yyyy-MM-dd hh:mm")
                : QString()));

        QString sizeStr;
        if (f.isFolder()) {
            sizeStr = tr("Folder");
        } else if (f.size < 1024) {
            sizeStr = tr("%1 B").arg(f.size);
        } else if (f.size < 1024 * 1024) {
            sizeStr = tr("%1 KB").arg(f.size / 1024);
        } else {
            sizeStr = tr("%1 MB").arg(f.size / (1024 * 1024));
        }
        m_driveFileTable->setItem(row, 2, new QTableWidgetItem(sizeStr));

        m_driveFileTable->item(row, 0)->setData(Qt::UserRole, f.id);
        m_driveFileTable->item(row, 0)->setData(Qt::UserRole + 1, f.isFolder());
        m_driveFileTable->item(row, 0)->setData(Qt::UserRole + 2, f.name);
    }

    if (files.isEmpty()) {
        m_driveStatusLabel->setText(tr("Your Drive is empty. Click \"New Folder\" to create one."));
    } else {
        m_driveStatusLabel->setText(tr("%1 item(s)").arg(files.size()));
    }
    m_driveRefreshButton->setEnabled(true);
}

void NewDatabaseWizardPageDestination::onDriveFileListFailed(const QString& error)
{
    Q_UNUSED(error);
    m_driveStatusLabel->setText(tr("Error: %1").arg(error));
    m_driveRefreshButton->setEnabled(true);
}

void NewDatabaseWizardPageDestination::onDriveAuthStatusChanged(bool authenticated)
{
    Q_UNUSED(authenticated);
    m_driveConnectButton->setVisible(!authenticated);
    m_driveUpButton->setEnabled(authenticated && !m_driveParentFolderId.isEmpty());
    m_driveNewFolderButton->setEnabled(authenticated);
    m_driveRefreshButton->setEnabled(authenticated);
    m_driveFileTable->setEnabled(authenticated);
    m_driveFileNameEdit->setEnabled(authenticated);
    m_driveStatusLabel->setText(authenticated
        ? tr("Connected.")
        : tr("Not connected to Google Drive."));
}

void NewDatabaseWizardPageDestination::onDriveAuthFailed(const QString& error)
{
    Q_UNUSED(error);
    m_driveStatusLabel->setText(tr("Authentication failed: %1").arg(error));
    m_driveConnectButton->setVisible(true);
}

void NewDatabaseWizardPageDestination::onDriveSelectionChanged()
{
    int row = m_driveFileTable->currentRow();
    if (row >= 0) {
        auto* item = m_driveFileTable->item(row, 0);
        if (item) {
            bool isFolder = item->data(Qt::UserRole + 1).toBool();
            if (isFolder) {
                m_driveSelectedFolderId = item->data(Qt::UserRole).toString();
                m_driveSelectedFolderName = item->data(Qt::UserRole + 2).toString();
                m_drivePathLabel->setText(m_driveSelectedFolderName);
            } else if (m_driveFileNameEdit) {
                m_driveFileNameEdit->setText(item->data(Qt::UserRole + 2).toString());
            }
        }
    }
}

void NewDatabaseWizardPageDestination::onDriveCellDoubleClicked(int row, int /*column*/)
{
    Q_UNUSED(row);
    auto* item = m_driveFileTable->item(row, 0);
    if (!item) {
        return;
    }

    bool isFolder = item->data(Qt::UserRole + 1).toBool();
    QString fileId = item->data(Qt::UserRole).toString();

    if (isFolder) {
        m_driveParentFolderId = m_driveCurrentFolderId;
        m_driveCurrentFolderId = fileId;
        m_driveSelectedFolderId.clear();
        m_driveSelectedFolderName.clear();
        m_drivePathLabel->setText(item->data(Qt::UserRole + 2).toString());
        m_driveUpButton->setEnabled(true);
        refreshDriveFiles();
    }
}

void NewDatabaseWizardPageDestination::onDriveFileNameChanged(const QString&)
{
}

#endif

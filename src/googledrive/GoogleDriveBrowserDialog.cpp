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

#include "GoogleDriveBrowserDialog.h"
#include "GoogleDriveService.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

GoogleDriveBrowserDialog::GoogleDriveBrowserDialog(QWidget* parent, Mode mode)
    : QDialog(parent)
    , m_mode(mode)
    , m_service(new GoogleDriveService(this))
{
    setWindowTitle(mode == Save ? tr("Save to Google Drive") : tr("Open from Google Drive"));
    setMinimumSize(650, 450);

    auto* mainLayout = new QVBoxLayout(this);

    m_pathLabel = new QLabel(tr("My Drive"), this);
    mainLayout->addWidget(m_pathLabel);

    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);

    m_fileTable = new QTableWidget(0, 3, this);
    m_fileTable->setHorizontalHeaderLabels({tr("Name"), tr("Modified"), tr("Size")});
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->verticalHeader()->hide();
    mainLayout->addWidget(m_fileTable);

    if (mode == Save) {
        auto* nameLayout = new QHBoxLayout();
        nameLayout->addWidget(new QLabel(tr("File name:"), this));
        m_fileNameEdit = new QLineEdit(this);
        m_fileNameEdit->setPlaceholderText(tr("MyPasswords"));
        nameLayout->addWidget(m_fileNameEdit, 1);
        auto* extLabel = new QLabel(".kdbx", this);
        extLabel->setStyleSheet("color: gray;");
        nameLayout->addWidget(extLabel);
        mainLayout->addLayout(nameLayout);
        connect(m_fileNameEdit, &QLineEdit::textChanged, this, &GoogleDriveBrowserDialog::onFileNameChanged);
    }

    auto* buttonLayout = new QHBoxLayout();

    m_connectButton = new QPushButton(tr("Connect to Google Drive"), this);
    buttonLayout->addWidget(m_connectButton);

    m_upButton = new QPushButton(tr("Up"), this);
    m_upButton->setEnabled(false);
    buttonLayout->addWidget(m_upButton);

    m_newFolderButton = new QPushButton(tr("New Folder"), this);
    m_newFolderButton->setEnabled(false);
    buttonLayout->addWidget(m_newFolderButton);

    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setEnabled(false);
    buttonLayout->addWidget(m_refreshButton);

    buttonLayout->addStretch();

    m_actionButton = new QPushButton(mode == Save ? tr("Save here") : tr("Open"), this);
    m_actionButton->setEnabled(false);
    m_actionButton->setDefault(true);
    buttonLayout->addWidget(m_actionButton);

    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_connectButton, &QPushButton::clicked, m_service, &GoogleDriveService::authenticate);
    connect(m_upButton, &QPushButton::clicked, this, &GoogleDriveBrowserDialog::navigateToParent);
    connect(m_newFolderButton, &QPushButton::clicked, this, &GoogleDriveBrowserDialog::createNewFolder);
    connect(m_refreshButton, &QPushButton::clicked, this, &GoogleDriveBrowserDialog::refreshFiles);
    connect(m_actionButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_fileTable, &QTableWidget::itemSelectionChanged, this, &GoogleDriveBrowserDialog::onSelectionChanged);
    connect(m_fileTable, &QTableWidget::cellDoubleClicked, this, &GoogleDriveBrowserDialog::onCellDoubleClicked);

    connect(m_service, &GoogleDriveService::authStatusChanged, this, &GoogleDriveBrowserDialog::onAuthStatusChanged);
    connect(m_service, &GoogleDriveService::authFailed, this, &GoogleDriveBrowserDialog::onAuthFailed);
    connect(m_service, &GoogleDriveService::fileListReady, this, &GoogleDriveBrowserDialog::onFileListReady);
    connect(m_service, &GoogleDriveService::fileListFailed, this, &GoogleDriveBrowserDialog::onFileListFailed);
    connect(m_service, &GoogleDriveService::folderCreated, this, &GoogleDriveBrowserDialog::onFolderCreated);
    connect(m_service, &GoogleDriveService::folderCreateFailed, this, &GoogleDriveBrowserDialog::onFolderCreateFailed);

    bool auth = m_service->isAuthenticated();
    onAuthStatusChanged(auth);
    if (auth) {
        refreshFiles();
    }
}

// Getters

GoogleDriveBrowserDialog::DriveFile GoogleDriveBrowserDialog::selectedFile() const
{
    return m_selectedFile;
}

QString GoogleDriveBrowserDialog::selectedFolderId() const
{
    if (!m_selectedFolderId.isEmpty()) {
        return m_selectedFolderId;
    }
    return m_currentFolderId;
}

QString GoogleDriveBrowserDialog::saveFileName() const
{
    if (m_mode == Save && m_fileNameEdit) {
        QString name = m_fileNameEdit->text().trimmed();
        if (!name.endsWith(".kdbx")) {
            name += ".kdbx";
        }
        return name;
    }
    return {};
}

// Slots

void GoogleDriveBrowserDialog::refreshFiles()
{
    m_statusLabel->setText(tr("Loading..."));
    m_refreshButton->setEnabled(false);
    m_fileTable->setRowCount(0);
    m_selectedFile = {};
    m_selectedFolderId.clear();
    m_selectedFolderName.clear();
    m_actionButton->setEnabled(false);
    updateActionButton();
    m_service->listFiles(m_currentFolderId);
}

void GoogleDriveBrowserDialog::navigateToParent()
{
    if (m_parentFolderId.isEmpty()) {
        return;
    }
    m_currentFolderId = m_parentFolderId;
    m_parentFolderId.clear();
    m_selectedFolderId.clear();
    m_selectedFolderName.clear();
    m_pathLabel->setText(tr("My Drive"));
    m_upButton->setEnabled(false);
    updateActionButton();
    refreshFiles();
}

void GoogleDriveBrowserDialog::createNewFolder()
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

    m_statusLabel->setText(tr("Creating folder..."));
    m_newFolderButton->setEnabled(false);
    m_service->createFolder(m_currentFolderId, name.trimmed());
}

void GoogleDriveBrowserDialog::onFolderCreated(const QString& folderId, const QString& name)
{
    m_statusLabel->setText(tr("Folder \"%1\" created.").arg(name));
    m_newFolderButton->setEnabled(true);
    m_selectedFolderId = folderId;
    m_selectedFolderName = name;
    updateActionButton();
    refreshFiles();
}

void GoogleDriveBrowserDialog::onFolderCreateFailed(const QString& error)
{
    m_statusLabel->setText(tr("Failed to create folder: %1").arg(error));
    m_newFolderButton->setEnabled(true);
}

void GoogleDriveBrowserDialog::onFileListReady(const QList<GoogleDriveService::FileInfo>& files)
{
    m_fileTable->setRowCount(0);

    for (const auto& f : files) {
        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);

        // Show folders with a marker
        QString displayName = f.isFolder() ? QString::fromUtf8("\xF0\x9F\x93\x81 ") + f.name : f.name;
        m_fileTable->setItem(row, 0, new QTableWidgetItem(displayName));
        m_fileTable->setItem(row, 1, new QTableWidgetItem(
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
        m_fileTable->setItem(row, 2, new QTableWidgetItem(sizeStr));

        m_fileTable->item(row, 0)->setData(Qt::UserRole, f.id);
        m_fileTable->item(row, 0)->setData(Qt::UserRole + 1, f.isFolder());
        m_fileTable->item(row, 0)->setData(Qt::UserRole + 2, f.name);
    }

    if (files.isEmpty()) {
        m_statusLabel->setText(tr("Your Drive is empty. Click \"New Folder\" to create one."));
    } else {
        m_statusLabel->setText(tr("%1 item(s)").arg(files.size()));
    }
    m_refreshButton->setEnabled(true);
}

void GoogleDriveBrowserDialog::onFileListFailed(const QString& error)
{
    m_statusLabel->setText(tr("Error: %1").arg(error));
    m_refreshButton->setEnabled(true);
}

void GoogleDriveBrowserDialog::onAuthStatusChanged(bool authenticated)
{
    m_connectButton->setVisible(!authenticated);
    m_upButton->setEnabled(authenticated && !m_parentFolderId.isEmpty());
    m_newFolderButton->setEnabled(authenticated);
    m_refreshButton->setEnabled(authenticated);
    m_fileTable->setEnabled(authenticated);
    m_statusLabel->setText(authenticated
        ? tr("Connected.")
        : tr("Not connected to Google Drive."));
    if (!authenticated) {
        m_actionButton->setEnabled(false);
    }
}

void GoogleDriveBrowserDialog::onAuthFailed(const QString& error)
{
    m_statusLabel->setText(tr("Authentication failed: %1").arg(error));
    m_connectButton->setVisible(true);
}

void GoogleDriveBrowserDialog::onSelectionChanged()
{
    int row = m_fileTable->currentRow();
    if (row >= 0) {
        auto* item = m_fileTable->item(row, 0);
        if (item) {
            bool isFolder = item->data(Qt::UserRole + 1).toBool();

            if (m_mode == Open) {
                m_actionButton->setEnabled(!isFolder);
            } else {
                if (isFolder) {
                    // Selecting a folder → save into it
                    m_selectedFolderId = item->data(Qt::UserRole).toString();
                    m_selectedFolderName = item->data(Qt::UserRole + 2).toString();
                    m_pathLabel->setText(m_selectedFolderName);
                    m_actionButton->setText(tr("Save to \"%1\"").arg(m_selectedFolderName));
                } else {
                    // Selecting a file → fill the name
                    if (m_fileNameEdit) {
                        m_fileNameEdit->setText(item->data(Qt::UserRole + 2).toString());
                    }
                }
            }

            m_selectedFile.id = item->data(Qt::UserRole).toString();
            m_selectedFile.name = item->text();
            if (m_fileTable->item(row, 1)) {
                m_selectedFile.modifiedTime = m_fileTable->item(row, 1)->text();
            }
        }
    } else if (m_mode == Open) {
        m_actionButton->setEnabled(false);
        m_selectedFile = {};
    }

    updateActionButton();
}

void GoogleDriveBrowserDialog::onFileNameChanged(const QString& text)
{
    Q_UNUSED(text);
    updateActionButton();
}

void GoogleDriveBrowserDialog::updateActionButton()
{
    if (m_mode == Open) {
        return;
    }

    bool hasFolder = !m_selectedFolderId.isEmpty();
    bool hasName = m_fileNameEdit && !m_fileNameEdit->text().trimmed().isEmpty();

    if (hasFolder) {
        m_actionButton->setText(hasName
            ? tr("Save \"%1\" to \"%2\"").arg(m_fileNameEdit->text().trimmed(), m_selectedFolderName)
            : tr("Save to \"%1\"").arg(m_selectedFolderName));
    } else {
        m_actionButton->setText(tr("Save here"));
    }

    m_actionButton->setEnabled(hasFolder && hasName);
}

void GoogleDriveBrowserDialog::onCellDoubleClicked(int row, int /*column*/)
{
    auto* item = m_fileTable->item(row, 0);
    if (!item) {
        return;
    }

    bool isFolder = item->data(Qt::UserRole + 1).toBool();
    QString fileId = item->data(Qt::UserRole).toString();

    if (isFolder) {
        // Navigate into the folder
        m_parentFolderId = m_currentFolderId;
        m_currentFolderId = fileId;
        m_selectedFolderId.clear();
        m_selectedFolderName.clear();
        m_pathLabel->setText(item->data(Qt::UserRole + 2).toString());
        m_upButton->setEnabled(true);
        m_actionButton->setEnabled(false);
        m_selectedFile = {};
        updateActionButton();
        refreshFiles();
    } else {
        // Double-click file = accept
        if (m_mode == Save) {
            // Select the file and fill the name
            onSelectionChanged();
        }
        accept();
    }
}

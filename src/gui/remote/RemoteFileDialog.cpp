/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "RemoteFileDialog.h"
#include "ui_RemoteFileDialog.h"

#include "gui/Icons.h"
#include "RemoteHandler.h"
#include "RemoteSettings.h"

#include <QDesktopServices>

RemoteFileDialog::RemoteFileDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::RemoteFileDialog())
    , m_remoteHandler(new RemoteHandler(this))
{
    m_ui->setupUi(this);
    m_ui->messageWidget->setHidden(true);

    // setup status bar
    m_statusBar = new QStatusBar(this);
    m_statusBar->setFixedHeight(24);
    m_progressBarLabel = new QLabel(m_statusBar);
    m_progressBarLabel->setVisible(false);
    m_statusBar->addPermanentWidget(m_progressBarLabel);
    m_progressBar = new QProgressBar(m_statusBar);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    m_progressBar->setMaximumWidth(100);
    m_progressBar->setFixedHeight(15);
    m_progressBar->setMaximum(100);
    m_statusBar->addPermanentWidget(m_progressBar);
    m_ui->verticalLayout->addWidget(m_statusBar);

    m_ui->helpButton->setIcon(icons()->icon("system-help"));
    connect(m_ui->helpButton, &QToolButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl("https://keepassxc.org/docs/KeePassXC_UserGuide#_remote_database_support"));
    });

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &RemoteFileDialog::close);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &RemoteFileDialog::acceptRemoteProgramParams);

    connect(m_remoteHandler, &RemoteHandler::downloadFinished, this, &RemoteFileDialog::onDownloadFinished);
}

RemoteFileDialog::~RemoteFileDialog() = default;

void RemoteFileDialog::acceptRemoteProgramParams()
{
    if (m_ui->downloadCommand->text().isEmpty()) {
        m_ui->messageWidget->showMessage(tr("No download command specified."), MessageWidget::Error);
        return;
    }
    setInputDisabled(true);
    updateProgressBar(50, tr("Downloading..."));
    auto* remoteProgramParams = getRemoteParams();
    m_remoteHandler->download(remoteProgramParams);
}

void RemoteFileDialog::onDownloadFinished(RemoteHandler::RemoteResult result)
{
    if (result.success) {
        accept();
        emit downloadedSuccessfullyTo(result.filePath);
    } else {
        setInputDisabled(false);
        updateProgressBar(-1, "");
        m_ui->messageWidget->showMessage(result.errorMessage, MessageWidget::Error);
    }
}

void RemoteFileDialog::handleSuccessfulDownload(const QString& downloadedFileName)
{
    accept();
    emit downloadedSuccessfullyTo(downloadedFileName);
}

void RemoteFileDialog::showRemoteDownloadErrorMessage(const QString& errorMessage)
{
    setInputDisabled(false);
    updateProgressBar(-1, "");
    m_ui->messageWidget->showMessage(errorMessage, MessageWidget::Error);
}

void RemoteFileDialog::setInputDisabled(bool disabled)
{
    m_ui->downloadCommand->setDisabled(disabled);
    m_ui->inputForDownload->setDisabled(disabled);
}

RemoteParams* RemoteFileDialog::getRemoteParams()
{
    auto* params = new RemoteParams();
    params->downloadCommand = m_ui->downloadCommand->text();
    params->downloadInput = m_ui->inputForDownload->toPlainText();
    return params;
}

void RemoteFileDialog::updateProgressBar(int percentage, const QString& message)
{
    if (percentage < 0) {
        m_progressBar->setVisible(false);
        m_progressBarLabel->setVisible(false);
    } else {
        m_progressBar->setValue(percentage);
        m_progressBar->setVisible(true);
        m_progressBarLabel->setText(message);
        m_progressBarLabel->setVisible(true);
    }
}

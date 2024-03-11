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

#ifndef KEEPASSXC_REMOTEFILEDIALOG_H
#define KEEPASSXC_REMOTEFILEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPointer>
#include <QProgressBar>
#include <QStatusBar>

#include "RemoteHandler.h"

struct RemoteParams;

namespace Ui
{
    class RemoteFileDialog;
}

class RemoteFileDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RemoteFileDialog(QWidget* parent = nullptr);
    ~RemoteFileDialog() override;

signals:
    void downloadedSuccessfullyTo(const QString& filePath);

private slots:
    void acceptRemoteProgramParams();

private:
    void onDownloadFinished(RemoteHandler::RemoteResult result);
    void showRemoteDownloadErrorMessage(const QString& errorMessage);
    void handleSuccessfulDownload(const QString& downloadedFileName);

    void setInputDisabled(bool disabled);
    RemoteParams* getRemoteParams();

    void updateProgressBar(int percentage, const QString& message);

    QPointer<QStatusBar> m_statusBar;
    QPointer<QLabel> m_progressBarLabel;
    QPointer<QProgressBar> m_progressBar;

    QScopedPointer<Ui::RemoteFileDialog> m_ui;
    QPointer<RemoteHandler> m_remoteHandler;
};

#endif // KEEPASSXC_REMOTEFILEDIALOG_H

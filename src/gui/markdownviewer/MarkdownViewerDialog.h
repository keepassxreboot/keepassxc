/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_MARKDOWNVIEWERDIALOG_H
#define KEEPASSXC_MARKDOWNVIEWERDIALOG_H

#include "core/NetworkManager.h"
#include "MarkdownViewerPage.h"

#include <QDialog>
#include <QScopedPointer>
#include <QWebEnginePage>
#include <QWebChannel>

namespace Ui
{
    class MarkdownViewerDialog;
}

class MarkdownViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MarkdownViewerDialog(QUrl urlToMarkdownContent, QWidget* parent = nullptr);
    ~MarkdownViewerDialog();

private slots:
    void errorProcessingNetworkReply(QNetworkReply::NetworkError code);
    void finishedProcessingNetworkReply();
    void finishedLoadingContent(bool isLoadSuccessful);

private:
    QScopedPointer<Ui::MarkdownViewerDialog> m_ui;
    MarkdownViewerPage* m_page;
    QNetworkReply* m_reply;
};

#endif // KEEPASSXC_MARKDOWNVIEWERDIALOG_H

/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_ICONDOWNLOADERDIALOG_H
#define KEEPASSX_ICONDOWNLOADERDIALOG_H

#include <QDialog>
#include <QMap>
#include <QMutex>

class Database;
class Entry;
class CustomIconModel;
class IconDownloader;
class QStandardItemModel;

namespace Ui
{
    class IconDownloaderDialog;
}

class IconDownloaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IconDownloaderDialog(QWidget* parent = nullptr);
    ~IconDownloaderDialog() override;

    void downloadFavicons(const QSharedPointer<Database>& database, const QList<Entry*>& entries, bool force = false);
    void downloadFaviconInBackground(const QSharedPointer<Database>& database, Entry* entry);

private slots:
    void downloadFinished(const QString& url, const QImage& icon);
    void abortDownloads();

private:
    IconDownloader* createDownloader(const QString& url);

    void showFallbackMessage(bool state);
    void updateTable(const QString& url, const QString& message);
    void updateProgressBar();
    void updateCancelButton();

    QScopedPointer<Ui::IconDownloaderDialog> m_ui;
    QStandardItemModel* m_dataModel;
    QSharedPointer<Database> m_db;
    QMultiMap<QString, Entry*> m_urlToEntries;
    QList<IconDownloader*> m_activeDownloaders;
    QMutex m_mutex;

    Q_DISABLE_COPY(IconDownloaderDialog)
};

#endif // KEEPASSX_ICONDOWNLOADERDIALOG_H

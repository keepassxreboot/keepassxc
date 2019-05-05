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

#ifndef KEEPASSX_ICONDOWNLOADERDIALOG_H
#define KEEPASSX_ICONDOWNLOADERDIALOG_H

#include <QDialog>
#include <QUrl>
#include <QUuid>
#include <QScopedPointer>
#include <QStandardItemModel>
#include <QCloseEvent>
#include <QFutureSynchronizer>
#include <QProgressDialog>
#include <QMutex>

#include "config-keepassx.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "gui/MessageWidget.h"
#ifdef WITH_XC_NETWORKING
#include "gui/IconDownloader.h"
#endif

class Database;
class CustomIconModel;

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

    void raiseWindow();
    void downloadFavicon(const QSharedPointer<Database>& database, Entry* entry);
    void downloadFavicons(const QSharedPointer<Database>& database, const QList<Entry*>& entries);

signals:
    void messageEditEntry(QString, MessageWidget::MessageType);
    void entryUpdated();

private slots:
    void iconReceived(const QImage& icon, Entry* entry);
    bool addCustomIcon(const QImage& icon, Entry* entry);
    void fallbackNotEnabled();
    void iconError(Entry* entry);

protected:
    bool downloadAllFavicons(const QSharedPointer<Database>& database, const QList<Entry*>& entries);
    void updateTable(Entry* entry, const QString& message);
    void closeEvent(QCloseEvent* event) override;

private:
    QScopedPointer<Ui::IconDownloaderDialog> m_ui;
    QStandardItemModel* m_dataModel;
    QSharedPointer<Database> m_db;
    QUuid m_currentUuid;
    CustomIconModel* const m_customIconModel;
    QWidget* m_parent;
    QFutureSynchronizer<void> m_futureList;
    QMutex m_mutex;

    Q_DISABLE_COPY(IconDownloaderDialog)
};

#endif // KEEPASSX_ICONDOWNLOADERDIALOG_H

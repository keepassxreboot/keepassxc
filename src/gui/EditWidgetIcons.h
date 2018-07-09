/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_EDITWIDGETICONS_H
#define KEEPASSX_EDITWIDGETICONS_H

#include <QSet>
#include <QProgressDialog>
#include <QUrl>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QUuid>

#include "config-keepassx.h"
#include "core/Global.h"
#include "gui/MessageWidget.h"

class Database;
class DefaultIconModel;
class CustomIconModel;
#ifdef WITH_XC_NETWORKING
class QNetworkReply;
#endif

namespace Ui
{
    class EditWidgetIcons;
}

struct IconStruct
{
    IconStruct();

    QUuid uuid;
    int number;
};

class UrlFetchProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    explicit UrlFetchProgressDialog(const QUrl &url, QWidget *parent = nullptr);

public slots:
    void networkReplyProgress(qint64 bytesRead, qint64 totalBytes);
};

class EditWidgetIcons : public QWidget
{
    Q_OBJECT

public:
    explicit EditWidgetIcons(QWidget* parent = nullptr);
    ~EditWidgetIcons();

    IconStruct state();
    void reset();
    void load(const QUuid& currentUuid, Database* database, const IconStruct& iconStruct, const QString& url = "");

public slots:
    void setUrl(const QString& url);

signals:
    void messageEditEntry(QString, MessageWidget::MessageType);
    void messageEditEntryDismiss();
    void widgetUpdated();

private slots:
    void downloadFavicon();
    void startFetchFavicon(const QUrl& url);
    void fetchFinished();
    void fetchReadyRead();
    void fetchCanceled();
    void addCustomIconFromFile();
    bool addCustomIcon(const QImage& icon);
    void removeCustomIcon();
    void updateWidgetsDefaultIcons(bool checked);
    void updateWidgetsCustomIcons(bool checked);
    void updateRadioButtonDefaultIcons();
    void updateRadioButtonCustomIcons();

private:
    const QScopedPointer<Ui::EditWidgetIcons> m_ui;
    Database* m_database;
    QUuid m_currentUuid;
#ifdef WITH_XC_NETWORKING
    QUrl m_url;
    QUrl m_fetchUrl;
    QList<QUrl> m_urlsToTry;
    QByteArray m_bytesReceived;
    QNetworkAccessManager m_netMgr;
    QNetworkReply *m_reply;
    int m_redirects;
#endif
    DefaultIconModel* const m_defaultIconModel;
    CustomIconModel* const m_customIconModel;

    Q_DISABLE_COPY(EditWidgetIcons)
};

#endif // KEEPASSX_EDITWIDGETICONS_H

/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include <QWidget>
#include <QSet>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "core/Global.h"
#include "core/Uuid.h"
#include "gui/MessageWidget.h"

class Database;
class DefaultIconModel;
class CustomIconModel;

namespace Ui {
    class EditWidgetIcons;
}

struct IconStruct
{
    IconStruct();

    Uuid uuid;
    int number;
};

class EditWidgetIcons : public QWidget
{
    Q_OBJECT

public:
    explicit EditWidgetIcons(QWidget* parent = nullptr);
    ~EditWidgetIcons();

    IconStruct state();
    void reset();
    void load(Uuid currentUuid, Database* database, IconStruct iconStruct, const QString &url = QString());

public Q_SLOTS:
    void setUrl(const QString &url);

Q_SIGNALS:
    void messageEditEntry(QString, MessageWidget::MessageType);
    void messageEditEntryDismiss();

private Q_SLOTS:
    void downloadFavicon();
    void fetchFavicon(QUrl url);
    void fetchFaviconFromGoogle(QString domain);
    void abortFaviconDownload(bool clearRedirect = true);
    void onRequestFinished(QNetworkReply *reply);
    void addCustomIcon();
    void removeCustomIcon();
    void updateWidgetsDefaultIcons(bool checked);
    void updateWidgetsCustomIcons(bool checked);
    void updateRadioButtonDefaultIcons();
    void updateRadioButtonCustomIcons();

private:
    const QScopedPointer<Ui::EditWidgetIcons> m_ui;
    Database* m_database;
    Uuid m_currentUuid;
    QString m_url;
    QUrl m_redirectUrl;
    bool m_fallbackToGoogle = true;
    unsigned short m_redirectCount = 0;
    DefaultIconModel* const m_defaultIconModel;
    CustomIconModel* const m_customIconModel;
    QNetworkAccessManager* const m_networkAccessMngr;
    QNetworkReply* m_networkOperation;

    Q_DISABLE_COPY(EditWidgetIcons)
};

#endif // KEEPASSX_EDITWIDGETICONS_H

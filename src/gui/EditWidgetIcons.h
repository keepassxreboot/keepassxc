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

#include <QWidget>
#include <QSet>
#include <QUrl>

#include "config-keepassx.h"
#include "core/Global.h"
#include "core/Uuid.h"
#include "gui/MessageWidget.h"

class Database;
class DefaultIconModel;
class CustomIconModel;

#ifdef WITH_XC_NETWORKING
namespace qhttp {
    namespace client {
        class QHttpClient;
    }
}
#endif

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
    void load(const Uuid& currentUuid, Database* database, const IconStruct& iconStruct, const QString& url = "");

public slots:
    void setUrl(const QString& url);

signals:
    void messageEditEntry(QString, MessageWidget::MessageType);
    void messageEditEntryDismiss();

private slots:
    void downloadFavicon();
#ifdef WITH_XC_NETWORKING
    void fetchFavicon(const QUrl& url);
    void fetchFaviconFromGoogle(const QString& domain);
    void resetFaviconDownload(bool clearRedirect = true);
#endif
    void addCustomIconFromFile();
    void addCustomIcon(const QImage& icon);
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
    DefaultIconModel* const m_defaultIconModel;
    CustomIconModel* const m_customIconModel;
#ifdef WITH_XC_NETWORKING
    QUrl m_redirectUrl;
    bool m_fallbackToGoogle;
    unsigned short m_redirectCount;
    qhttp::client::QHttpClient* m_httpClient;
#endif

    Q_DISABLE_COPY(EditWidgetIcons)
};

#endif // KEEPASSX_EDITWIDGETICONS_H

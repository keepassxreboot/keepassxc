/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_KEESHARE_H
#define KEEPASSXC_KEESHARE_H

#include <QMap>
#include <QObject>

#include "gui/MessageWidget.h"
#include "keeshare/KeeShareSettings.h"

class Group;
class Database;
class ShareObserver;
class QXmlStreamWriter;
class QXmlStreamReader;

class KeeShare : public QObject
{
    Q_OBJECT
public:
    static KeeShare* instance();
    static void init(QObject* parent);

    static QString indicatorSuffix(const Group* group, const QString& text);
    static QPixmap indicatorBadge(const Group* group, QPixmap pixmap);

    static bool isShared(const Group* group);

    static KeeShareSettings::Own own();
    static KeeShareSettings::Active active();
    static KeeShareSettings::Foreign foreign();
    static void setForeign(const KeeShareSettings::Foreign& foreign);
    static void setActive(const KeeShareSettings::Active& active);
    static void setOwn(const KeeShareSettings::Own& own);

    static KeeShareSettings::Reference referenceOf(const Group* group);
    static void setReferenceTo(Group* group, const KeeShareSettings::Reference& reference);
    static QString referenceTypeLabel(const KeeShareSettings::Reference& reference);

    void connectDatabase(Database* newDb, Database* oldDb);
    void handleDatabaseOpened(Database* db);
    void handleDatabaseSaved(Database* db);

signals:
    void activeChanged();
    void sharingMessage(Database*, QString, MessageWidget::MessageType);

private slots:
    void emitSharingMessage(const QString&, MessageWidget::MessageType);
    void handleDatabaseDeleted(QObject*);
    void handleObserverDeleted(QObject*);
    void handleSettingsChanged(const QString&);

private:
    static KeeShare* m_instance;

    explicit KeeShare(QObject* parent);

    QMap<QObject*, QPointer<ShareObserver>> m_observersByDatabase;
    QMap<QObject*, QPointer<Database>> m_databasesByObserver;
};

#endif // KEEPASSXC_KEESHARE_H

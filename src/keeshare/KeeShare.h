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

#include <QFileInfo>
#include <QUuid>

#include "core/Config.h"
#include "gui/MessageWidget.h"

namespace KeeShareSettings
{
    struct Own;
    struct Active;
    struct Foreign;
    struct Reference;
} // namespace KeeShareSettings

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
    static bool isEnabled(const Group* group);

    static const Group* resolveSharedGroup(const Group* group);
    static QString sharingLabel(const Group* group);

    static KeeShareSettings::Own own();
    static KeeShareSettings::Active active();
    static KeeShareSettings::Foreign foreign();
    static void setForeign(const KeeShareSettings::Foreign& foreign);
    static void setActive(const KeeShareSettings::Active& active);
    static void setOwn(const KeeShareSettings::Own& own);

    static KeeShareSettings::Reference referenceOf(const Group* group);
    static void setReferenceTo(Group* group, const KeeShareSettings::Reference& reference);
    static QString referenceTypeLabel(const KeeShareSettings::Reference& reference);

    void connectDatabase(QSharedPointer<Database> newDb, QSharedPointer<Database> oldDb);

    static const QString signedContainerFileType();
    static const QString unsignedContainerFileType();
    static bool isContainerType(const QFileInfo& fileInfo, const QString type);

    static const QString signatureFileName();
    static const QString containerFileName();
signals:
    void activeChanged();
    void sharingMessage(QString, MessageWidget::MessageType);

private slots:
    void handleSettingsChanged(Config::ConfigKey key);

private:
    static KeeShare* m_instance;

    explicit KeeShare(QObject* parent);

    QMap<QUuid, QPointer<ShareObserver>> m_observersByDatabase;
};

#endif // KEEPASSXC_KEESHARE_H

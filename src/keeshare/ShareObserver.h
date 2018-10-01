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

#ifndef KEEPASSXC_SHAREOBSERVER_H
#define KEEPASSXC_SHAREOBSERVER_H

#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include "gui/MessageWidget.h"
#include "keeshare/KeeShareSettings.h"

class BulkFileWatcher;
class Entry;
class Group;
class CustomData;
class Database;

class ShareObserver : public QObject
{
    Q_OBJECT

public:
    explicit ShareObserver(Database* db, QObject* parent = nullptr);
    ~ShareObserver();

    void handleDatabaseSaved();
    void handleDatabaseOpened();

    const Database* database() const;
    Database* database();

signals:
    void sharingMessage(QString, MessageWidget::MessageType);

public slots:
    void handleDatabaseChanged();

private slots:
    void handleFileCreated(const QString& path);
    void handleFileChanged(const QString& path);
    void handleFileRemoved(const QString& path);

private:
    enum Change
    {
        Creation,
        Update,
        Deletion
    };

    struct Result
    {
        enum Type
        {
            Success,
            Info,
            Warning,
            Error
        };

        QString path;
        Type type;
        QString message;

        Result(const QString& path = QString(), Type type = Success, const QString& message = QString());

        bool isValid() const;
        bool isError() const;
        bool isWarning() const;
        bool isInfo() const;
    };

    static void resolveReferenceAttributes(Entry* targetEntry, const Database* sourceDb);

    static Database* exportIntoContainer(const KeeShareSettings::Reference& reference, const Group* sourceRoot);
    static Result importContainerInto(const KeeShareSettings::Reference& reference, Group* targetGroup);

    Result importFromReferenceContainer(const QString& path);
    QList<ShareObserver::Result> exportIntoReferenceContainers();
    void deinitialize();
    void reinitialize();
    void handleFileUpdated(const QString& path, Change change);
    void notifyAbout(const QStringList& success, const QStringList& warning, const QStringList& error);

private:
    Database* const m_db;
    QMap<KeeShareSettings::Reference, QPointer<Group>> m_referenceToGroup;
    QMap<QPointer<Group>, KeeShareSettings::Reference> m_groupToReference;
    QMap<QString, QPointer<Group>> m_shareToGroup;

    BulkFileWatcher* m_fileWatcher;
};

#endif // KEEPASSXC_SHAREOBSERVER_H

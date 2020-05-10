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
#include <QSharedPointer>
#include <QStringList>

#include "gui/MessageWidget.h"
#include "keeshare/KeeShareSettings.h"

class FileWatcher;
class Group;
class Database;

class ShareObserver : public QObject
{
    Q_OBJECT

public:
    explicit ShareObserver(QSharedPointer<Database> db, QObject* parent = nullptr);
    ~ShareObserver();

    QSharedPointer<Database> database();

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

signals:
    void sharingMessage(QString, MessageWidget::MessageType);

private slots:
    void handleDatabaseChanged();
    void handleDatabaseSaved();
    void handleFileUpdated(const QString& path);

private:
    Result importShare(const QString& path);
    QList<Result> exportShares();

    void deinitialize();
    void reinitialize();
    void notifyAbout(const QStringList& success, const QStringList& warning, const QStringList& error);

private:
    QSharedPointer<Database> m_db;
    QMap<QPointer<Group>, KeeShareSettings::Reference> m_groupToReference;
    QMap<QString, QPointer<Group>> m_shareToGroup;
    QMap<QString, QSharedPointer<FileWatcher>> m_fileWatchers;
};

#endif // KEEPASSXC_SHAREOBSERVER_H

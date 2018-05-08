/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
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

#ifndef BROWSERSERVICE_H
#define BROWSERSERVICE_H

#include "core/Entry.h"
#include "gui/DatabaseTabWidget.h"
#include <QObject>
#include <QtCore>

typedef QPair<QString, QString> StringPair;
typedef QList<StringPair> StringPairList;

enum
{
    max_length = 16 * 1024
};

class BrowserService : public QObject
{
    Q_OBJECT

public:
    explicit BrowserService(DatabaseTabWidget* parent);

    bool isDatabaseOpened() const;
    bool openDatabase(bool triggerUnlock);
    QString getDatabaseRootUuid();
    QString getDatabaseRecycleBinUuid();
    Entry* getConfigEntry(bool create = false);
    QString getKey(const QString& id);
    void addEntry(const QString& id,
                  const QString& login,
                  const QString& password,
                  const QString& url,
                  const QString& submitUrl,
                  const QString& realm);
    QList<Entry*> searchEntries(Database* db, const QString& hostname);
    QList<Entry*> searchEntries(const QString& text, const StringPairList& keyList);
    void removeSharedEncryptionKeys();
    void removeStoredPermissions();

public slots:
    QJsonArray findMatchingEntries(const QString& id,
                                   const QString& url,
                                   const QString& submitUrl,
                                   const QString& realm,
                                   const StringPairList& keyList);
    QString storeKey(const QString& key);
    void updateEntry(const QString& id,
                     const QString& uuid,
                     const QString& login,
                     const QString& password,
                     const QString& url);
    void databaseLocked(DatabaseWidget* dbWidget);
    void databaseUnlocked(DatabaseWidget* dbWidget);
    void activateDatabaseChanged(DatabaseWidget* dbWidget);
    void lockDatabase();

signals:
    void databaseLocked();
    void databaseUnlocked();
    void databaseChanged();

private:
    enum Access
    {
        Denied,
        Unknown,
        Allowed
    };

private:
    QList<Entry*> sortEntries(QList<Entry*>& pwEntries, const QString& host, const QString& submitUrl);
    bool confirmEntries(QList<Entry*>& pwEntriesToConfirm,
                        const QString& url,
                        const QString& host,
                        const QString& submitHost,
                        const QString& realm);
    QJsonObject prepareEntry(const Entry* entry);
    Access checkAccess(const Entry* entry, const QString& host, const QString& submitHost, const QString& realm);
    Group* findCreateAddEntryGroup();
    int
    sortPriority(const Entry* entry, const QString& host, const QString& submitUrl, const QString& baseSubmitUrl) const;
    bool matchUrlScheme(const QString& url);
    bool removeFirstDomain(QString& hostname);
    Database* getDatabase();

private:
    DatabaseTabWidget* const m_dbTabWidget;
    bool m_dialogActive;
    bool m_bringToFrontRequested;
};

#endif // BROWSERSERVICE_H

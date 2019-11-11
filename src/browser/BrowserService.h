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
    enum ReturnValue
    {
        Success,
        Error,
        Canceled
    };

    explicit BrowserService(DatabaseTabWidget* parent);

    bool isDatabaseOpened() const;
    bool openDatabase(bool triggerUnlock);
    QString getDatabaseRootUuid();
    QString getDatabaseRecycleBinUuid();
    QJsonObject getDatabaseGroups(const QSharedPointer<Database>& selectedDb = {});
    QJsonObject createNewGroup(const QString& groupName);
    QString getKey(const QString& id);
    void addEntry(const QString& id,
                  const QString& login,
                  const QString& password,
                  const QString& url,
                  const QString& submitUrl,
                  const QString& realm,
                  const QString& group,
                  const QString& groupUuid,
                  const QSharedPointer<Database>& selectedDb = {});
    QList<Entry*> searchEntries(const QSharedPointer<Database>& db, const QString& hostname, const QString& url);
    QList<Entry*> searchEntries(const QString& url, const StringPairList& keyList);
    void convertAttributesToCustomData(const QSharedPointer<Database>& currentDb = {});

public:
    static const QString KEEPASSXCBROWSER_NAME;
    static const QString KEEPASSXCBROWSER_OLD_NAME;
    static const QString ASSOCIATE_KEY_PREFIX;
    static const QString LEGACY_ASSOCIATE_KEY_PREFIX;
    static const QString OPTION_SKIP_AUTO_SUBMIT;
    static const QString OPTION_HIDE_ENTRY;
    static const QString ADDITIONAL_URL;

public slots:
    QJsonArray findMatchingEntries(const QString& id,
                                   const QString& url,
                                   const QString& submitUrl,
                                   const QString& realm,
                                   const StringPairList& keyList,
                                   const bool httpAuth = false);
    QString storeKey(const QString& key);
    ReturnValue updateEntry(const QString& id,
                            const QString& uuid,
                            const QString& login,
                            const QString& password,
                            const QString& url,
                            const QString& submitUrl);
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

    enum WindowState
    {
        Normal,
        Minimized,
        Hidden
    };

private:
    QList<Entry*> sortEntries(QList<Entry*>& pwEntries, const QString& host, const QString& submitUrl);
    bool confirmEntries(QList<Entry*>& pwEntriesToConfirm,
                        const QString& url,
                        const QString& host,
                        const QString& submitUrl,
                        const QString& realm,
                        const bool httpAuth);
    QJsonObject prepareEntry(const Entry* entry);
    Access checkAccess(const Entry* entry, const QString& host, const QString& submitHost, const QString& realm);
    Group* getDefaultEntryGroup(const QSharedPointer<Database>& selectedDb = {});
    int
    sortPriority(const Entry* entry, const QString& host, const QString& submitUrl, const QString& baseSubmitUrl) const;
    bool schemeFound(const QString& url);
    bool removeFirstDomain(QString& hostname);
    bool handleURL(const QString& entryUrl, const QString& hostname, const QString& url);
    QString baseDomain(const QString& hostname) const;
    QSharedPointer<Database> getDatabase();
    QSharedPointer<Database> selectedDatabase();
    QJsonArray getChildrenFromGroup(Group* group);
    bool moveSettingsToCustomData(Entry* entry, const QString& name) const;
    int moveKeysToCustomData(Entry* entry, const QSharedPointer<Database>& db) const;
    bool checkLegacySettings();
    void hideWindow() const;
    void raiseWindow(const bool force = false);

private:
    DatabaseTabWidget* const m_dbTabWidget;
    bool m_dialogActive;
    bool m_bringToFrontRequested;
    WindowState m_prevWindowState;
    QUuid m_keepassBrowserUUID;

    friend class TestBrowser;
};

#endif // BROWSERSERVICE_H

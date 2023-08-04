/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
 *  Copyright (C) 2013 Francois Ferrand
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

#include "BrowserAccessControlDialog.h"
#include "core/Entry.h"
#include "gui/PasswordGeneratorWidget.h"

class QLocalSocket;

typedef QPair<QString, QString> StringPair;
typedef QList<StringPair> StringPairList;

enum
{
    max_length = 16 * 1024
};

struct KeyPairMessage
{
    QLocalSocket* socket;
    QString nonce;
    QString publicKey;
    QString secretKey;
};

struct EntryParameters
{
    QString dbid;
    QString login;
    QString password;
    QString realm;
    QString hash;
    QString siteUrl;
    QString formUrl;
    bool httpAuth;
};

class DatabaseWidget;
class BrowserHost;
class BrowserAction;

class BrowserService : public QObject
{
    Q_OBJECT

public:
    explicit BrowserService();
    static BrowserService* instance();

    void setEnabled(bool enabled);

    QString getKey(const QString& id);
    QString storeKey(const QString& key);
    QString getDatabaseHash(bool legacy = false);

    bool isDatabaseOpened() const;
    bool openDatabase(bool triggerUnlock);
    void lockDatabase();

    QJsonObject getDatabaseGroups();
    QJsonArray getDatabaseEntries(bool* accessDenied, const QSharedPointer<Database>& selectedDb = {});
    QJsonObject createNewGroup(const QString& groupName);
    QString getCurrentTotp(const QString& uuid);
    void showPasswordGenerator(const KeyPairMessage& keyPairMessage);
    bool isPasswordGeneratorRequested() const;

    bool getAlwaysAllowAccess();
    void setAlwaysAllowAccess(bool enabled);
    bool getAllowGetDatabaseEntriesRequest();
    void setAllowGetDatabaseEntriesRequest(bool enabled);

    void addEntry(const EntryParameters& entryParameters,
                  const QString& group,
                  const QString& groupUuid,
                  const bool downloadFavicon,
                  const QSharedPointer<Database>& selectedDb = {});
    bool updateEntry(const EntryParameters& entryParameters, const QString& uuid);
    bool deleteEntry(const QString& uuid);
    QJsonArray findEntries(const EntryParameters& entryParameters, const StringPairList& keyList, bool* entriesFound);
    void requestGlobalAutoType(const QString& search);

    static const QString KEEPASSXCBROWSER_NAME;
    static const QString KEEPASSXCBROWSER_OLD_NAME;
    static const QString OPTION_SKIP_AUTO_SUBMIT;
    static const QString OPTION_HIDE_ENTRY;
    static const QString OPTION_ONLY_HTTP_AUTH;
    static const QString OPTION_NOT_HTTP_AUTH;
    static const QString OPTION_OMIT_WWW;
    static const QString ADDITIONAL_URL;

signals:
    void requestUnlock();
    void passwordGenerated(QLocalSocket* socket, const QString& password, const QString& nonce);

public slots:
    void databaseLocked(DatabaseWidget* dbWidget);
    void databaseUnlocked(DatabaseWidget* dbWidget);
    void activeDatabaseChanged(DatabaseWidget* dbWidget);

private slots:
    void processClientMessage(QLocalSocket* socket, const QJsonObject& message);

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

    QList<Entry*> searchEntries(const QSharedPointer<Database>& db, const QString& siteUrl, const QString& formUrl);
    QList<Entry*> searchEntries(const QString& siteUrl, const QString& formUrl, const StringPairList& keyList);
    QList<Entry*> sortEntries(QList<Entry*>& entries, const QString& siteUrl, const QString& formUrl);
    QList<Entry*> confirmEntries(QList<Entry*>& entriesToConfirm,
                                 const EntryParameters& entryParameters,
                                 const QString& siteHost,
                                 const QString& formUrl,
                                 const bool httpAuth);
    QJsonObject prepareEntry(const Entry* entry);
    void allowEntry(Entry* entry, const QString& siteHost, const QString& formUrl, const QString& realm);
    void denyEntry(Entry* entry, const QString& siteHost, const QString& formUrl, const QString& realm);
    QJsonArray getChildrenFromGroup(Group* group);
    Access checkAccess(const Entry* entry, const QString& siteHost, const QString& formHost, const QString& realm);
    Group* getDefaultEntryGroup(const QSharedPointer<Database>& selectedDb = {});
    int sortPriority(const QStringList& urls, const QString& siteUrl, const QString& formUrl);
    bool schemeFound(const QString& url);
    bool isIpAddress(const QString& host) const;
    bool removeFirstDomain(QString& hostname);
    bool
    shouldIncludeEntry(Entry* entry, const QString& url, const QString& submitUrl, const bool omitWwwSubdomain = false);
    bool handleURL(const QString& entryUrl,
                   const QString& siteUrl,
                   const QString& formUrl,
                   const bool omitWwwSubdomain = false);
    QString getTopLevelDomainFromUrl(const QString& url) const;
    QString baseDomain(const QString& hostname) const;
    QSharedPointer<Database> getDatabase();
    QSharedPointer<Database> selectedDatabase();
    QString getDatabaseRootUuid();
    QString getDatabaseRecycleBinUuid();
    bool checkLegacySettings(QSharedPointer<Database> db);
    void hideWindow() const;
    void raiseWindow(const bool force = false);
    void updateWindowState();

    QPointer<BrowserHost> m_browserHost;
    QHash<QString, QSharedPointer<BrowserAction>> m_browserClients;

    bool m_dialogActive;
    bool m_bringToFrontRequested;
    bool m_passwordGeneratorRequested;
    WindowState m_prevWindowState;
    QUuid m_keepassBrowserUUID;

    QPointer<DatabaseWidget> m_currentDatabaseWidget;
    QScopedPointer<PasswordGeneratorWidget> m_passwordGenerator;

    Q_DISABLE_COPY(BrowserService);

    friend class TestBrowser;
};

static inline BrowserService* browserService()
{
    return BrowserService::instance();
}

#endif // BROWSERSERVICE_H

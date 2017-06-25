/*
*  Copyright (C) 2013 Francois Ferrand
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

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include "gui/DatabaseTabWidget.h"
#include "Server.h"

class Service : public KeepassHttpProtocol::Server
{
    Q_OBJECT

public:
    explicit Service(DatabaseTabWidget* parent = 0);

    virtual bool isDatabaseOpened() const;
    virtual bool openDatabase();
    virtual QString getDatabaseRootUuid();
    virtual QString getDatabaseRecycleBinUuid();
    virtual QString getKey(const QString& id);
    virtual QString storeKey(const QString& key);
    virtual QList<KeepassHttpProtocol::Entry> findMatchingEntries(const QString& id, const QString& url, const QString&  submitUrl, const QString&  realm);
    virtual int countMatchingEntries(const QString& id, const QString& url, const QString&  submitUrl, const QString&  realm);
    virtual QList<KeepassHttpProtocol::Entry> searchAllEntries(const QString& id);
    virtual void addEntry(const QString& id, const QString& login, const QString& password, const QString& url, const QString& submitUrl, const QString& realm);
    virtual void updateEntry(const QString& id, const QString& uuid, const QString& login, const QString& password, const QString& url);
    virtual QString generatePassword();

public slots:
    void removeSharedEncryptionKeys();
    void removeStoredPermissions();

private:
    enum Access { Denied, Unknown, Allowed};
    Entry* getConfigEntry(bool create = false);
    bool matchUrlScheme(const QString& url);
    Access checkAccess(const Entry* entry, const QString&  host, const QString&  submitHost, const QString&  realm);
    bool removeFirstDomain(QString& hostname);
    Group *findCreateAddEntryGroup();
    class SortEntries;
    int sortPriority(const Entry *entry, const QString &host, const QString &submitUrl, const QString &baseSubmitUrl) const;
    KeepassHttpProtocol::Entry prepareEntry(const Entry* entry);
    QList<Entry*> searchEntries(Database* db, const QString& hostname);
    QList<Entry*> searchEntries(const QString& text);

    DatabaseTabWidget * const m_dbTabWidget;
};

#endif // SERVICE_H

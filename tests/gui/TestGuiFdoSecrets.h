/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_TESTGUIFDOSECRETS_H
#define KEEPASSXC_TESTGUIFDOSECRETS_H

#include <QByteArray>
#include <QDBusConnection>
#include <QPointer>

#include "fdosecrets/dbus/DBusTypes.h"

class MainWindow;
class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class TemporaryFile;
class FdoSecretsPlugin;
namespace FdoSecrets
{
    class Service;
    class Session;
    class Collection;
    class Item;
    class Prompt;
    class DhIetf1024Sha256Aes128CbcPkcs7;
    class DBusClient;
} // namespace FdoSecrets
class ServiceProxy;
class CollectionProxy;
class ItemProxy;
class SessionProxy;
class PromptProxy;

class QAbstractItemView;

class TestGuiFdoSecrets : public QObject
{
    Q_OBJECT

public:
    ~TestGuiFdoSecrets() override;

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testServiceEnable();
    void testServiceEnableNoExposedDatabase();
    void testServiceSearch();
    void testServiceUnlock();
    void testServiceUnlockItems();
    void testServiceLock();

    void testSessionOpen();
    void testSessionClose();

    void testCollectionCreate();
    void testCollectionDelete();
    void testCollectionChange();

    void testItemCreate();
    void testItemChange();
    void testItemReplace();
    void testItemReplaceExistingLocked();
    void testItemSecret();
    void testItemDelete();
    void testItemLockState();

    void testAlias();
    void testDefaultAliasAlwaysPresent();

    void testExposeSubgroup();
    void testModifyingExposedGroup();

    void testHiddenFilename();
    void testDuplicateName();

protected slots:
    void driveNewDatabaseWizard();
    bool driveAccessControlDialog(bool remember = true);

private:
    void lockDatabaseInBackend();
    void unlockDatabaseInBackend();
    QSharedPointer<ServiceProxy> enableService();
    QSharedPointer<SessionProxy> openSession(const QSharedPointer<ServiceProxy>& service, const QString& algo);
    QSharedPointer<CollectionProxy> getDefaultCollection(const QSharedPointer<ServiceProxy>& service);
    QSharedPointer<ItemProxy> getFirstItem(const QSharedPointer<CollectionProxy>& coll);
    QSharedPointer<ItemProxy> createItem(const QSharedPointer<SessionProxy>& sess,
                                         const QSharedPointer<CollectionProxy>& coll,
                                         const QString& label,
                                         const QString& pass,
                                         const FdoSecrets::wire::StringStringMap& attr,
                                         bool replace,
                                         bool expectPrompt = false);
    template <typename Proxy> QSharedPointer<Proxy> getProxy(const QDBusObjectPath& path) const
    {
        auto ret = QSharedPointer<Proxy>{
            new Proxy(QStringLiteral("org.freedesktop.secrets"), path.path(), QDBusConnection::sessionBus())};
        if (!ret->isValid()) {
            return {};
        }
        return ret;
    }

    template <typename T> T getSignalVariantArgument(const QVariant& arg)
    {
        const auto& in = arg.value<QDBusVariant>().variant();
        return qdbus_cast<T>(in);
    }

private:
    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;

    QPointer<FdoSecretsPlugin> m_plugin;
    QSharedPointer<FdoSecrets::DBusClient> m_client;

    QScopedPointer<FdoSecrets::DhIetf1024Sha256Aes128CbcPkcs7> m_clientCipher;

    QByteArray m_dbData;
    QScopedPointer<TemporaryFile> m_dbFile;
};

#endif // KEEPASSXC_TESTGUIFDOSECRETS_H

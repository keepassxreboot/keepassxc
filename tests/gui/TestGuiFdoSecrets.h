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
#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>

#include "fdosecrets/GcryptMPI.h"
#include "fdosecrets/objects/DBusTypes.h"

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
} // namespace FdoSecrets

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

    void testDBusSpec();

    void testServiceEnable();
    void testServiceEnableNoExposedDatabase();
    void testServiceSearch();
    void testServiceUnlock();
    void testServiceLock();

    void testSessionOpen();
    void testSessionClose();

    void testCollectionCreate();
    void testCollectionDelete();

    void testItemCreate();
    void testItemReplace();
    void testItemSecret();
    void testItemDelete();

    void testAlias();
    void testDefaultAliasAlwaysPresent();

    void testExposeSubgroup();
    void testModifyingExposedGroup();

    void testHiddenFilename();
    void testDuplicateName();

protected slots:
    void createDatabaseCallback();

private:
    void lockDatabaseInBackend();
    void unlockDatabaseInBackend();
    QPointer<FdoSecrets::Service> enableService();
    QPointer<FdoSecrets::Session> openSession(FdoSecrets::Service* service, const QString& algo);
    QPointer<FdoSecrets::Collection> getDefaultCollection(FdoSecrets::Service* service);
    QPointer<FdoSecrets::Item> getFirstItem(FdoSecrets::Collection* coll);
    QPointer<FdoSecrets::Item> createItem(FdoSecrets::Session* sess,
                                          FdoSecrets::Collection* coll,
                                          const QString& label,
                                          const QString& pass,
                                          const StringStringMap& attr,
                                          bool replace);

private:
    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;

    QPointer<FdoSecretsPlugin> m_plugin;

    // For DH session tests
    GcryptMPI m_serverPrivate;
    GcryptMPI m_serverPublic;
    std::unique_ptr<FdoSecrets::DhIetf1024Sha256Aes128CbcPkcs7> m_cipher;

    QByteArray m_dbData;
    QScopedPointer<TemporaryFile> m_dbFile;
};

#endif // KEEPASSXC_TESTGUIFDOSECRETS_H

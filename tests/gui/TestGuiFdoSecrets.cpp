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

#include "TestGuiFdoSecrets.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/SessionCipher.h"
#include "fdosecrets/widgets/AccessControlDialog.h"

#include "config-keepassx-tests.h"

#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/wizard/NewDatabaseWizard.h"
#include "util/FdoSecretsProxy.h"
#include "util/TemporaryFile.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>
#include <utility>

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setApplicationVersion(KEEPASSXC_VERSION);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    app.applyTheme();
    QTEST_DISABLE_KEYPAD_NAVIGATION
    TestGuiFdoSecrets tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#define DBUS_PATH_DEFAULT_ALIAS "/org/freedesktop/secrets/aliases/default"

// assert macros compatible with function having return values
#define VERIFY2_RET(statement, msg)                                                                                    \
    do {                                                                                                               \
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, (msg), __FILE__, __LINE__))                      \
            return {};                                                                                                 \
    } while (false)

#define COMPARE_RET(actual, expected)                                                                                  \
    do {                                                                                                               \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))                                \
            return {};                                                                                                 \
    } while (false)

// by default use these with Qt macros
#define VERIFY QVERIFY
#define COMPARE QCOMPARE
#define VERIFY2 QVERIFY2

#define DBUS_COMPARE(actual, expected)                                                                                 \
    do {                                                                                                               \
        auto reply = (actual);                                                                                         \
        VERIFY2(reply.isValid(), reply.error().name().toLocal8Bit());                                                  \
        COMPARE(reply.value(), (expected));                                                                            \
    } while (false)

#define DBUS_VERIFY(stmt)                                                                                              \
    do {                                                                                                               \
        auto reply = (stmt);                                                                                           \
        VERIFY2(reply.isValid(), reply.error().name().toLocal8Bit());                                                  \
    } while (false)

#define DBUS_GET(var, stmt)                                                                                            \
    std::remove_cv<decltype((stmt).argumentAt<0>())>::type var;                                                        \
    do {                                                                                                               \
        const auto rep = (stmt);                                                                                       \
        VERIFY2(rep.isValid(), rep.error().name().toLocal8Bit());                                                      \
        var = rep.argumentAt<0>();                                                                                     \
    } while (false)

#define DBUS_GET2(name1, name2, stmt)                                                                                  \
    std::remove_cv<decltype((stmt).argumentAt<0>())>::type name1;                                                      \
    std::remove_cv<decltype((stmt).argumentAt<1>())>::type name2;                                                      \
    do {                                                                                                               \
        const auto rep = (stmt);                                                                                       \
        VERIFY2(rep.isValid(), rep.error().name().toLocal8Bit());                                                      \
        name1 = rep.argumentAt<0>();                                                                                   \
        name2 = rep.argumentAt<1>();                                                                                   \
    } while (false)

using namespace FdoSecrets;

class FakeClient : public DBusClient
{
public:
    explicit FakeClient(DBusMgr* dbus)
        : DBusClient(
            dbus,
            {QStringLiteral("local"), 0, true, {ProcInfo{0, 0, QStringLiteral("fake-client"), QString{}, QString{}}}})
    {
    }
};

// pretty print QDBusObjectPath in QCOMPARE
char* toString(const QDBusObjectPath& path)
{
    return QTest::toString("ObjectPath(" + path.path() + ")");
}

TestGuiFdoSecrets::~TestGuiFdoSecrets() = default;

void TestGuiFdoSecrets::initTestCase()
{
    VERIFY(Crypto::init());
    Config::createTempFileInstance();
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    config()->set(Config::GUI_ShowTrayIcon, true);
    config()->set(Config::UpdateCheckMessageShown, true);
    // Disable quick unlock
    config()->set(Config::Security_QuickUnlock, false);
    // Disable secret service integration (activate within individual tests to test the plugin)
    FdoSecrets::settings()->setEnabled(false);
    // activate within individual tests
    FdoSecrets::settings()->setShowNotification(false);

    Application::bootstrap();

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    VERIFY(m_tabWidget);
    m_plugin = FdoSecretsPlugin::getPlugin();
    VERIFY(m_plugin);
    m_mainWindow->show();

    auto key = QByteArray::fromHex("e407997e8b918419cf851cf3345358fdf"
                                   "ffb9564a220ac9c3934efd277cea20d17"
                                   "467ecdc56e817f75ac39501f38a4a04ff"
                                   "64d627e16c09981c7ad876da255b61c8e"
                                   "6a8408236c2a4523cfe6961c26dbdfc77"
                                   "c1a27a5b425ca71a019e829fae32c0b42"
                                   "0e1b3096b48bc2ce9ccab1d1ff13a5eb4"
                                   "b263cee30bdb1a57af9bfa93f");
    m_clientCipher.reset(new DhIetf1024Sha256Aes128CbcPkcs7(key));

    // Load the NewDatabase.kdbx file into temporary storage
    QFile sourceDbFile(QStringLiteral(KEEPASSX_TEST_DATA_DIR "/NewDatabase.kdbx"));
    VERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    VERIFY(Tools::readAllFromDevice(&sourceDbFile, m_dbData));
    sourceDbFile.close();

    // set a fake dbus client all the time so we can freely access DBusMgr anywhere
    m_client.reset(new FakeClient(m_plugin->dbus().data()));
    m_plugin->dbus()->overrideClient(m_client);
}

// Every test starts with opening the temp database
void TestGuiFdoSecrets::init()
{
    m_dbFile.reset(new TemporaryFile());
    // Write the temp storage to a temp database file for use in our tests
    VERIFY(m_dbFile->open());
    COMPARE(m_dbFile->write(m_dbData), static_cast<qint64>(m_dbData.size()));
    m_dbFile->close();

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    processEvents();

    // open and unlock the database
    m_tabWidget->addDatabaseTab(m_dbFile->fileName(), false, "a");
    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();

    // by default expose the root group
    FdoSecrets::settings()->setExposedGroup(m_db, m_db->rootGroup()->uuid());
    VERIFY(m_dbWidget->save());

    // enforce consistent default settings at the beginning
    FdoSecrets::settings()->setUnlockBeforeSearch(false);
    FdoSecrets::settings()->setShowNotification(false);
    FdoSecrets::settings()->setConfirmAccessItem(false);
    FdoSecrets::settings()->setEnabled(false);
}

// Every test ends with closing the temp database without saving
void TestGuiFdoSecrets::cleanup()
{
    // restore to default settings
    FdoSecrets::settings()->setUnlockBeforeSearch(false);
    FdoSecrets::settings()->setShowNotification(false);
    FdoSecrets::settings()->setConfirmAccessItem(false);
    FdoSecrets::settings()->setEnabled(false);
    if (m_plugin) {
        m_plugin->updateServiceState();
    }

    // DO NOT save the database
    for (int i = 0; i != m_tabWidget->count(); ++i) {
        m_tabWidget->databaseWidgetFromIndex(i)->database()->markAsClean();
    }

    // Close any dialogs
    while (auto w = QApplication::activeModalWidget()) {
        w->close();
    }

    VERIFY(m_tabWidget->closeAllDatabaseTabs());
    processEvents();

    if (m_dbFile) {
        m_dbFile->remove();
    }

    m_client->clearAuthorization();
}

void TestGuiFdoSecrets::cleanupTestCase()
{
    m_plugin->dbus()->overrideClient({});
    if (m_dbFile) {
        m_dbFile->remove();
    }
}

void TestGuiFdoSecrets::testServiceEnable()
{
    QSignalSpy sigError(m_plugin, SIGNAL(error(QString)));
    VERIFY(sigError.isValid());

    QSignalSpy sigStarted(m_plugin, SIGNAL(secretServiceStarted()));
    VERIFY(sigStarted.isValid());

    // make sure no one else is holding the service
    VERIFY(!QDBusConnection::sessionBus().interface()->isServiceRegistered(DBUS_SERVICE_SECRET));

    // enable the service
    auto service = enableService();
    VERIFY(service);

    // service started without error
    VERIFY(sigError.isEmpty());
    COMPARE(sigStarted.size(), 1);

    processEvents();

    VERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(DBUS_SERVICE_SECRET));

    // there will be one default collection
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    DBUS_COMPARE(coll->locked(), false);
    DBUS_COMPARE(coll->label(), m_db->metadata()->name());

    DBUS_COMPARE(coll->created(),
                 static_cast<qulonglong>(m_db->rootGroup()->timeInfo().creationTime().toMSecsSinceEpoch() / 1000));
    DBUS_COMPARE(
        coll->modified(),
        static_cast<qulonglong>(m_db->rootGroup()->timeInfo().lastModificationTime().toMSecsSinceEpoch() / 1000));
}

void TestGuiFdoSecrets::testServiceEnableNoExposedDatabase()
{
    // reset the exposed group and then enable the service
    FdoSecrets::settings()->setExposedGroup(m_db, {});
    auto service = enableService();
    VERIFY(service);

    // no collections
    DBUS_COMPARE(service->collections(), QList<QDBusObjectPath>{});
}

void TestGuiFdoSecrets::testServiceSearch()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);

    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    auto entry = itemObj->backend();
    VERIFY(entry);

    entry->attributes()->set("fdosecrets-test", "1");
    entry->attributes()->set("fdosecrets-test-protected", "2", true);
    const QString crazyKey = "_a:bc&-+'-e%12df_d";
    const QString crazyValue = "[v]al@-ue";
    entry->attributes()->set(crazyKey, crazyValue);

    // search by title
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", entry->title()}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {QDBusObjectPath(item->path())});
    }

    // search by attribute
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"fdosecrets-test", "1"}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {QDBusObjectPath(item->path())});
    }
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{crazyKey, crazyValue}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {QDBusObjectPath(item->path())});
    }

    // searching using empty terms returns nothing
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({}));
        COMPARE(locked, {});
        COMPARE(unlocked, {});
    }

    // searching using protected attributes or password returns nothing
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"Password", entry->password()}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {});
    }
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"fdosecrets-test-protected", "2"}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {});
    }
}

void TestGuiFdoSecrets::testServiceSearchBlockingUnlock()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    auto entries = m_db->rootGroup()->entriesRecursive();
    VERIFY(!entries.isEmpty());
    // assumes the db is not empty
    auto title = entries.first()->title();

    // NOTE: entries are no longer valid after locking
    lockDatabaseInBackend();

    // when database is locked, nothing is returned
    FdoSecrets::settings()->setUnlockBeforeSearch(false);
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", title}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {});
    }

    // when database is locked, nothing is returned
    FdoSecrets::settings()->setUnlockBeforeSearch(true);
    {
        // SearchItems will block because the blocking wait is implemented
        // using a local QEventLoop.
        // so we do a little trick here to get the return value back
        bool unlockDialogWorks = false;
        QTimer::singleShot(50, [&]() { unlockDialogWorks = driveUnlockDialog(); });

        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", title}}));
        VERIFY(unlockDialogWorks);
        COMPARE(locked, {});
        COMPARE(unlocked.size(), 1);
        auto item = getProxy<ItemProxy>(unlocked.first());
        DBUS_COMPARE(item->label(), title);
    }
}

void TestGuiFdoSecrets::testServiceSearchBlockingUnlockMultiple()
{
    // setup: two databases, both locked, one with exposed db, the other not.

    // add another database tab with a database with no exposed group
    // to avoid modify the original, copy to a temp file first
    QFile sourceDbFile(QStringLiteral(KEEPASSX_TEST_DATA_DIR "/NewDatabase2.kdbx"));
    QByteArray dbData;
    VERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    VERIFY(Tools::readAllFromDevice(&sourceDbFile, dbData));
    sourceDbFile.close();

    QTemporaryFile anotherFile;
    VERIFY(anotherFile.open());
    COMPARE(anotherFile.write(dbData), static_cast<qint64>(dbData.size()));
    anotherFile.close();

    m_tabWidget->addDatabaseTab(anotherFile.fileName(), false);
    auto anotherWidget = m_tabWidget->currentDatabaseWidget();

    auto service = enableService();
    VERIFY(service);

    // when there are multiple locked databases,
    // repeatedly show the dialog until there is at least one unlocked collection
    FdoSecrets::settings()->setUnlockBeforeSearch(true);

    // when only unlocking the one with no exposed group, a second dialog is shown
    lockDatabaseInBackend();
    {
        bool unlockDialogWorks = false;
        QTimer::singleShot(50, [&]() {
            unlockDialogWorks = driveUnlockDialog(anotherWidget);
            QTimer::singleShot(50, [&]() { unlockDialogWorks &= driveUnlockDialog(); });
        });

        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", "Sample Entry"}}));
        VERIFY(unlockDialogWorks);
        COMPARE(locked, {});
        COMPARE(unlocked.size(), 1);
    }

    // when unlocking the one with exposed group, the other one remains locked
    lockDatabaseInBackend();
    {
        bool unlockDialogWorks = false;
        QTimer::singleShot(50, [&]() { unlockDialogWorks = driveUnlockDialog(m_dbWidget); });

        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", "Sample Entry"}}));
        VERIFY(unlockDialogWorks);
        COMPARE(locked, {});
        COMPARE(unlocked.size(), 1);
        VERIFY(anotherWidget->isLocked());
    }
}

void TestGuiFdoSecrets::testServiceSearchForce()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);

    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    auto entry = itemObj->backend();
    VERIFY(entry);

    // fdosecrets should still find the item even if searching is disabled
    entry->group()->setSearchingEnabled(Group::Disable);

    // search by title
    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", entry->title()}}));
        COMPARE(locked, {});
        COMPARE(unlocked, {QDBusObjectPath(item->path())});
    }
}

void TestGuiFdoSecrets::testServiceUnlock()
{
    lockDatabaseInBackend();

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    QSignalSpy spyCollectionCreated(service.data(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    VERIFY(spyCollectionCreated.isValid());
    QSignalSpy spyCollectionDeleted(service.data(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    VERIFY(spyCollectionDeleted.isValid());
    QSignalSpy spyCollectionChanged(service.data(), SIGNAL(CollectionChanged(QDBusObjectPath)));
    VERIFY(spyCollectionChanged.isValid());

    DBUS_GET2(unlocked, promptPath, service->Unlock({QDBusObjectPath(coll->path())}));
    // nothing is unlocked immediately without user's action
    COMPARE(unlocked, {});

    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    // nothing is unlocked yet
    VERIFY(waitForSignal(spyPromptCompleted, 0));
    DBUS_COMPARE(coll->locked(), true);

    // show the prompt
    DBUS_VERIFY(prompt->Prompt(""));

    // still not unlocked before user action
    VERIFY(waitForSignal(spyPromptCompleted, 0));
    DBUS_COMPARE(coll->locked(), true);

    VERIFY(driveUnlockDialog());

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    {
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.size(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});
    }

    // check unlocked *AFTER* the prompt signal
    DBUS_COMPARE(coll->locked(), false);

    VERIFY(waitForSignal(spyCollectionCreated, 0));
    QTRY_VERIFY(!spyCollectionChanged.isEmpty());
    for (const auto& args : spyCollectionChanged) {
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), coll->path());
    }
    VERIFY(waitForSignal(spyCollectionDeleted, 0));
}

void TestGuiFdoSecrets::testServiceUnlockDatabaseConcurrent()
{
    lockDatabaseInBackend();

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    DBUS_GET2(unlocked, promptPath, service->Unlock({QDBusObjectPath(coll->path())}));
    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());
    DBUS_VERIFY(prompt->Prompt(""));

    // while the first prompt is running, another request come in
    DBUS_GET2(unlocked2, promptPath2, service->Unlock({QDBusObjectPath(coll->path())}));
    auto prompt2 = getProxy<PromptProxy>(promptPath2);
    VERIFY(prompt2);
    QSignalSpy spyPromptCompleted2(prompt2.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted2.isValid());
    DBUS_VERIFY(prompt2->Prompt(""));

    // there should be only one unlock dialog
    VERIFY(driveUnlockDialog());

    // both prompts should complete
    VERIFY(waitForSignal(spyPromptCompleted, 1));
    {
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.size(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});
    }
    VERIFY(waitForSignal(spyPromptCompleted2, 1));
    {
        auto args = spyPromptCompleted2.takeFirst();
        COMPARE(args.size(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});
    }

    // check unlocked *AFTER* prompt signal
    DBUS_COMPARE(coll->locked(), false);
}

void TestGuiFdoSecrets::testServiceUnlockItems()
{
    FdoSecrets::settings()->setConfirmAccessItem(true);

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    DBUS_COMPARE(item->locked(), true);

    {
        DBUS_GET2(unlocked, promptPath, service->Unlock({QDBusObjectPath(item->path())}));
        // nothing is unlocked immediately without user's action
        COMPARE(unlocked, {});

        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);
        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        // nothing is unlocked yet
        COMPARE(spyPromptCompleted.count(), 0);
        DBUS_COMPARE(item->locked(), true);

        // drive the prompt
        DBUS_VERIFY(prompt->Prompt(""));
        // only allow once
        VERIFY(driveAccessControlDialog(false));

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        {
            auto args = spyPromptCompleted.takeFirst();
            COMPARE(args.size(), 2);
            COMPARE(args.at(0).toBool(), false);
            COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(item->path())});
        }

        // unlocked
        DBUS_COMPARE(item->locked(), false);
    }

    // access the secret should reset the locking state
    {
        DBUS_GET(ss, item->GetSecret(QDBusObjectPath(sess->path())));
    }
    DBUS_COMPARE(item->locked(), true);

    // unlock again with remember
    {
        DBUS_GET2(unlocked, promptPath, service->Unlock({QDBusObjectPath(item->path())}));
        // nothing is unlocked immediately without user's action
        COMPARE(unlocked, {});

        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);
        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        // nothing is unlocked yet
        COMPARE(spyPromptCompleted.count(), 0);
        DBUS_COMPARE(item->locked(), true);

        // drive the prompt
        DBUS_VERIFY(prompt->Prompt(""));
        // only allow and remember
        VERIFY(driveAccessControlDialog(true));

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        {
            auto args = spyPromptCompleted.takeFirst();
            COMPARE(args.size(), 2);
            COMPARE(args.at(0).toBool(), false);
            COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(item->path())});
        }

        // unlocked
        DBUS_COMPARE(item->locked(), false);
    }

    // access the secret does not reset the locking state
    {
        DBUS_GET(ss, item->GetSecret(QDBusObjectPath(sess->path())));
    }
    DBUS_COMPARE(item->locked(), false);
}

void TestGuiFdoSecrets::testServiceUnlockItemsIncludeFutureEntries()
{
    FdoSecrets::settings()->setConfirmAccessItem(true);

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    DBUS_COMPARE(item->locked(), true);

    {
        DBUS_GET2(unlocked, promptPath, service->Unlock({QDBusObjectPath(item->path())}));
        // nothing is unlocked immediately without user's action
        COMPARE(unlocked, {});

        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);
        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        // nothing is unlocked yet
        COMPARE(spyPromptCompleted.count(), 0);
        DBUS_COMPARE(item->locked(), true);

        // drive the prompt
        DBUS_VERIFY(prompt->Prompt(""));
        // remember and include future entries
        VERIFY(driveAccessControlDialog(true, true));

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        {
            auto args = spyPromptCompleted.takeFirst();
            COMPARE(args.size(), 2);
            COMPARE(args.at(0).toBool(), false);
            COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(item->path())});
        }

        // unlocked
        DBUS_COMPARE(item->locked(), false);
    }

    // check other entries are also unlocked
    {
        DBUS_GET(itemPaths, coll->items());
        VERIFY(itemPaths.size() > 1);
        auto anotherItem = getProxy<ItemProxy>(itemPaths.last());
        VERIFY(anotherItem);
        DBUS_COMPARE(anotherItem->locked(), false);
    }
}

void TestGuiFdoSecrets::testServiceLock()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    QSignalSpy spyCollectionCreated(service.data(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    VERIFY(spyCollectionCreated.isValid());
    QSignalSpy spyCollectionDeleted(service.data(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    VERIFY(spyCollectionDeleted.isValid());
    QSignalSpy spyCollectionChanged(service.data(), SIGNAL(CollectionChanged(QDBusObjectPath)));
    VERIFY(spyCollectionChanged.isValid());

    // if the db is modified, prompt user
    m_db->markAsModified();
    {
        DBUS_GET2(locked, promptPath, service->Lock({QDBusObjectPath(coll->path())}));
        COMPARE(locked, {});
        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);
        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        // prompt and click cancel
        MessageBox::setNextAnswer(MessageBox::Cancel);
        DBUS_VERIFY(prompt->Prompt(""));
        processEvents();

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), true);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {});

        DBUS_COMPARE(coll->locked(), false);
    }
    {
        DBUS_GET2(locked, promptPath, service->Lock({QDBusObjectPath(coll->path())}));
        COMPARE(locked, {});
        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);
        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        // prompt and click save
        MessageBox::setNextAnswer(MessageBox::Save);
        DBUS_VERIFY(prompt->Prompt(""));
        processEvents();

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});

        DBUS_COMPARE(coll->locked(), true);
    }

    VERIFY(waitForSignal(spyCollectionCreated, 0));
    QTRY_VERIFY(!spyCollectionChanged.isEmpty());
    for (const auto& args : spyCollectionChanged) {
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), coll->path());
    }
    VERIFY(waitForSignal(spyCollectionDeleted, 0));

    // locking item locks the whole db
    unlockDatabaseInBackend();
    {
        auto item = getFirstItem(coll);
        DBUS_GET2(locked, promptPath, service->Lock({QDBusObjectPath(item->path())}));
        COMPARE(locked, {});
        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);

        MessageBox::setNextAnswer(MessageBox::Save);
        DBUS_VERIFY(prompt->Prompt(""));
        processEvents();

        DBUS_COMPARE(coll->locked(), true);
    }
}

void TestGuiFdoSecrets::testServiceLockConcurrent()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    m_db->markAsModified();

    DBUS_GET2(locked, promptPath, service->Lock({QDBusObjectPath(coll->path())}));
    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    DBUS_GET2(locked2, promptPath2, service->Lock({QDBusObjectPath(coll->path())}));
    auto prompt2 = getProxy<PromptProxy>(promptPath2);
    VERIFY(prompt2);
    QSignalSpy spyPromptCompleted2(prompt2.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted2.isValid());

    // prompt and click save
    MessageBox::setNextAnswer(MessageBox::Save);
    DBUS_VERIFY(prompt->Prompt(""));

    // second prompt should not show dialog
    DBUS_VERIFY(prompt2->Prompt(""));

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    {
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});
    }

    VERIFY(waitForSignal(spyPromptCompleted2, 1));
    {
        auto args = spyPromptCompleted2.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(getSignalVariantArgument<QList<QDBusObjectPath>>(args.at(1)), {QDBusObjectPath(coll->path())});
    }

    DBUS_COMPARE(coll->locked(), true);
}

void TestGuiFdoSecrets::testSessionOpen()
{
    auto service = enableService();
    VERIFY(service);

    auto sess = openSession(service, PlainCipher::Algorithm);
    VERIFY(sess);

    sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);
}

void TestGuiFdoSecrets::testSessionClose()
{
    auto service = enableService();
    VERIFY(service);

    auto sess = openSession(service, PlainCipher::Algorithm);
    VERIFY(sess);

    DBUS_VERIFY(sess->Close());
}

void TestGuiFdoSecrets::testCollectionCreate()
{
    auto service = enableService();
    VERIFY(service);

    QSignalSpy spyCollectionCreated(service.data(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    VERIFY(spyCollectionCreated.isValid());

    // returns existing if alias is nonempty and exists
    {
        auto existing = getDefaultCollection(service);
        DBUS_GET2(collPath,
                  promptPath,
                  service->CreateCollection({{DBUS_INTERFACE_SECRET_COLLECTION + ".Label", "NewDB"}}, "default"));
        COMPARE(promptPath, QDBusObjectPath("/"));
        COMPARE(collPath.path(), existing->path());
    }
    VERIFY(waitForSignal(spyCollectionCreated, 0));

    // create new one and set properties
    {
        DBUS_GET2(collPath,
                  promptPath,
                  service->CreateCollection({{DBUS_INTERFACE_SECRET_COLLECTION + ".Label", "Test NewDB"}}, "mydatadb"));
        COMPARE(collPath, QDBusObjectPath("/"));
        auto prompt = getProxy<PromptProxy>(promptPath);
        VERIFY(prompt);

        QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
        VERIFY(spyPromptCompleted.isValid());

        DBUS_VERIFY(prompt->Prompt(""));
        VERIFY(driveNewDatabaseWizard());

        VERIFY(waitForSignal(spyPromptCompleted, 1));
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.size(), 2);
        COMPARE(args.at(0).toBool(), false);
        auto coll = getProxy<CollectionProxy>(getSignalVariantArgument<QDBusObjectPath>(args.at(1)));
        VERIFY(coll);

        DBUS_COMPARE(coll->label(), QStringLiteral("Test NewDB"));

        VERIFY(waitForSignal(spyCollectionCreated, 1));
        {
            args = spyCollectionCreated.takeFirst();
            COMPARE(args.size(), 1);
            COMPARE(args.at(0).value<QDBusObjectPath>().path(), coll->path());
        }
    }
}

void TestGuiFdoSecrets::testCollectionDelete()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    // save the path which will be gone after the deletion.
    auto collPath = coll->path();

    QSignalSpy spyCollectionDeleted(service.data(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    VERIFY(spyCollectionDeleted.isValid());

    m_db->markAsModified();
    DBUS_GET(promptPath, coll->Delete());
    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    // prompt and click save
    MessageBox::setNextAnswer(MessageBox::Save);
    DBUS_VERIFY(prompt->Prompt(""));

    // closing the tab should have deleted the database if not in testing
    // but deleteLater is not processed in QApplication::processEvent
    // see https://doc.qt.io/qt-5/qcoreapplication.html#processEvents

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    auto args = spyPromptCompleted.takeFirst();
    COMPARE(args.count(), 2);
    COMPARE(args.at(0).toBool(), false);
    COMPARE(args.at(1).value<QDBusVariant>().variant().toString(), QStringLiteral(""));

    // however, the object should already be taken down from dbus
    {
        auto reply = coll->locked();
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::UnknownObject);
    }

    VERIFY(waitForSignal(spyCollectionDeleted, 1));
    {
        args = spyCollectionDeleted.takeFirst();
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), collPath);
    }
}

void TestGuiFdoSecrets::testCollectionDeleteConcurrent()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    m_db->markAsModified();
    DBUS_GET(promptPath, coll->Delete());
    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    // before interacting with the prompt, another request come in
    DBUS_GET(promptPath2, coll->Delete());
    auto prompt2 = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt2);
    QSignalSpy spyPromptCompleted2(prompt2.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted2.isValid());

    // prompt and click save
    MessageBox::setNextAnswer(MessageBox::Save);
    DBUS_VERIFY(prompt->Prompt(""));

    // there should be no prompt
    DBUS_VERIFY(prompt2->Prompt(""));

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    {
        auto args = spyPromptCompleted.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(args.at(1).value<QDBusVariant>().variant().toString(), QStringLiteral(""));
    }

    VERIFY(waitForSignal(spyPromptCompleted2, 1));
    {
        auto args = spyPromptCompleted2.takeFirst();
        COMPARE(args.count(), 2);
        COMPARE(args.at(0).toBool(), false);
        COMPARE(args.at(1).value<QDBusVariant>().variant().toString(), QStringLiteral(""));
    }

    {
        auto reply = coll->locked();
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::UnknownObject);
    }
}

void TestGuiFdoSecrets::testCollectionChange()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    QSignalSpy spyCollectionChanged(service.data(), SIGNAL(CollectionChanged(QDBusObjectPath)));
    VERIFY(spyCollectionChanged.isValid());

    DBUS_VERIFY(coll->setLabel("anotherLabel"));
    COMPARE(m_db->metadata()->name(), QStringLiteral("anotherLabel"));
    QTRY_COMPARE(spyCollectionChanged.size(), 1);
    {
        auto args = spyCollectionChanged.takeFirst();
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), coll->path());
    }
}

void TestGuiFdoSecrets::testHiddenFilename()
{
    // when file name contains leading dot, all parts excepting the last should be used
    // for collection name, and the registration should success
    VERIFY(m_dbFile->rename(QFileInfo(*m_dbFile).path() + "/.Name.kdbx"));

    // reset is necessary to not hold database longer and cause connections
    // not cleaned up when the database tab is closed.
    m_db.reset();
    VERIFY(m_tabWidget->closeAllDatabaseTabs());
    m_tabWidget->addDatabaseTab(m_dbFile->fileName(), false, "a");
    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();

    // enable the service
    auto service = enableService();
    VERIFY(service);

    // collection is properly registered
    auto coll = getDefaultCollection(service);
    auto collObj = m_plugin->dbus()->pathToObject<Collection>(QDBusObjectPath(coll->path()));
    VERIFY(collObj);
    COMPARE(collObj->name(), QStringLiteral(".Name"));
}

void TestGuiFdoSecrets::testDuplicateName()
{
    QTemporaryDir dir;
    VERIFY(dir.isValid());
    // create another file under different path but with the same filename
    QString anotherFile = dir.path() + "/" + QFileInfo(*m_dbFile).fileName();
    m_dbFile->copy(anotherFile);
    m_tabWidget->addDatabaseTab(anotherFile, false, "a");

    auto service = enableService();
    VERIFY(service);

    // when two databases have the same name, one of it will have part of its uuid suffixed
    const QString pathNoSuffix = QStringLiteral("/org/freedesktop/secrets/collection/KeePassXC");
    DBUS_GET(colls, service->collections());
    COMPARE(colls.size(), 2);
    COMPARE(colls[0].path(), pathNoSuffix);
    VERIFY(colls[1].path() != pathNoSuffix);
}

void TestGuiFdoSecrets::testItemCreate()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    QSignalSpy spyItemCreated(coll.data(), SIGNAL(ItemCreated(QDBusObjectPath)));
    VERIFY(spyItemCreated.isValid());

    // create item
    StringStringMap attributes{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
    };

    auto item = createItem(sess, coll, "abc", "Password", attributes, false);
    VERIFY(item);

    // signals
    {
        VERIFY(waitForSignal(spyItemCreated, 1));
        auto args = spyItemCreated.takeFirst();
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), item->path());
    }

    // attributes
    {
        DBUS_GET(actual, item->attributes());
        for (const auto& key : attributes.keys()) {
            COMPARE(actual[key], attributes[key]);
        }
    }

    // label
    DBUS_COMPARE(item->label(), QStringLiteral("abc"));

    // secrets
    {
        DBUS_GET(ss, item->GetSecret(QDBusObjectPath(sess->path())));
        auto decrypted = m_clientCipher->decrypt(ss.unmarshal(m_plugin->dbus()));
        COMPARE(decrypted.value, QByteArrayLiteral("Password"));
    }

    // searchable
    {
        DBUS_GET2(unlocked, locked, service->SearchItems(attributes));
        COMPARE(locked, {});
        COMPARE(unlocked, {QDBusObjectPath(item->path())});
    }
    {
        DBUS_GET(unlocked, coll->SearchItems(attributes));
        VERIFY(unlocked.contains(QDBusObjectPath(item->path())));
    }
}

void TestGuiFdoSecrets::testItemCreateUnlock()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    // NOTE: entries are no longer valid after locking
    lockDatabaseInBackend();

    QSignalSpy spyItemCreated(coll.data(), SIGNAL(ItemCreated(QDBusObjectPath)));
    VERIFY(spyItemCreated.isValid());

    // create item
    StringStringMap attributes{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
    };

    auto item = createItem(sess, coll, "abc", "Password", attributes, false, false, true);
    VERIFY(item);
}

void TestGuiFdoSecrets::testItemChange()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    auto entry = itemObj->backend();
    VERIFY(entry);

    QSignalSpy spyItemChanged(coll.data(), SIGNAL(ItemChanged(QDBusObjectPath)));
    VERIFY(spyItemChanged.isValid());

    DBUS_VERIFY(item->setLabel("anotherLabel"));
    COMPARE(entry->title(), QStringLiteral("anotherLabel"));
    QTRY_VERIFY(!spyItemChanged.isEmpty());
    for (const auto& args : spyItemChanged) {
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), item->path());
    }

    spyItemChanged.clear();
    DBUS_VERIFY(item->setAttributes({
        {"abc", "def"},
    }));
    COMPARE(entry->attributes()->value("abc"), QStringLiteral("def"));
    QTRY_VERIFY(!spyItemChanged.isEmpty());
    for (const auto& args : spyItemChanged) {
        COMPARE(args.size(), 1);
        COMPARE(args.at(0).value<QDBusObjectPath>().path(), item->path());
    }
}

void TestGuiFdoSecrets::testItemReplace()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    // create item
    StringStringMap attr1{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
        {"fdosecrets-attr", "1"},
    };
    StringStringMap attr2{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
        {"fdosecrets-attr", "2"},
    };

    auto item1 = createItem(sess, coll, "abc1", "Password", attr1, false);
    VERIFY(item1);
    auto item2 = createItem(sess, coll, "abc2", "Password", attr2, false);
    VERIFY(item2);

    {
        DBUS_GET2(unlocked, locked, service->SearchItems({{"application", "fdosecrets-test"}}));
        QSet<QDBusObjectPath> expected{QDBusObjectPath(item1->path()), QDBusObjectPath(item2->path())};
        COMPARE(QSet<QDBusObjectPath>::fromList(unlocked), expected);
    }

    QSignalSpy spyItemCreated(coll.data(), SIGNAL(ItemCreated(QDBusObjectPath)));
    VERIFY(spyItemCreated.isValid());
    QSignalSpy spyItemChanged(coll.data(), SIGNAL(ItemChanged(QDBusObjectPath)));
    VERIFY(spyItemChanged.isValid());

    {
        // when replace, existing item with matching attr is updated
        auto item3 = createItem(sess, coll, "abc3", "Password", attr2, true);
        VERIFY(item3);
        COMPARE(item2->path(), item3->path());
        DBUS_COMPARE(item3->label(), QStringLiteral("abc3"));
        // there are still 2 entries
        DBUS_GET2(unlocked, locked, service->SearchItems({{"application", "fdosecrets-test"}}));
        QSet<QDBusObjectPath> expected{QDBusObjectPath(item1->path()), QDBusObjectPath(item2->path())};
        COMPARE(QSet<QDBusObjectPath>::fromList(unlocked), expected);

        VERIFY(waitForSignal(spyItemCreated, 0));
        // there may be multiple changed signals, due to each item attribute is set separately
        QTRY_VERIFY(!spyItemChanged.isEmpty());
        for (const auto& args : spyItemChanged) {
            COMPARE(args.size(), 1);
            COMPARE(args.at(0).value<QDBusObjectPath>().path(), item3->path());
        }
    }

    spyItemCreated.clear();
    spyItemChanged.clear();
    {
        // when NOT replace, another entry is created
        auto item4 = createItem(sess, coll, "abc4", "Password", attr2, false);
        VERIFY(item4);
        DBUS_COMPARE(item2->label(), QStringLiteral("abc3"));
        DBUS_COMPARE(item4->label(), QStringLiteral("abc4"));
        // there are 3 entries
        DBUS_GET2(unlocked, locked, service->SearchItems({{"application", "fdosecrets-test"}}));
        QSet<QDBusObjectPath> expected{
            QDBusObjectPath(item1->path()),
            QDBusObjectPath(item2->path()),
            QDBusObjectPath(item4->path()),
        };
        COMPARE(QSet<QDBusObjectPath>::fromList(unlocked), expected);

        VERIFY(waitForSignal(spyItemCreated, 1));
        {
            auto args = spyItemCreated.takeFirst();
            COMPARE(args.size(), 1);
            COMPARE(args.at(0).value<QDBusObjectPath>().path(), item4->path());
        }
        // there may be multiple changed signals, due to each item attribute is set separately
        VERIFY(!spyItemChanged.isEmpty());
        for (const auto& args : spyItemChanged) {
            COMPARE(args.size(), 1);
            COMPARE(args.at(0).value<QDBusObjectPath>().path(), item4->path());
        }
    }
}

void TestGuiFdoSecrets::testItemReplaceExistingLocked()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    // create item
    StringStringMap attr1{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
        {"fdosecrets-attr", "1"},
    };

    auto item = createItem(sess, coll, "abc1", "Password", attr1, false);
    VERIFY(item);

    // make sure the item is locked
    {
        auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
        VERIFY(itemObj);
        auto entry = itemObj->backend();
        VERIFY(entry);
        FdoSecrets::settings()->setConfirmAccessItem(true);
        m_client->setItemAuthorized(entry->uuid(), AuthDecision::Undecided);
        DBUS_COMPARE(item->locked(), true);
    }

    // when replace with a locked item, there will be a prompt
    auto item2 = createItem(sess, coll, "abc2", "PasswordUpdated", attr1, true, true);
    VERIFY(item2);
    COMPARE(item2->path(), item->path());
    DBUS_COMPARE(item2->label(), QStringLiteral("abc2"));
}

void TestGuiFdoSecrets::testItemSecret()
{
    const QString TEXT_PLAIN = "text/plain";
    const QString APPLICATION_OCTET_STREAM = "application/octet-stream";

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    auto entry = itemObj->backend();
    VERIFY(entry);

    // plain text secret
    {
        DBUS_GET(encrypted, item->GetSecret(QDBusObjectPath(sess->path())));
        auto ss = m_clientCipher->decrypt(encrypted.unmarshal(m_plugin->dbus()));
        COMPARE(ss.contentType, TEXT_PLAIN);
        COMPARE(ss.value, entry->password().toUtf8());
    }

    // get secret with notification
    FdoSecrets::settings()->setShowNotification(true);
    {
        QSignalSpy spyShowNotification(m_plugin, SIGNAL(requestShowNotification(QString, QString, int)));
        VERIFY(spyShowNotification.isValid());

        DBUS_GET(encrypted, item->GetSecret(QDBusObjectPath(sess->path())));
        auto ss = m_clientCipher->decrypt(encrypted.unmarshal(m_plugin->dbus()));
        COMPARE(ss.contentType, TEXT_PLAIN);
        COMPARE(ss.value, entry->password().toUtf8());

        COMPARE(ss.contentType, TEXT_PLAIN);
        COMPARE(ss.value, entry->password().toUtf8());

        VERIFY(waitForSignal(spyShowNotification, 1));
    }
    FdoSecrets::settings()->setShowNotification(false);

    // set secret with plain text
    {
        // first create Secret in wire format,
        // then convert to internal format and encrypt
        // finally convert encrypted internal format back to wire format to pass to SetSecret
        const QByteArray expected = QByteArrayLiteral("NewPassword");
        auto encrypted = encryptPassword(expected, TEXT_PLAIN, sess);
        DBUS_VERIFY(item->SetSecret(encrypted));
        COMPARE(entry->password().toUtf8(), expected);
    }

    // set secret with something else is saved as attachment
    const QByteArray expected = QByteArrayLiteral("NewPasswordBinary");
    {
        auto encrypted = encryptPassword(expected, APPLICATION_OCTET_STREAM, sess);
        DBUS_VERIFY(item->SetSecret(encrypted));
        COMPARE(entry->password(), QStringLiteral(""));
    }
    {
        DBUS_GET(encrypted, item->GetSecret(QDBusObjectPath(sess->path())));
        auto ss = m_clientCipher->decrypt(encrypted.unmarshal(m_plugin->dbus()));
        COMPARE(ss.contentType, APPLICATION_OCTET_STREAM);
        COMPARE(ss.value, expected);
    }
}

void TestGuiFdoSecrets::testItemDelete()
{
    FdoSecrets::settings()->setConfirmDeleteItem(true);

    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    // save the path which will be gone after the deletion.
    auto itemPath = item->path();

    QSignalSpy spyItemDeleted(coll.data(), SIGNAL(ItemDeleted(QDBusObjectPath)));
    VERIFY(spyItemDeleted.isValid());

    DBUS_GET(promptPath, item->Delete());
    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);

    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    // prompt and click save
    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    MessageBox::setNextAnswer(MessageBox::Delete);
    DBUS_VERIFY(prompt->Prompt(""));

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    auto args = spyPromptCompleted.takeFirst();
    COMPARE(args.count(), 2);
    COMPARE(args.at(0).toBool(), false);
    COMPARE(args.at(1).toString(), QStringLiteral(""));

    VERIFY(waitForSignal(spyItemDeleted, 1));
    args = spyItemDeleted.takeFirst();
    COMPARE(args.size(), 1);
    COMPARE(args.at(0).value<QDBusObjectPath>().path(), itemPath);
}

void TestGuiFdoSecrets::testItemLockState()
{
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);
    auto itemObj = m_plugin->dbus()->pathToObject<Item>(QDBusObjectPath(item->path()));
    VERIFY(itemObj);
    auto entry = itemObj->backend();
    VERIFY(entry);

    auto secret =
        wire::Secret{
            QDBusObjectPath(sess->path()),
            {},
            "NewPassword",
            "text/plain",
        }
            .unmarshal(m_plugin->dbus());
    auto encrypted = m_clientCipher->encrypt(secret).marshal();

    // when access confirmation is disabled, item is unlocked when the collection is unlocked
    FdoSecrets::settings()->setConfirmAccessItem(false);
    DBUS_COMPARE(item->locked(), false);

    // when access confirmation is enabled, item is locked if the client has no authorization
    FdoSecrets::settings()->setConfirmAccessItem(true);
    DBUS_COMPARE(item->locked(), true);
    // however, item properties are still accessible as long as the collection is unlocked
    DBUS_VERIFY(item->attributes());
    DBUS_VERIFY(item->setAttributes({}));
    DBUS_VERIFY(item->label());
    DBUS_VERIFY(item->setLabel("abc"));
    DBUS_VERIFY(item->created());
    DBUS_VERIFY(item->modified());
    // except secret, which is locked
    {
        auto reply = item->GetSecret(QDBusObjectPath(sess->path()));
        VERIFY(reply.isError());
        COMPARE(reply.error().name(), DBUS_ERROR_SECRET_IS_LOCKED);
    }
    {
        auto reply = item->SetSecret(encrypted);
        VERIFY(reply.isError());
        COMPARE(reply.error().name(), DBUS_ERROR_SECRET_IS_LOCKED);
    }

    // item is unlocked if the client is authorized
    m_client->setItemAuthorized(entry->uuid(), AuthDecision::Allowed);
    DBUS_COMPARE(item->locked(), false);
    DBUS_VERIFY(item->GetSecret(QDBusObjectPath(sess->path())));
    DBUS_VERIFY(item->SetSecret(encrypted));
}

void TestGuiFdoSecrets::testItemRejectSetReferenceFields()
{
    // expose a subgroup, entries in it should not be able to retrieve data from entries outside it
    auto rootEntry = m_db->rootGroup()->entries().first();
    VERIFY(rootEntry);
    auto subgroup = m_db->rootGroup()->findGroupByPath("/Homebanking/Subgroup");
    VERIFY(subgroup);
    FdoSecrets::settings()->setExposedGroup(m_db, subgroup->uuid());
    auto service = enableService();
    VERIFY(service);
    auto coll = getDefaultCollection(service);
    VERIFY(coll);
    auto item = getFirstItem(coll);
    VERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    VERIFY(sess);

    const auto refText = QStringLiteral("{REF:P@T:%1}").arg(rootEntry->title());

    // reject ref in label
    {
        auto reply = item->setLabel(refText);
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::InvalidArgs);
    }
    // reject ref in custom attributes
    {
        auto reply = item->setAttributes({{"steal", refText}});
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::InvalidArgs);
    }
    // reject ref in password
    {
        auto reply = item->SetSecret(encryptPassword(refText.toUtf8(), "text/plain", sess));
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::InvalidArgs);
    }
    // reject ref in content type
    {
        auto reply = item->SetSecret(encryptPassword("dummy", refText, sess));
        VERIFY(reply.isFinished() && reply.isError());
        COMPARE(reply.error().type(), QDBusError::InvalidArgs);
    }
}

void TestGuiFdoSecrets::testAlias()
{
    auto service = enableService();
    VERIFY(service);

    // read default alias
    DBUS_GET(collPath, service->ReadAlias("default"));
    auto coll = getProxy<CollectionProxy>(collPath);
    VERIFY(coll);

    // set extra alias
    DBUS_VERIFY(service->SetAlias("another", QDBusObjectPath(collPath)));

    // get using extra alias
    DBUS_GET(collPath2, service->ReadAlias("another"));
    COMPARE(collPath2, collPath);
}

void TestGuiFdoSecrets::testDefaultAliasAlwaysPresent()
{
    auto service = enableService();
    VERIFY(service);

    // one collection, which is default alias
    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    // after locking, the collection is still there, but locked
    lockDatabaseInBackend();

    coll = getDefaultCollection(service);
    VERIFY(coll);
    DBUS_COMPARE(coll->locked(), true);

    // unlock the database, the alias and collection is present
    unlockDatabaseInBackend();

    coll = getDefaultCollection(service);
    VERIFY(coll);
    DBUS_COMPARE(coll->locked(), false);
}

void TestGuiFdoSecrets::testExposeSubgroup()
{
    auto subgroup = m_db->rootGroup()->findGroupByPath("/Homebanking/Subgroup");
    VERIFY(subgroup);
    FdoSecrets::settings()->setExposedGroup(m_db, subgroup->uuid());
    auto service = enableService();
    VERIFY(service);

    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    // exposing subgroup does not expose entries in other groups
    DBUS_GET(itemPaths, coll->items());
    QSet<Entry*> exposedEntries;
    for (const auto& itemPath : itemPaths) {
        exposedEntries << m_plugin->dbus()->pathToObject<Item>(itemPath)->backend();
    }
    COMPARE(exposedEntries, QSet<Entry*>::fromList(subgroup->entries()));
}

void TestGuiFdoSecrets::testModifyingExposedGroup()
{
    // test when exposed group is removed the collection is not exposed anymore
    auto subgroup = m_db->rootGroup()->findGroupByPath("/Homebanking");
    VERIFY(subgroup);
    FdoSecrets::settings()->setExposedGroup(m_db, subgroup->uuid());
    auto service = enableService();
    VERIFY(service);

    {
        DBUS_GET(collPaths, service->collections());
        COMPARE(collPaths.size(), 1);
    }

    m_db->metadata()->setRecycleBinEnabled(true);
    m_db->recycleGroup(subgroup);
    processEvents();

    {
        DBUS_GET(collPaths, service->collections());
        COMPARE(collPaths, {});
    }

    // test setting another exposed group, the collection will be exposed again
    FdoSecrets::settings()->setExposedGroup(m_db, m_db->rootGroup()->uuid());
    processEvents();
    {
        DBUS_GET(collPaths, service->collections());
        COMPARE(collPaths.size(), 1);
    }
}

void TestGuiFdoSecrets::testNoExposeRecycleBin()
{
    // when the recycle bin is underneath the exposed group
    // be careful not to expose entries in there

    FdoSecrets::settings()->setExposedGroup(m_db, m_db->rootGroup()->uuid());
    m_db->metadata()->setRecycleBinEnabled(true);

    auto entry = m_db->rootGroup()->entries().first();
    VERIFY(entry);
    m_db->recycleEntry(entry);
    processEvents();

    auto service = enableService();
    VERIFY(service);

    auto coll = getDefaultCollection(service);
    VERIFY(coll);

    // exposing subgroup does not expose entries in other groups
    DBUS_GET(itemPaths, coll->items());
    QSet<Entry*> exposedEntries;
    for (const auto& itemPath : itemPaths) {
        exposedEntries << m_plugin->dbus()->pathToObject<Item>(itemPath)->backend();
    }
    VERIFY(!exposedEntries.contains(entry));

    // searching should not return the entry
    DBUS_GET2(unlocked, locked, service->SearchItems({{"Title", entry->title()}}));
    COMPARE(locked, {});
    COMPARE(unlocked, {});
}

void TestGuiFdoSecrets::lockDatabaseInBackend()
{
    m_tabWidget->lockDatabases();
    m_db.reset();
    processEvents();
}

void TestGuiFdoSecrets::unlockDatabaseInBackend()
{
    m_dbWidget->performUnlockDatabase("a");
    m_db = m_dbWidget->database();
    processEvents();
}

void TestGuiFdoSecrets::processEvents()
{
    // Couldn't use QApplication::processEvents, because per Qt documentation:
    //     events that are posted while the function runs will be queued until a later round of event processing.
    // and we may post QTimer single shot events during event handling to achieve async method.
    // So we directly call event dispatcher in a loop until no events can be handled
    while (QAbstractEventDispatcher::instance()->processEvents(QEventLoop::AllEvents)) {
        // pass
    }
}

// the following functions have return value, switch macros to the version supporting that
#undef VERIFY
#undef VERIFY2
#undef COMPARE
#define VERIFY(stmt) VERIFY2_RET(stmt, "")
#define VERIFY2 VERIFY2_RET
#define COMPARE COMPARE_RET

QSharedPointer<ServiceProxy> TestGuiFdoSecrets::enableService()
{
    FdoSecrets::settings()->setEnabled(true);
    VERIFY(m_plugin);
    m_plugin->updateServiceState();
    return getProxy<ServiceProxy>(QDBusObjectPath(DBUS_PATH_SECRETS));
}

QSharedPointer<SessionProxy> TestGuiFdoSecrets::openSession(const QSharedPointer<ServiceProxy>& service,
                                                            const QString& algo)
{
    VERIFY(service);

    if (algo == PlainCipher::Algorithm) {
        DBUS_GET2(output, sessPath, service->OpenSession(algo, QDBusVariant("")));

        return getProxy<SessionProxy>(sessPath);
    } else if (algo == DhIetf1024Sha256Aes128CbcPkcs7::Algorithm) {
        DBUS_GET2(output, sessPath, service->OpenSession(algo, QDBusVariant(m_clientCipher->negotiationOutput())));
        m_clientCipher->updateClientPublicKey(output.variant().toByteArray());
        return getProxy<SessionProxy>(sessPath);
    }
    QTest::qFail("Unsupported algorithm", __FILE__, __LINE__);
    return {};
}

QSharedPointer<CollectionProxy> TestGuiFdoSecrets::getDefaultCollection(const QSharedPointer<ServiceProxy>& service)
{
    VERIFY(service);
    DBUS_GET(collPath, service->ReadAlias("default"));
    return getProxy<CollectionProxy>(collPath);
}

QSharedPointer<ItemProxy> TestGuiFdoSecrets::getFirstItem(const QSharedPointer<CollectionProxy>& coll)
{
    VERIFY(coll);
    DBUS_GET(itemPaths, coll->items());
    VERIFY(!itemPaths.isEmpty());
    return getProxy<ItemProxy>(itemPaths.first());
}

QSharedPointer<ItemProxy> TestGuiFdoSecrets::createItem(const QSharedPointer<SessionProxy>& sess,
                                                        const QSharedPointer<CollectionProxy>& coll,
                                                        const QString& label,
                                                        const QString& pass,
                                                        const StringStringMap& attr,
                                                        bool replace,
                                                        bool expectPrompt,
                                                        bool expectUnlockPrompt)
{
    VERIFY(sess);
    VERIFY(coll);

    QVariantMap properties{
        {DBUS_INTERFACE_SECRET_ITEM + ".Label", QVariant::fromValue(label)},
        {DBUS_INTERFACE_SECRET_ITEM + ".Attributes", QVariant::fromValue(attr)},
    };

    wire::Secret ss;
    ss.session = QDBusObjectPath(sess->path());
    ss.value = pass.toLocal8Bit();
    ss.contentType = "plain/text";
    auto encrypted = m_clientCipher->encrypt(ss.unmarshal(m_plugin->dbus())).marshal();

    DBUS_GET2(itemPath, promptPath, coll->CreateItem(properties, encrypted, replace));

    auto prompt = getProxy<PromptProxy>(promptPath);
    VERIFY(prompt);
    QSignalSpy spyPromptCompleted(prompt.data(), SIGNAL(Completed(bool, QDBusVariant)));
    VERIFY(spyPromptCompleted.isValid());

    // drive the prompt
    DBUS_VERIFY(prompt->Prompt(""));

    bool unlockFound = driveUnlockDialog();
    COMPARE(unlockFound, expectUnlockPrompt);

    bool found = driveAccessControlDialog();
    COMPARE(found, expectPrompt);

    VERIFY(waitForSignal(spyPromptCompleted, 1));
    auto args = spyPromptCompleted.takeFirst();
    COMPARE(args.size(), 2);
    COMPARE(args.at(0).toBool(), false);
    itemPath = getSignalVariantArgument<QDBusObjectPath>(args.at(1));

    return getProxy<ItemProxy>(itemPath);
}

FdoSecrets::wire::Secret
TestGuiFdoSecrets::encryptPassword(QByteArray value, QString contentType, const QSharedPointer<SessionProxy>& sess)
{
    wire::Secret ss;
    ss.contentType = std::move(contentType);
    ss.value = std::move(value);
    ss.session = QDBusObjectPath(sess->path());
    return m_clientCipher->encrypt(ss.unmarshal(m_plugin->dbus())).marshal();
}

bool TestGuiFdoSecrets::driveAccessControlDialog(bool remember, bool includeFutureEntries)
{
    processEvents();
    for (auto w : QApplication::topLevelWidgets()) {
        if (!w->isWindow()) {
            continue;
        }
        auto dlg = qobject_cast<AccessControlDialog*>(w);
        if (dlg && dlg->isVisible()) {
            auto rememberCheck = dlg->findChild<QCheckBox*>("rememberCheck");
            VERIFY(rememberCheck);
            rememberCheck->setChecked(remember);

            if (includeFutureEntries) {
                dlg->done(AccessControlDialog::AllowAll);
            } else {
                dlg->done(AccessControlDialog::AllowSelected);
            }

            processEvents();
            VERIFY(dlg->isHidden());
            return true;
        }
    }
    return false;
}

bool TestGuiFdoSecrets::driveNewDatabaseWizard()
{
    // processEvents will block because the NewDatabaseWizard is shown using exec
    // which creates a local QEventLoop.
    // so we do a little trick here to get the return value back
    bool ret = false;
    QTimer::singleShot(0, this, [this, &ret]() {
        ret = [this]() -> bool {
            auto wizard = m_tabWidget->findChild<NewDatabaseWizard*>();
            VERIFY(wizard);

            COMPARE(wizard->currentId(), 0);
            wizard->next();
            wizard->next();
            COMPARE(wizard->currentId(), 2);

            // enter password
            auto* passwordEdit =
                wizard->findChild<PasswordWidget*>("enterPasswordEdit")->findChild<QLineEdit*>("passwordEdit");
            auto* passwordRepeatEdit =
                wizard->findChild<PasswordWidget*>("repeatPasswordEdit")->findChild<QLineEdit*>("passwordEdit");
            VERIFY(passwordEdit);
            VERIFY(passwordRepeatEdit);
            QTest::keyClicks(passwordEdit, "test");
            QTest::keyClick(passwordEdit, Qt::Key::Key_Tab);
            QTest::keyClicks(passwordRepeatEdit, "test");

            // save database to temporary file
            TemporaryFile tmpFile;
            VERIFY(tmpFile.open());
            tmpFile.close();
            fileDialog()->setNextFileName(tmpFile.fileName());

            // click Continue on the warning due to weak password
            MessageBox::setNextAnswer(MessageBox::ContinueWithWeakPass);
            wizard->accept();

            tmpFile.remove();
            return true;
        }();
    });
    processEvents();
    return ret;
}

bool TestGuiFdoSecrets::driveUnlockDialog(DatabaseWidget* target)
{
    processEvents();
    auto dbOpenDlg = m_tabWidget->findChild<DatabaseOpenDialog*>();
    VERIFY(dbOpenDlg);
    if (!dbOpenDlg->isVisible()) {
        return false;
    }
    dbOpenDlg->setActiveDatabaseTab(target);

    auto editPassword = dbOpenDlg->findChild<PasswordWidget*>("editPassword")->findChild<QLineEdit*>("passwordEdit");
    VERIFY(editPassword);
    editPassword->setFocus();
    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);
    processEvents();
    return true;
}

bool TestGuiFdoSecrets::waitForSignal(QSignalSpy& spy, int expectedCount)
{
    processEvents();
    // If already expected count, do not wait and return immediately
    if (spy.count() == expectedCount) {
        return true;
    } else if (spy.count() > expectedCount) {
        return false;
    }
    spy.wait();
    COMPARE(spy.count(), expectedCount);
    return true;
}

#undef VERIFY
#define VERIFY QVERIFY
#undef COMPARE
#define COMPARE QCOMPARE

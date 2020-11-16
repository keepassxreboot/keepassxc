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
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"
#include "fdosecrets/objects/SessionCipher.h"

#include "TestGlobal.h"
#include "config-keepassx-tests.h"

#include "core/Config.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/wizard/NewDatabaseWizard.h"
#include "util/TemporaryFile.h"

#include <QAbstractItemView>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QLineEdit>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#include <memory>
#include <type_traits>

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

#define VERIFY(statement)                                                                                              \
    do {                                                                                                               \
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__))                         \
            return {};                                                                                                 \
    } while (false)

#define COMPARE(actual, expected)                                                                                      \
    do {                                                                                                               \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))                                \
            return {};                                                                                                 \
    } while (false)

#define FAIL(message)                                                                                                  \
    do {                                                                                                               \
        QTest::qFail(static_cast<const char*>(message), __FILE__, __LINE__);                                           \
        return {};                                                                                                     \
    } while (false)

#define COMPARE_DBUS_LOCAL_CALL(actual, expected)                                                                      \
    do {                                                                                                               \
        const auto a = (actual);                                                                                       \
        QVERIFY(!a.isError());                                                                                         \
        QCOMPARE(a.value(), (expected));                                                                               \
    } while (false)

#define CHECKED_DBUS_LOCAL_CALL(name, stmt)                                                                            \
    std::remove_cv<decltype(stmt)::value_type>::type name;                                                             \
    do {                                                                                                               \
        const auto rep = stmt;                                                                                         \
        QVERIFY(!rep.isError());                                                                                       \
        name = rep.value();                                                                                            \
    } while (false)

namespace
{
    std::unique_ptr<QDBusInterface> interfaceOf(const QDBusObjectPath& objPath, const QString& interface)
    {
        std::unique_ptr<QDBusInterface> iface(new QDBusInterface(DBUS_SERVICE_SECRET, objPath.path(), interface));
        iface->setTimeout(5);
        VERIFY(iface->isValid());
        return iface;
    }

    std::unique_ptr<QDBusInterface> interfaceOf(FdoSecrets::DBusObject* obj)
    {
        VERIFY(obj);
        auto metaAdaptor = obj->dbusAdaptor().metaObject();
        auto ifaceName = metaAdaptor->classInfo(metaAdaptor->indexOfClassInfo("D-Bus Interface")).value();

        return interfaceOf(obj->objectPath(), ifaceName);
    }

    template <typename T> QString extractElement(const QString& doc, T cond)
    {
        QXmlStreamReader reader(doc);
        while (!reader.atEnd()) {
            int st = reader.characterOffset();

            if (reader.readNext() != QXmlStreamReader::StartElement || !cond(reader)) {
                continue;
            }

            reader.skipCurrentElement();
            if (reader.hasError()) {
                break;
            }

            // remove whitespaces between elements to be a little bit flexible
            int ed = reader.characterOffset();
            return doc.mid(st - 1, ed - st + 1).replace(QRegularExpression(R"(>[\s\n]+<)"), "><");
        }
        VERIFY(!reader.hasError());
        return {};
    }

    bool checkDBusSpec(const QString& path, const QString& interface)
    {
        QFile f(QStringLiteral(KEEPASSX_TEST_DATA_DIR "/dbus/interfaces/%1.xml").arg(interface));
        VERIFY(f.open(QFile::ReadOnly | QFile::Text));
        QTextStream in(&f);
        auto spec = in.readAll().replace(QRegularExpression(R"(>[\s\n]+<)"), "><").trimmed();

        auto bus = QDBusConnection::sessionBus();
        auto msg = QDBusMessage::createMethodCall(
            DBUS_SERVICE_SECRET, path, "org.freedesktop.DBus.Introspectable", "Introspect");

        // BlockWithGui enters event loop
        auto reply = QDBusPendingReply<QString>(bus.call(msg, QDBus::BlockWithGui, 5));
        VERIFY(reply.isValid());
        auto actual = extractElement(reply.argumentAt<0>(), [&](const QXmlStreamReader& reader) {
            return reader.name() == "interface" && reader.attributes().value("name") == interface;
        });

        COMPARE(actual, spec);
        return true;
    }
} // namespace

using namespace FdoSecrets;

TestGuiFdoSecrets::~TestGuiFdoSecrets() = default;

void TestGuiFdoSecrets::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    config()->set(Config::GUI_ShowTrayIcon, true);
    config()->set(Config::UpdateCheckMessageShown, true);
    // Disable secret service integration (activate within individual tests to test the plugin)
    FdoSecrets::settings()->setEnabled(false);
    // activate within individual tests
    FdoSecrets::settings()->setShowNotification(false);

    Application::bootstrap();

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    QVERIFY(m_tabWidget);
    m_plugin = FdoSecretsPlugin::getPlugin();
    QVERIFY(m_plugin);
    m_mainWindow->show();

    // Load the NewDatabase.kdbx file into temporary storage
    QFile sourceDbFile(QStringLiteral(KEEPASSX_TEST_DATA_DIR "/NewDatabase.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, m_dbData));
    sourceDbFile.close();

    // set keys for session encryption
    m_serverPublic = MpiFromHex("e407997e8b918419cf851cf3345358fdf"
                                "ffb9564a220ac9c3934efd277cea20d17"
                                "467ecdc56e817f75ac39501f38a4a04ff"
                                "64d627e16c09981c7ad876da255b61c8e"
                                "6a8408236c2a4523cfe6961c26dbdfc77"
                                "c1a27a5b425ca71a019e829fae32c0b42"
                                "0e1b3096b48bc2ce9ccab1d1ff13a5eb4"
                                "b263cee30bdb1a57af9bfa93f");
    m_serverPrivate = MpiFromHex("013f4f3381ef0ca11c4c7363079577b56"
                                 "99b238644e0aba47e24bdba6173590216"
                                 "4f1e12dd0944800a373e090e63192f53b"
                                 "93583e9a9e50bb9d792aafaa3a0f5ae77"
                                 "de0c3423f5820848d88ee3bdd01c889f2"
                                 "7af58a02f5b6693d422b9d189b300d7b1"
                                 "be5076b5795cf8808c31e2e2898368d18"
                                 "ab5c26b0ea3480c9aba8154cf");
    // use the same cipher to do the client side encryption, but exchange the position of client/server keys
    m_cipher.reset(new DhIetf1024Sha256Aes128CbcPkcs7);
    QVERIFY(m_cipher->initialize(MpiFromBytes(MpiToBytes(m_serverPublic)),
                                 MpiFromHex("30d18c6b328bac970c05bda6af2e708b9"
                                            "d6bbbb6dc136c1a2d96e870fabc86ad74"
                                            "1846a26a4197f32f65ea2e7580ad2afe3"
                                            "dd5d6c1224b8368b0df2cd75d520a9ff9"
                                            "7fe894cc7da71b7bd285b4633359c16c8"
                                            "d341f822fa4f0fdf59b5d3448658c46a2"
                                            "a86dbb14ff85823873f8a259ccc52bbb8"
                                            "2b5a4c2a75447982553b42221"),
                                 MpiFromHex("84aafe9c9f356f7762307f4d791acb59e"
                                            "8e3fd562abdbb481d0587f8400ad6c51d"
                                            "af561a1beb9a22c8cd4d2807367c5787b"
                                            "2e06d631ccbb5194b6bb32211583ce688"
                                            "f9c2cebc22a9e4d494d12ebdd570c61a1"
                                            "62a94e88561d25ccd0415339d1f59e1b0"
                                            "6bc6b6b5fde46e23b2410eb034be390d3"
                                            "2407ec7ae90f0831f24afd5ac")));
}

// Every test starts with opening the temp database
void TestGuiFdoSecrets::init()
{
    m_dbFile.reset(new TemporaryFile());
    // Write the temp storage to a temp database file for use in our tests
    QVERIFY(m_dbFile->open());
    QCOMPARE(m_dbFile->write(m_dbData), static_cast<qint64>((m_dbData.size())));
    m_dbFile->close();

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    QApplication::processEvents();

    // open and unlock the database
    m_tabWidget->addDatabaseTab(m_dbFile->fileName(), false, "a");
    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();

    // by default expose the root group
    FdoSecrets::settings()->setExposedGroup(m_db, m_db->rootGroup()->uuid());
    QVERIFY(m_dbWidget->save());
}

// Every test ends with closing the temp database without saving
void TestGuiFdoSecrets::cleanup()
{
    // restore to default settings
    FdoSecrets::settings()->setShowNotification(false);
    FdoSecrets::settings()->setEnabled(false);
    if (m_plugin) {
        m_plugin->updateServiceState();
    }

    // DO NOT save the database
    for (int i = 0; i != m_tabWidget->count(); ++i) {
        m_tabWidget->databaseWidgetFromIndex(i)->database()->markAsClean();
    }
    QVERIFY(m_tabWidget->closeAllDatabaseTabs());
    QApplication::processEvents();

    if (m_dbFile) {
        m_dbFile->remove();
    }
}

void TestGuiFdoSecrets::cleanupTestCase()
{
    if (m_dbFile) {
        m_dbFile->remove();
    }
}

void TestGuiFdoSecrets::testDBusSpec()
{
    auto service = enableService();
    QVERIFY(service);

    // service
    QCOMPARE(service->objectPath().path(), QStringLiteral(DBUS_PATH_SECRETS));
    QVERIFY(checkDBusSpec(service->objectPath().path(), DBUS_INTERFACE_SECRET_SERVICE));

    // default alias
    QVERIFY(checkDBusSpec(DBUS_PATH_DEFAULT_ALIAS, DBUS_INTERFACE_SECRET_COLLECTION));

    // collection
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    QVERIFY(checkDBusSpec(coll->objectPath().path(), DBUS_INTERFACE_SECRET_COLLECTION));

    // item
    auto item = getFirstItem(coll);
    QVERIFY(item);
    QVERIFY(checkDBusSpec(item->objectPath().path(), DBUS_INTERFACE_SECRET_ITEM));

    // session
    auto sess = openSession(service, PlainCipher::Algorithm);
    QVERIFY(sess);
    QVERIFY(checkDBusSpec(sess->objectPath().path(), DBUS_INTERFACE_SECRET_SESSION));

    // prompt
    FdoSecrets::settings()->setNoConfirmDeleteItem(true);
    PromptBase* prompt = nullptr;
    {
        auto rep = item->deleteItem();
        QVERIFY(!rep.isError());
        prompt = rep.value();
    }
    QVERIFY(prompt);
    QVERIFY(checkDBusSpec(prompt->objectPath().path(), DBUS_INTERFACE_SECRET_PROMPT));
}

void TestGuiFdoSecrets::testServiceEnable()
{
    QSignalSpy sigError(m_plugin, SIGNAL(error(QString)));
    QVERIFY(sigError.isValid());

    QSignalSpy sigStarted(m_plugin, SIGNAL(secretServiceStarted()));
    QVERIFY(sigStarted.isValid());

    // make sure no one else is holding the service
    QVERIFY(!QDBusConnection::sessionBus().interface()->isServiceRegistered(DBUS_SERVICE_SECRET));

    // enable the service
    auto service = enableService();
    QVERIFY(service);

    // service started without error
    QVERIFY(sigError.isEmpty());
    QCOMPARE(sigStarted.size(), 1);

    QApplication::processEvents();

    QVERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(DBUS_SERVICE_SECRET));

    // there will be one default collection
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);

    COMPARE_DBUS_LOCAL_CALL(coll->locked(), false);
    COMPARE_DBUS_LOCAL_CALL(coll->label(), m_db->metadata()->name());
    COMPARE_DBUS_LOCAL_CALL(
        coll->created(),
        static_cast<qulonglong>(m_db->rootGroup()->timeInfo().creationTime().toMSecsSinceEpoch() / 1000));
    COMPARE_DBUS_LOCAL_CALL(
        coll->modified(),
        static_cast<qulonglong>(m_db->rootGroup()->timeInfo().lastModificationTime().toMSecsSinceEpoch() / 1000));
}

void TestGuiFdoSecrets::testServiceEnableNoExposedDatabase()
{
    // reset the exposed group and then enable the service
    FdoSecrets::settings()->setExposedGroup(m_db, {});
    auto service = enableService();
    QVERIFY(service);

    // no collections
    COMPARE_DBUS_LOCAL_CALL(service->collections(), QList<Collection*>{});
}

void TestGuiFdoSecrets::testServiceSearch()
{
    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    auto item = getFirstItem(coll);
    QVERIFY(item);

    item->backend()->attributes()->set("fdosecrets-test", "1");
    item->backend()->attributes()->set("fdosecrets-test-protected", "2", true);
    const QString crazyKey = "_a:bc&-+'-e%12df_d";
    const QString crazyValue = "[v]al@-ue";
    item->backend()->attributes()->set(crazyKey, crazyValue);

    // search by title
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"Title", item->backend()->title()}}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked, {item});
    }

    // search by attribute
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"fdosecrets-test", "1"}}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked, {item});
    }
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{crazyKey, crazyValue}}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked, {item});
    }

    // searching using empty terms returns nothing
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked.size(), 0);
    }

    // searching using protected attributes or password returns nothing
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"Password", item->backend()->password()}}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked.size(), 0);
    }
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"fdosecrets-test-protected", "2"}}, locked));
        QCOMPARE(locked.size(), 0);
        QCOMPARE(unlocked.size(), 0);
    }
}

void TestGuiFdoSecrets::testServiceUnlock()
{
    lockDatabaseInBackend();

    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);

    QSignalSpy spyCollectionCreated(&service->dbusAdaptor(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    QVERIFY(spyCollectionCreated.isValid());
    QSignalSpy spyCollectionDeleted(&service->dbusAdaptor(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    QVERIFY(spyCollectionDeleted.isValid());
    QSignalSpy spyCollectionChanged(&service->dbusAdaptor(), SIGNAL(CollectionChanged(QDBusObjectPath)));
    QVERIFY(spyCollectionChanged.isValid());

    PromptBase* prompt = nullptr;
    {
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->unlock({coll.data()}, prompt));
        // nothing is unlocked immediately without user's action
        QVERIFY(unlocked.isEmpty());
    }
    QVERIFY(prompt);
    QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
    QVERIFY(spyPromptCompleted.isValid());

    // nothing is unlocked yet
    QCOMPARE(spyPromptCompleted.count(), 0);
    QVERIFY(coll);
    QVERIFY(coll->backend()->isLocked());

    // drive the prompt
    QVERIFY(!prompt->prompt("").isError());

    // still not unlocked before user action
    QCOMPARE(spyPromptCompleted.count(), 0);
    QVERIFY(coll);
    QVERIFY(coll->backend()->isLocked());

    // interact with the dialog
    QApplication::processEvents();
    {
        auto dbOpenDlg = m_tabWidget->findChild<DatabaseOpenDialog*>();
        QVERIFY(dbOpenDlg);
        auto editPassword = dbOpenDlg->findChild<QLineEdit*>("editPassword");
        QVERIFY(editPassword);
        editPassword->setFocus();
        QTest::keyClicks(editPassword, "a");
        QTest::keyClick(editPassword, Qt::Key_Enter);
    }
    QApplication::processEvents();

    // unlocked
    QVERIFY(coll);
    QVERIFY(!coll->backend()->isLocked());

    QCOMPARE(spyPromptCompleted.count(), 1);
    {
        auto args = spyPromptCompleted.takeFirst();
        QCOMPARE(args.size(), 2);
        QCOMPARE(args.at(0).toBool(), false);
        QCOMPARE(args.at(1).value<QDBusVariant>().variant().value<QList<QDBusObjectPath>>(), {coll->objectPath()});
    }
    QCOMPARE(spyCollectionCreated.count(), 0);
    QCOMPARE(spyCollectionChanged.count(), 1);
    {
        auto args = spyCollectionChanged.takeFirst();
        QCOMPARE(args.size(), 1);
        QCOMPARE(args.at(0).value<QDBusObjectPath>(), coll->objectPath());
    }
    QCOMPARE(spyCollectionDeleted.count(), 0);
}

void TestGuiFdoSecrets::testServiceLock()
{
    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);

    QSignalSpy spyCollectionCreated(&service->dbusAdaptor(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    QVERIFY(spyCollectionCreated.isValid());
    QSignalSpy spyCollectionDeleted(&service->dbusAdaptor(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    QVERIFY(spyCollectionDeleted.isValid());
    QSignalSpy spyCollectionChanged(&service->dbusAdaptor(), SIGNAL(CollectionChanged(QDBusObjectPath)));
    QVERIFY(spyCollectionChanged.isValid());

    // if the db is modified, prompt user
    m_db->markAsModified();
    {
        PromptBase* prompt = nullptr;
        CHECKED_DBUS_LOCAL_CALL(locked, service->lock({coll}, prompt));
        QCOMPARE(locked.size(), 0);
        QVERIFY(prompt);
        QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
        QVERIFY(spyPromptCompleted.isValid());

        // prompt and click cancel
        MessageBox::setNextAnswer(MessageBox::Cancel);
        QVERIFY(!prompt->prompt("").isError());
        QApplication::processEvents();

        QVERIFY(!coll->backend()->isLocked());

        QCOMPARE(spyPromptCompleted.count(), 1);
        auto args = spyPromptCompleted.takeFirst();
        QCOMPARE(args.count(), 2);
        QCOMPARE(args.at(0).toBool(), true);
        QCOMPARE(args.at(1).value<QList<QDBusObjectPath>>(), {});
    }
    {
        PromptBase* prompt = nullptr;
        CHECKED_DBUS_LOCAL_CALL(locked, service->lock({coll}, prompt));
        QCOMPARE(locked.size(), 0);
        QVERIFY(prompt);
        QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
        QVERIFY(spyPromptCompleted.isValid());

        // prompt and click save
        MessageBox::setNextAnswer(MessageBox::Save);
        QVERIFY(!prompt->prompt("").isError());
        QApplication::processEvents();

        QVERIFY(coll->backend()->isLocked());

        QCOMPARE(spyPromptCompleted.count(), 1);
        auto args = spyPromptCompleted.takeFirst();
        QCOMPARE(args.count(), 2);
        QCOMPARE(args.at(0).toBool(), false);
        QCOMPARE(args.at(1).value<QDBusVariant>().variant().value<QList<QDBusObjectPath>>(), {coll->objectPath()});
    }

    QCOMPARE(spyCollectionCreated.count(), 0);
    QCOMPARE(spyCollectionChanged.count(), 1);
    {
        auto args = spyCollectionChanged.takeFirst();
        QCOMPARE(args.size(), 1);
        QCOMPARE(args.at(0).value<QDBusObjectPath>(), coll->objectPath());
    }
    QCOMPARE(spyCollectionDeleted.count(), 0);

    // locking item locks the whole db
    unlockDatabaseInBackend();
    {
        auto item = getFirstItem(coll);
        PromptBase* prompt = nullptr;
        CHECKED_DBUS_LOCAL_CALL(locked, service->lock({item}, prompt));
        QCOMPARE(locked.size(), 0);
        QVERIFY(prompt);

        MessageBox::setNextAnswer(MessageBox::Save);
        QVERIFY(!prompt->prompt("").isError());
        QApplication::processEvents();

        QVERIFY(coll->backend()->isLocked());
    }
}

void TestGuiFdoSecrets::testSessionOpen()
{
    auto service = enableService();
    QVERIFY(service);

    auto sess = openSession(service, PlainCipher::Algorithm);
    QVERIFY(sess);
    QCOMPARE(service->sessions().size(), 1);

    sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    QVERIFY(sess);
    QCOMPARE(service->sessions().size(), 2);
}

void TestGuiFdoSecrets::testSessionClose()
{
    auto service = enableService();
    QVERIFY(service);

    auto sess = openSession(service, PlainCipher::Algorithm);
    QVERIFY(sess);

    QCOMPARE(service->sessions().size(), 1);

    auto rep = sess->close();
    QVERIFY(!rep.isError());

    QCOMPARE(service->sessions().size(), 0);
}

void TestGuiFdoSecrets::testCollectionCreate()
{
    auto service = enableService();
    QVERIFY(service);

    QSignalSpy spyCollectionCreated(&service->dbusAdaptor(), SIGNAL(CollectionCreated(QDBusObjectPath)));
    QVERIFY(spyCollectionCreated.isValid());

    // returns existing if alias is nonempty and exists
    {
        PromptBase* prompt = nullptr;
        CHECKED_DBUS_LOCAL_CALL(
            coll, service->createCollection({{DBUS_INTERFACE_SECRET_COLLECTION ".Label", "NewDB"}}, "default", prompt));
        QVERIFY(!prompt);
        QCOMPARE(coll, getDefaultCollection(service).data());
    }
    QCOMPARE(spyCollectionCreated.count(), 0);

    // create new one and set properties
    {
        PromptBase* prompt = nullptr;
        CHECKED_DBUS_LOCAL_CALL(
            created,
            service->createCollection({{DBUS_INTERFACE_SECRET_COLLECTION ".Label", "Test NewDB"}}, "mydatadb", prompt));
        QVERIFY(!created);
        QVERIFY(prompt);

        QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
        QVERIFY(spyPromptCompleted.isValid());

        QTimer::singleShot(50, this, SLOT(createDatabaseCallback()));
        QVERIFY(!prompt->prompt("").isError());
        QApplication::processEvents();

        QCOMPARE(spyPromptCompleted.count(), 1);
        auto args = spyPromptCompleted.takeFirst();
        QCOMPARE(args.size(), 2);
        QCOMPARE(args.at(0).toBool(), false);
        auto coll =
            FdoSecrets::pathToObject<Collection>(args.at(1).value<QDBusVariant>().variant().value<QDBusObjectPath>());
        QVERIFY(coll);

        QCOMPARE(coll->backend()->database()->metadata()->name(), QStringLiteral("Test NewDB"));

        QCOMPARE(spyCollectionCreated.count(), 1);
        {
            args = spyCollectionCreated.takeFirst();
            QCOMPARE(args.size(), 1);
            QCOMPARE(args.at(0).value<QDBusObjectPath>(), coll->objectPath());
        }
    }
}

void TestGuiFdoSecrets::createDatabaseCallback()
{
    auto wizard = m_tabWidget->findChild<NewDatabaseWizard*>();
    QVERIFY(wizard);

    QCOMPARE(wizard->currentId(), 0);
    wizard->next();
    wizard->next();
    QCOMPARE(wizard->currentId(), 2);

    // enter password
    auto* passwordEdit = wizard->findChild<QLineEdit*>("enterPasswordEdit");
    auto* passwordRepeatEdit = wizard->findChild<QLineEdit*>("repeatPasswordEdit");
    QTest::keyClicks(passwordEdit, "test");
    QTest::keyClick(passwordEdit, Qt::Key::Key_Tab);
    QTest::keyClicks(passwordRepeatEdit, "test");

    // save database to temporary file
    TemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.close();
    fileDialog()->setNextFileName(tmpFile.fileName());

    wizard->accept();

    tmpFile.remove();
}

void TestGuiFdoSecrets::testCollectionDelete()
{
    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    // save the path which will be gone after the deletion.
    auto collPath = coll->objectPath();

    QSignalSpy spyCollectionDeleted(&service->dbusAdaptor(), SIGNAL(CollectionDeleted(QDBusObjectPath)));
    QVERIFY(spyCollectionDeleted.isValid());

    m_db->markAsModified();
    CHECKED_DBUS_LOCAL_CALL(prompt, coll->deleteCollection());
    QVERIFY(prompt);
    QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
    QVERIFY(spyPromptCompleted.isValid());

    // prompt and click save
    MessageBox::setNextAnswer(MessageBox::Save);
    QVERIFY(!prompt->prompt("").isError());

    QApplication::processEvents();

    // closing the tab should have deleted coll if not in testing
    // but deleteLater is not processed in QApplication::processEvent
    // see https://doc.qt.io/qt-5/qcoreapplication.html#processEvents
    // QVERIFY(!coll);

    QCOMPARE(spyPromptCompleted.count(), 1);
    auto args = spyPromptCompleted.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(args.at(0).toBool(), false);
    QCOMPARE(args.at(1).value<QDBusVariant>().variant().toString(), QStringLiteral(""));

    QCOMPARE(spyCollectionDeleted.count(), 1);
    {
        args = spyCollectionDeleted.takeFirst();
        QCOMPARE(args.size(), 1);
        QCOMPARE(args.at(0).value<QDBusObjectPath>(), collPath);
    }
}

void TestGuiFdoSecrets::testHiddenFilename()
{
    // when file name contains leading dot, all parts excepting the last should be used
    // for collection name, and the registration should success
    QVERIFY(m_dbFile->rename(QFileInfo(*m_dbFile).path() + "/.Name.kdbx"));

    // reset is necessary to not hold database longer and cause connections
    // not cleaned up when the database tab is closed.
    m_db.reset();
    QVERIFY(m_tabWidget->closeAllDatabaseTabs());
    m_tabWidget->addDatabaseTab(m_dbFile->fileName(), false, "a");
    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();

    // enable the service
    auto service = enableService();
    QVERIFY(service);

    // collection is properly registered
    auto coll = getDefaultCollection(service);
    QVERIFY(coll->objectPath().path() != "/");
    QCOMPARE(coll->name(), QStringLiteral(".Name"));
}

void TestGuiFdoSecrets::testDuplicateName()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // create another file under different path but with the same filename
    QString anotherFile = dir.path() + "/" + QFileInfo(*m_dbFile).fileName();
    m_dbFile->copy(anotherFile);
    m_tabWidget->addDatabaseTab(anotherFile, false, "a");

    auto service = enableService();
    QVERIFY(service);

    // when two databases have the same name, one of it will have part of its uuid suffixed
    const auto pathNoSuffix = QStringLiteral("/org/freedesktop/secrets/collection/KeePassXC");
    CHECKED_DBUS_LOCAL_CALL(colls, service->collections());
    QCOMPARE(colls.size(), 2);
    QCOMPARE(colls[0]->objectPath().path(), pathNoSuffix);
    QVERIFY(colls[1]->objectPath().path() != pathNoSuffix);
}

void TestGuiFdoSecrets::testItemCreate()
{
    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    QVERIFY(sess);

    QSignalSpy spyItemCreated(&coll->dbusAdaptor(), SIGNAL(ItemCreated(QDBusObjectPath)));
    QVERIFY(spyItemCreated.isValid());

    // create item
    StringStringMap attributes{
        {"application", "fdosecrets-test"},
        {"attr-i[bute]", "![some] -value*"},
    };

    auto item = createItem(sess, coll, "abc", "Password", attributes, false);
    QVERIFY(item);

    // signals
    {
        QCOMPARE(spyItemCreated.count(), 1);
        auto args = spyItemCreated.takeFirst();
        QCOMPARE(args.size(), 1);
        QCOMPARE(args.at(0).value<QDBusObjectPath>(), item->objectPath());
    }

    // attributes
    {
        CHECKED_DBUS_LOCAL_CALL(actual, item->attributes());
        for (const auto& key : attributes.keys()) {
            QVERIFY(actual.contains(key));
            QCOMPARE(actual[key], attributes[key]);
        }
    }

    // label
    COMPARE_DBUS_LOCAL_CALL(item->label(), QStringLiteral("abc"));

    // secrets
    {
        CHECKED_DBUS_LOCAL_CALL(ss, item->getSecret(sess));
        ss = m_cipher->decrypt(ss);
        QCOMPARE(ss.value, QByteArray("Password"));
    }

    // searchable
    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems(attributes, locked));
        QCOMPARE(locked, QList<Item*>{});
        QCOMPARE(unlocked, QList<Item*>{item});
    }
    {
        CHECKED_DBUS_LOCAL_CALL(unlocked, coll->searchItems(attributes));
        QVERIFY(unlocked.contains(item));
    }
}

void TestGuiFdoSecrets::testItemReplace()
{
    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    QVERIFY(sess);

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
    QVERIFY(item1);
    auto item2 = createItem(sess, coll, "abc2", "Password", attr2, false);
    QVERIFY(item2);

    {
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"application", "fdosecrets-test"}}, locked));
        QCOMPARE(unlocked.size(), 2);
    }

    QSignalSpy spyItemCreated(&coll->dbusAdaptor(), SIGNAL(ItemCreated(QDBusObjectPath)));
    QVERIFY(spyItemCreated.isValid());
    QSignalSpy spyItemChanged(&coll->dbusAdaptor(), SIGNAL(ItemChanged(QDBusObjectPath)));
    QVERIFY(spyItemChanged.isValid());

    {
        // when replace, existing item with matching attr is updated
        auto item3 = createItem(sess, coll, "abc3", "Password", attr2, true);
        QVERIFY(item3);
        QCOMPARE(item2, item3);
        COMPARE_DBUS_LOCAL_CALL(item3->label(), QStringLiteral("abc3"));
        // there are still 2 entries
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"application", "fdosecrets-test"}}, locked));
        QCOMPARE(unlocked.size(), 2);

        QCOMPARE(spyItemCreated.count(), 0);
        // there may be multiple changed signals, due to each item attribute is set separately
        QVERIFY(!spyItemChanged.isEmpty());
        for (const auto& args : spyItemChanged) {
            QCOMPARE(args.size(), 1);
            QCOMPARE(args.at(0).value<QDBusObjectPath>(), item3->objectPath());
        }
    }

    spyItemCreated.clear();
    spyItemChanged.clear();
    {
        // when NOT replace, another entry is created
        auto item4 = createItem(sess, coll, "abc4", "Password", attr2, false);
        QVERIFY(item4);
        COMPARE_DBUS_LOCAL_CALL(item2->label(), QStringLiteral("abc3"));
        COMPARE_DBUS_LOCAL_CALL(item4->label(), QStringLiteral("abc4"));
        // there are 3 entries
        QList<Item*> locked;
        CHECKED_DBUS_LOCAL_CALL(unlocked, service->searchItems({{"application", "fdosecrets-test"}}, locked));
        QCOMPARE(unlocked.size(), 3);

        QCOMPARE(spyItemCreated.count(), 1);
        {
            auto args = spyItemCreated.takeFirst();
            QCOMPARE(args.size(), 1);
            QCOMPARE(args.at(0).value<QDBusObjectPath>(), item4->objectPath());
        }
        // there may be multiple changed signals, due to each item attribute is set separately
        QVERIFY(!spyItemChanged.isEmpty());
        for (const auto& args : spyItemChanged) {
            QCOMPARE(args.size(), 1);
            QCOMPARE(args.at(0).value<QDBusObjectPath>(), item4->objectPath());
        }
    }
}

void TestGuiFdoSecrets::testItemSecret()
{
    const QString TEXT_PLAIN = "text/plain";
    const QString APPLICATION_OCTET_STREAM = "application/octet-stream";

    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    auto item = getFirstItem(coll);
    QVERIFY(item);
    auto sess = openSession(service, DhIetf1024Sha256Aes128CbcPkcs7::Algorithm);
    QVERIFY(sess);

    // plain text secret
    {
        CHECKED_DBUS_LOCAL_CALL(encrypted, item->getSecret(sess));
        auto ss = m_cipher->decrypt(encrypted);
        QCOMPARE(ss.contentType, TEXT_PLAIN);
        QCOMPARE(ss.value, item->backend()->password().toUtf8());
    }

    // get secret with notification (only works when called from DBUS)
    FdoSecrets::settings()->setShowNotification(true);
    {
        QSignalSpy spyShowNotification(m_plugin, SIGNAL(requestShowNotification(QString, QString, int)));
        QVERIFY(spyShowNotification.isValid());

        auto iitem = interfaceOf(item);
        QVERIFY(static_cast<bool>(iitem));

        auto replyMsg = iitem->call(QDBus::BlockWithGui, "GetSecret", QVariant::fromValue(sess->objectPath()));
        auto reply = QDBusPendingReply<SecretStruct>(replyMsg);
        QVERIFY(reply.isValid());
        auto ss = m_cipher->decrypt(reply.argumentAt<0>());

        QCOMPARE(ss.contentType, TEXT_PLAIN);
        QCOMPARE(ss.value, item->backend()->password().toUtf8());

        QCOMPARE(spyShowNotification.count(), 1);
    }
    FdoSecrets::settings()->setShowNotification(false);

    // set secret with plain text
    {
        SecretStruct ss;
        ss.contentType = TEXT_PLAIN;
        ss.value = "NewPassword";
        ss.session = sess->objectPath();
        QVERIFY(!item->setSecret(m_cipher->encrypt(ss)).isError());

        QCOMPARE(item->backend()->password().toUtf8(), ss.value);
    }

    // set secret with something else is saved as attachment
    {
        SecretStruct expected;
        expected.contentType = APPLICATION_OCTET_STREAM;
        expected.value = "NewPasswordBinary";
        expected.session = sess->objectPath();
        QVERIFY(!item->setSecret(m_cipher->encrypt(expected)).isError());

        QCOMPARE(item->backend()->password(), QStringLiteral(""));

        CHECKED_DBUS_LOCAL_CALL(encrypted, item->getSecret(sess));
        auto ss = m_cipher->decrypt(encrypted);
        QCOMPARE(ss.contentType, expected.contentType);
        QCOMPARE(ss.value, expected.value);
    }
}

void TestGuiFdoSecrets::testItemDelete()
{
    FdoSecrets::settings()->setNoConfirmDeleteItem(false);

    auto service = enableService();
    QVERIFY(service);
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);
    auto item = getFirstItem(coll);
    QVERIFY(item);
    // save the path which will be gone after the deletion.
    auto itemPath = item->objectPath();

    QSignalSpy spyItemDeleted(&coll->dbusAdaptor(), SIGNAL(ItemDeleted(QDBusObjectPath)));
    QVERIFY(spyItemDeleted.isValid());

    CHECKED_DBUS_LOCAL_CALL(prompt, item->deleteItem());
    QVERIFY(prompt);

    QSignalSpy spyPromptCompleted(&prompt->dbusAdaptor(), SIGNAL(Completed(bool, QDBusVariant)));
    QVERIFY(spyPromptCompleted.isValid());

    // prompt and click save
    if (item->isDeletePermanent()) {
        MessageBox::setNextAnswer(MessageBox::Delete);
    } else {
        MessageBox::setNextAnswer(MessageBox::Move);
    }
    QVERIFY(!prompt->prompt("").isError());

    QApplication::processEvents();

    QCOMPARE(spyPromptCompleted.count(), 1);
    auto args = spyPromptCompleted.takeFirst();
    QCOMPARE(args.count(), 2);
    QCOMPARE(args.at(0).toBool(), false);
    QCOMPARE(args.at(1).toString(), QStringLiteral(""));

    QCOMPARE(spyItemDeleted.count(), 1);
    {
        args = spyItemDeleted.takeFirst();
        QCOMPARE(args.size(), 1);
        QCOMPARE(args.at(0).value<QDBusObjectPath>(), itemPath);
    }
}

void TestGuiFdoSecrets::testAlias()
{
    auto service = enableService();
    QVERIFY(service);

    // read default alias
    CHECKED_DBUS_LOCAL_CALL(coll, service->readAlias("default"));
    QVERIFY(coll);

    // set extra alias
    QVERIFY(!service->setAlias("another", coll).isError());

    // get using extra alias
    CHECKED_DBUS_LOCAL_CALL(coll2, service->readAlias("another"));
    QVERIFY(coll2);
    QCOMPARE(coll, coll2);
}

void TestGuiFdoSecrets::testDefaultAliasAlwaysPresent()
{
    auto service = enableService();
    QVERIFY(service);

    // one collection, which is default alias
    auto coll = getDefaultCollection(service);
    QVERIFY(coll);

    // after locking, the collection is still there, but locked
    lockDatabaseInBackend();

    coll = getDefaultCollection(service);
    QVERIFY(coll);
    COMPARE_DBUS_LOCAL_CALL(coll->locked(), true);

    // unlock the database, the alias and collection is present
    unlockDatabaseInBackend();

    coll = getDefaultCollection(service);
    QVERIFY(coll);
    COMPARE_DBUS_LOCAL_CALL(coll->locked(), false);
}

void TestGuiFdoSecrets::testExposeSubgroup()
{
    auto subgroup = m_db->rootGroup()->findGroupByPath("/Homebanking/Subgroup");
    QVERIFY(subgroup);
    FdoSecrets::settings()->setExposedGroup(m_db, subgroup->uuid());
    auto service = enableService();
    QVERIFY(service);

    auto coll = getDefaultCollection(service);
    QVERIFY(coll);

    // exposing subgroup does not expose entries in other groups
    auto items = coll->items();
    QVERIFY(!items.isError());
    QList<Entry*> exposedEntries;
    for (const auto& item : items.value()) {
        exposedEntries << item->backend();
    }
    QCOMPARE(exposedEntries, subgroup->entries());
}

void TestGuiFdoSecrets::testModifyingExposedGroup()
{
    // test when exposed group is removed the collection is not exposed anymore
    auto subgroup = m_db->rootGroup()->findGroupByPath("/Homebanking");
    QVERIFY(subgroup);
    FdoSecrets::settings()->setExposedGroup(m_db, subgroup->uuid());
    auto service = enableService();
    QVERIFY(service);

    {
        CHECKED_DBUS_LOCAL_CALL(colls, service->collections());
        QCOMPARE(colls.size(), 1);
    }

    m_db->metadata()->setRecycleBinEnabled(true);
    m_db->recycleGroup(subgroup);
    QApplication::processEvents();

    {
        CHECKED_DBUS_LOCAL_CALL(colls, service->collections());
        QCOMPARE(colls.size(), 0);
    }

    // test setting another exposed group, the collection will be exposed again
    FdoSecrets::settings()->setExposedGroup(m_db, m_db->rootGroup()->uuid());
    QApplication::processEvents();
    {
        CHECKED_DBUS_LOCAL_CALL(colls, service->collections());
        QCOMPARE(colls.size(), 1);
    }
}

QPointer<Service> TestGuiFdoSecrets::enableService()
{
    FdoSecrets::settings()->setEnabled(true);
    VERIFY(m_plugin);
    m_plugin->updateServiceState();
    return m_plugin->serviceInstance();
}

QPointer<Session> TestGuiFdoSecrets::openSession(Service* service, const QString& algo)
{
    // open session has to be called actually over DBUS to get peer info

    VERIFY(service);
    auto iservice = interfaceOf(service);
    VERIFY(iservice);

    if (algo == PlainCipher::Algorithm) {
        auto replyMsg = iservice->call(QDBus::BlockWithGui, "OpenSession", algo, QVariant::fromValue(QDBusVariant("")));
        auto reply = QDBusPendingReply<QDBusVariant, QDBusObjectPath>(replyMsg);

        VERIFY(reply.isValid());
        return FdoSecrets::pathToObject<Session>(reply.argumentAt<1>());
    } else if (algo == DhIetf1024Sha256Aes128CbcPkcs7::Algorithm) {

        DhIetf1024Sha256Aes128CbcPkcs7::fixNextServerKeys(MpiFromBytes(MpiToBytes(m_serverPrivate)),
                                                          MpiFromBytes(MpiToBytes(m_serverPublic)));

        auto replyMsg = iservice->call(
            QDBus::BlockWithGui, "OpenSession", algo, QVariant::fromValue(QDBusVariant(m_cipher->m_publicKey)));
        auto reply = QDBusPendingReply<QDBusVariant, QDBusObjectPath>(replyMsg);
        VERIFY(reply.isValid());
        COMPARE(qvariant_cast<QByteArray>(reply.argumentAt<0>().variant()), MpiToBytes(m_serverPublic));
        return FdoSecrets::pathToObject<Session>(reply.argumentAt<1>());
    }
    FAIL("Unsupported algorithm");
}

QPointer<Collection> TestGuiFdoSecrets::getDefaultCollection(Service* service)
{
    VERIFY(service);
    auto coll = service->readAlias("default");
    VERIFY(!coll.isError());
    return coll.value();
}

QPointer<Item> TestGuiFdoSecrets::getFirstItem(Collection* coll)
{
    VERIFY(coll);
    auto items = coll->items();
    VERIFY(!items.isError());
    VERIFY(!items.value().isEmpty());
    return items.value().at(0);
}

QPointer<Item> TestGuiFdoSecrets::createItem(Session* sess,
                                             Collection* coll,
                                             const QString& label,
                                             const QString& pass,
                                             const StringStringMap& attr,
                                             bool replace)
{
    VERIFY(sess);
    VERIFY(coll);

    QVariantMap properties{
        {DBUS_INTERFACE_SECRET_ITEM ".Label", QVariant::fromValue(label)},
        {DBUS_INTERFACE_SECRET_ITEM ".Attributes", QVariant::fromValue(attr)},
    };

    SecretStruct ss;
    ss.session = sess->objectPath();
    ss.value = pass.toLocal8Bit();
    ss.contentType = "plain/text";
    ss = m_cipher->encrypt(ss);

    PromptBase* prompt = nullptr;
    auto item = coll->createItem(properties, ss, replace, prompt);
    VERIFY(!item.isError());
    // creating item does not have a prompt to show
    VERIFY(!prompt);
    return item.value();
}

void TestGuiFdoSecrets::lockDatabaseInBackend()
{
    m_dbWidget->lock();
    m_db.reset();
    QApplication::processEvents();
}

void TestGuiFdoSecrets::unlockDatabaseInBackend()
{
    m_dbWidget->performUnlockDatabase("a");
    m_db = m_dbWidget->database();
    QApplication::processEvents();
}

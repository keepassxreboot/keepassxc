/*
 *  Copyright (C) 2026 Frederic Ruget <fred@atlant.is>
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

#include "TestDeviceStorage.h"

#include <QTest>

#include "core/Database.h"
#include "core/DeviceStorage.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestDeviceStorage)

namespace
{
    const QString Pin = QStringLiteral("654321");
    const QString Location = QStringLiteral("yubikey:/Passwords.kdbx");

    /// An in-memory YubiKey: blobs by name, each with a generation that
    /// changes with every store.
    struct FakeDevice
    {
        QHash<QString, QByteArray> blobs;
        QHash<QString, quint32> generations;
        quint32 nextGeneration = 1;
        int fetches = 0;
        int stores = 0;
        int pinTries = 3;
        bool full = false;
    };

    FakeDevice* fake = nullptr;

    /// A stand-in for libyubiblob, following its documented contract (yb
    /// spec 0039): listing needs no PIN; a wrong PIN is WrongPin with the
    /// tries left; a missing blob is NotFound; a store without room is
    /// StoreFull and writes nothing.
    class FakeBackend : public DeviceStorage::Backend
    {
    public:
        DeviceStorage::Status list(const QString&, QList<DeviceStorage::BlobInfo>* blobs, QString*) override
        {
            blobs->clear();
            for (auto it = fake->blobs.constBegin(); it != fake->blobs.constEnd(); ++it) {
                blobs->append(
                    {it.key(), static_cast<quint32>(it.value().size()), 0, fake->generations.value(it.key()), true});
            }
            return DeviceStorage::Ok;
        }

        DeviceStorage::Status fetch(const QString&,
                                    const QString& name,
                                    const QByteArray& pin,
                                    QByteArray* data,
                                    QString* error,
                                    int* pinTries) override
        {
            if (!pinOk(pin, error, pinTries)) {
                return DeviceStorage::WrongPin;
            }
            if (!fake->blobs.contains(name)) {
                *error = "no blob named " + name;
                return DeviceStorage::NotFound;
            }
            ++fake->fetches;
            *data = fake->blobs.value(name);
            return DeviceStorage::Ok;
        }

        DeviceStorage::Status store(const QString&,
                                    const QString& name,
                                    const QByteArray& data,
                                    const QByteArray& pin,
                                    QString* error,
                                    int* pinTries) override
        {
            if (!pinOk(pin, error, pinTries)) {
                return DeviceStorage::WrongPin;
            }
            if (fake->full) {
                *error = "not enough free objects";
                return DeviceStorage::StoreFull;
            }
            ++fake->stores;
            fake->blobs.insert(name, data);
            fake->generations.insert(name, fake->nextGeneration++);
            return DeviceStorage::Ok;
        }

    private:
        static bool pinOk(const QByteArray& pin, QString* error, int* pinTries)
        {
            if (pin == Pin.toUtf8()) {
                fake->pinTries = 3;
                return true;
            }
            *pinTries = --fake->pinTries;
            *error = "wrong PIN";
            return false;
        }
    };

    QSharedPointer<CompositeKey> key()
    {
        auto k = QSharedPointer<CompositeKey>::create();
        k->addKey(QSharedPointer<PasswordKey>::create("master"));
        return k;
    }

    /// A new database saved to the fake YubiKey.
    void saveNewDatabase(const QString& name)
    {
        auto db = QSharedPointer<Database>::create();
        db->setKey(key());
        db->metadata()->setName(name);
        QString error;
        QVERIFY2(db->saveAs(Location, Database::Atomic, {}, &error), qPrintable(error));
    }
} // namespace

void TestDeviceStorage::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestDeviceStorage::init()
{
    delete fake;
    fake = new FakeDevice;
    auto* storage = DeviceStorage::instance();
    storage->setBackend(std::make_unique<FakeBackend>());
    storage->setPinPrompt([](const QString&) { return Pin; });
    storage->clearCache();
    storage->forgetPin("");
}

void TestDeviceStorage::cleanupTestCase()
{
    DeviceStorage::instance()->setBackend(nullptr);
    DeviceStorage::instance()->setPinPrompt({});
    delete fake;
    fake = nullptr;
}

void TestDeviceStorage::testLocations()
{
    QVERIFY(DeviceStorage::isDeviceLocation("yubikey:12345678/Passwords.kdbx"));
    QVERIFY(!DeviceStorage::isDeviceLocation("/home/me/Passwords.kdbx"));
    QString serial, blob;
    QVERIFY(DeviceStorage::parse("yubikey:12345678/Passwords.kdbx", &serial, &blob));
    QCOMPARE(serial, QString("12345678"));
    QCOMPARE(blob, QString("Passwords.kdbx"));
    QVERIFY(DeviceStorage::parse("yubikey:/Work.kdbx", &serial, &blob));
    QVERIFY(serial.isEmpty());
    QVERIFY(!DeviceStorage::parse("yubikey:12345678/", &serial, &blob));

    // A device location is its own canonical path (there is no file).
    Database db(Location);
    QCOMPARE(db.canonicalFilePath(), Location);
}

/// A database saved to the device and opened from it: the bytes go through
/// `yb store` and `yb fetch -p`, with the PIN on standard input.
void TestDeviceStorage::testSaveAndOpen()
{
    saveNewDatabase("on the YubiKey");
    QCOMPARE(fake->stores, 1);
    QVERIFY(fake->blobs.value("Passwords.kdbx").size() > 0);

    DeviceStorage::instance()->clearCache();
    auto db = QSharedPointer<Database>::create();
    QString error;
    QVERIFY2(db->open(Location, key(), &error), qPrintable(error));
    QCOMPARE(db->metadata()->name(), QString("on the YubiKey"));
    QCOMPARE(db->filePath(), Location);
    QCOMPARE(fake->fetches, 1);
    QVERIFY(!db->isModified());
}

/// Opening again (unlock after lock, header reads) uses the cache: the
/// device is only asked whether the vault changed, without the PIN.
void TestDeviceStorage::testOpenUsesTheCache()
{
    saveNewDatabase("cached");
    for (int i = 0; i < 3; ++i) {
        auto db = QSharedPointer<Database>::create();
        QString error;
        QVERIFY2(db->open(Location, key(), &error), qPrintable(error));
    }
    QCOMPARE(fake->fetches, 0);
}

/// A vault changed on the device (another computer) is fetched again.
void TestDeviceStorage::testChangedVaultIsFetchedAgain()
{
    saveNewDatabase("first");
    auto stored = fake->blobs.value("Passwords.kdbx");
    saveNewDatabase("second");
    // Elsewhere, the first version is stored back.
    fake->blobs.insert("Passwords.kdbx", stored);
    fake->generations.insert("Passwords.kdbx", fake->nextGeneration++);

    auto db = QSharedPointer<Database>::create();
    QString error;
    QVERIFY2(db->open(Location, key(), &error), qPrintable(error));
    QCOMPARE(db->metadata()->name(), QString("first"));
    QCOMPARE(fake->fetches, 1);
}

/// A save the device refuses (store full) fails with a clear message, and
/// leaves the database modified, so nothing is lost.
void TestDeviceStorage::testStoreFullKeepsTheDatabaseModified()
{
    saveNewDatabase("before");
    auto db = QSharedPointer<Database>::create();
    QString error;
    QVERIFY(db->open(Location, key(), &error));
    db->metadata()->setName("after");
    fake->full = true;
    QVERIFY(!db->save(Database::Atomic, {}, &error));
    QVERIFY(error.contains("Not enough room"));
    QVERIFY(db->isModified());

    fake->full = false;
    QVERIFY2(db->save(Database::Atomic, {}, &error), qPrintable(error));
    QVERIFY(!db->isModified());
}

/// The PIN is asked when first needed, then held; forgetting it (at lock)
/// makes the next save ask again.
void TestDeviceStorage::testPinIsAskedOnceAndForgotten()
{
    int prompts = 0;
    DeviceStorage::instance()->setPinPrompt([&prompts](const QString&) {
        ++prompts;
        return Pin;
    });
    saveNewDatabase("one");
    saveNewDatabase("two");
    QCOMPARE(prompts, 1);
    DeviceStorage::instance()->forgetPin("");
    saveNewDatabase("three");
    QCOMPARE(prompts, 2);

    // A cancelled prompt fails the save, and nothing is stored.
    DeviceStorage::instance()->forgetPin("");
    DeviceStorage::instance()->setPinPrompt([](const QString&) { return QString(); });
    auto db = QSharedPointer<Database>::create();
    db->setKey(key());
    QString error;
    QVERIFY(!db->saveAs(Location, Database::Atomic, {}, &error));
    QCOMPARE(fake->stores, 3);
}

/// Reading only the public headers (no key: KeePassXC restoring tabs at
/// start-up) never asks for the PIN; unlocking does.
void TestDeviceStorage::testHeaderReadDoesNotAskForThePin()
{
    saveNewDatabase("restored");
    DeviceStorage::instance()->clearCache();
    DeviceStorage::instance()->forgetPin("");
    int prompts = 0;
    DeviceStorage::instance()->setPinPrompt([&prompts](const QString&) {
        ++prompts;
        return Pin;
    });

    auto headers = QSharedPointer<Database>::create();
    QString error;
    QVERIFY(!headers->open(Location, nullptr, &error));
    QCOMPARE(prompts, 0);
    QCOMPARE(fake->fetches, 0);

    auto db = QSharedPointer<Database>::create();
    QVERIFY2(db->open(Location, key(), &error), qPrintable(error));
    QCOMPARE(prompts, 1);
}

/// A vault that is not on the YubiKey: opening fails, saying so.
void TestDeviceStorage::testMissingVault()
{
    auto db = QSharedPointer<Database>::create();
    QString error;
    QVERIFY(!db->open(Location, key(), &error));
    QVERIFY2(error.contains("is not on the YubiKey"), qPrintable(error));
}

/// A wrong PIN fails with the tries left, and is forgotten: the next
/// operation asks again.
void TestDeviceStorage::testWrongPin()
{
    saveNewDatabase("pin");
    DeviceStorage::instance()->clearCache();
    DeviceStorage::instance()->forgetPin("");
    QStringList answers{"000000", Pin};
    int prompts = 0;
    DeviceStorage::instance()->setPinPrompt([&](const QString&) { return answers.value(prompts++); });

    auto db = QSharedPointer<Database>::create();
    QString error;
    QVERIFY(!db->open(Location, key(), &error));
    QVERIFY2(error.contains("Wrong PIN (2 tries left)"), qPrintable(error));
    QVERIFY2(db->open(Location, key(), &error), qPrintable(error));
    QCOMPARE(prompts, 2);
}

/// The vault list holds the `*.kdbx` blobs only, sorted.
void TestDeviceStorage::testListVaults()
{
    fake->blobs.insert("b.kdbx", "x");
    fake->blobs.insert("a.kdbx", "x");
    fake->blobs.insert("a.passwd", "x");
    QString error;
    QCOMPARE(DeviceStorage::instance()->listVaults({}, &error), QStringList({"a.kdbx", "b.kdbx"}));
}

/// The vault's master password, kept on the YubiKey as `<vault>.passwd`,
/// is read without its final newline; without one, nothing is read and
/// nothing fails (KeePassXC then asks for the master password).
void TestDeviceStorage::testStoredMasterPassword()
{
    QString password, error;
    QVERIFY(DeviceStorage::instance()->readPassword(Location, &password, &error));
    QVERIFY(password.isEmpty());

    QVERIFY(!DeviceStorage::instance()->hasStoredPassword(Location));

    fake->blobs.insert("Passwords.passwd", "correct horse\n");
    QVERIFY(DeviceStorage::instance()->hasStoredPassword(Location));
    QVERIFY(DeviceStorage::instance()->readPassword(Location, &password, &error));
    QCOMPARE(password, QString("correct horse"));
}

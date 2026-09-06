/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "TestPasswordProfile.h"

#include "core/CustomData.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "mock/MockClock.h"
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "core/Database.h"
#include "core/Metadata.h"
#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"
#include "core/PasswordProfile.h"
#include "crypto/Crypto.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestPasswordProfile)

void TestPasswordProfile::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestPasswordProfile::testPasswordProfile()
{
    PasswordProfile profile("TestPassword");

    // Test profile name
    QCOMPARE(profile.name(), QString("TestPassword"));

    // Test password settings
    profile.setPasswordSettings(16,
                                PasswordGenerator::LowerLetters | PasswordGenerator::UpperLetters,
                                PasswordGenerator::ExcludeLookAlike,
                                "!@#",
                                "lo0");

    QCOMPARE(profile.type(), PasswordProfile::Password);
    QVERIFY(profile.isValid());
}

void TestPasswordProfile::testPassphraseProfile()
{
    PasswordProfile profile("TestPassphrase");

    // Test passphrase settings
    profile.setPassphraseSettings(6, PassphraseGenerator::TITLECASE, "-", "eff_large_wordlist.txt");

    QCOMPARE(profile.type(), PasswordProfile::Passphrase);
    QVERIFY(profile.isValid());
}

void TestPasswordProfile::testSerializationAndDeserialization()
{
    // Test password profile serialization
    PasswordProfile originalPassword("SerializationTest");
    originalPassword.setPasswordSettings(20,
                                         PasswordGenerator::LowerLetters | PasswordGenerator::Numbers,
                                         PasswordGenerator::CharFromEveryGroup,
                                         "@#$%",
                                         "abc123");

    QVariantMap data = originalPassword.toVariantMap();
    PasswordProfile deserializedPassword = PasswordProfile::fromVariantMap(data);

    QCOMPARE(deserializedPassword.toVariantMap(), originalPassword.toVariantMap());
    QCOMPARE(deserializedPassword.name(), originalPassword.name());
    QCOMPARE(deserializedPassword.type(), originalPassword.type());
    QVERIFY(deserializedPassword.isValid());

    // Test passphrase profile serialization
    PasswordProfile originalPassphrase("PassphraseSerializationTest");
    originalPassphrase.setPassphraseSettings(4, PassphraseGenerator::UPPERCASE, "_", "custom_word_list.txt");

    data = originalPassphrase.toVariantMap();
    PasswordProfile deserializedPassphrase = PasswordProfile::fromVariantMap(data);

    QCOMPARE(deserializedPassphrase.toVariantMap(), originalPassphrase.toVariantMap());
    QCOMPARE(deserializedPassphrase.name(), originalPassphrase.name());
    QCOMPARE(deserializedPassphrase.type(), originalPassphrase.type());
    QVERIFY(deserializedPassphrase.isValid());
}

void TestPasswordProfile::testDatabaseIntegration()
{
    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("password"));
    db->setKey(key);

    // Test adding profiles
    PasswordProfile profile1("WebsiteProfile");
    profile1.setPasswordSettings(12, PasswordGenerator::DefaultCharset, PasswordGenerator::DefaultFlags);

    PasswordProfile profile2("APIProfile");
    profile2.setPasswordSettings(32,
                                 PasswordGenerator::LowerLetters | PasswordGenerator::UpperLetters
                                     | PasswordGenerator::Numbers,
                                 PasswordGenerator::ExcludeLookAlike,
                                 "!@#$%^&*");

    PasswordProfile profile3("PassphraseProfile");
    profile3.setPassphraseSettings(5, PassphraseGenerator::LOWERCASE, "-");

    // Add profiles to database
    db->addPasswordProfile(profile1);
    db->addPasswordProfile(profile2);
    db->addPasswordProfile(profile3);

    // Test retrieving profiles
    QVERIFY(db->hasPasswordProfile("WebsiteProfile"));
    QVERIFY(db->hasPasswordProfile("APIProfile"));
    QVERIFY(db->hasPasswordProfile("PassphraseProfile"));
    QVERIFY(!db->hasPasswordProfile("NonexistentProfile"));

    QStringList profileNames = db->passwordProfileNames();
    QCOMPARE(profileNames.size(), 3);
    QVERIFY(profileNames.contains("WebsiteProfile"));
    QVERIFY(profileNames.contains("APIProfile"));
    QVERIFY(profileNames.contains("PassphraseProfile"));

    // Test retrieving specific profile
    PasswordProfile retrieved = db->passwordProfile("APIProfile");
    QCOMPARE(retrieved.name(), QString("APIProfile"));
    QCOMPARE(retrieved.type(), PasswordProfile::Password);

    // Test removing profile
    db->removePasswordProfile("WebsiteProfile");
    QVERIFY(!db->hasPasswordProfile("WebsiteProfile"));
    QCOMPARE(db->passwordProfileNames().size(), 2);

    // Test removing non-existent profile (should not crash)
    db->removePasswordProfile("NonexistentProfile");
    QCOMPARE(db->passwordProfileNames().size(), 2);
}

void TestPasswordProfile::testDuplicateNameHandling()
{
    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("password"));
    db->setKey(key);

    // Add initial profile
    PasswordProfile profile1("DuplicateTest");
    profile1.setPasswordSettings(10, PasswordGenerator::LowerLetters, PasswordGenerator::DefaultFlags);
    db->addPasswordProfile(profile1);

    QCOMPARE(db->passwordProfileNames().size(), 1);

    // Add profile with same name (should overwrite)
    PasswordProfile profile2("DuplicateTest");
    profile2.setPasswordSettings(20, PasswordGenerator::UpperLetters, PasswordGenerator::ExcludeLookAlike);
    db->addPasswordProfile(profile2);

    // Should still have only one profile
    QCOMPARE(db->passwordProfileNames().size(), 1);

    // Retrieved profile should have the new settings
    PasswordProfile retrieved = db->passwordProfile("DuplicateTest");
    QCOMPARE(retrieved.name(), QString("DuplicateTest"));
    QCOMPARE(retrieved.toVariantMap().value("passwordLength").toInt(), 20);
    QCOMPARE(retrieved.id(), profile1.id());
}

void TestPasswordProfile::testProfileApplication()
{
    // Test password profile application
    PasswordProfile passwordProfile("ApplicationTest");
    passwordProfile.setPasswordSettings(15,
                                        PasswordGenerator::LowerLetters | PasswordGenerator::Numbers,
                                        PasswordGenerator::CharFromEveryGroup,
                                        "!@#",
                                        "l1o0");

    PasswordGenerator passwordGen;
    passwordProfile.applyPasswordSettings(&passwordGen);

    QCOMPARE(passwordGen.getLength(), 15);
    QCOMPARE(passwordGen.getActiveClasses(), PasswordGenerator::LowerLetters | PasswordGenerator::Numbers);
    QCOMPARE(passwordGen.getFlags(), PasswordGenerator::CharFromEveryGroup);
    QCOMPARE(passwordGen.getCustomCharacterSet(), QString("!@#"));
    QCOMPARE(passwordGen.getExcludedCharacterSet(), QString("l1o0"));

    // Test passphrase profile application
    PasswordProfile passphraseProfile("PassphraseApplicationTest");
    passphraseProfile.setPassphraseSettings(
        6, PassphraseGenerator::UPPERCASE, "_", PassphraseGenerator::DefaultWordList);

    PassphraseGenerator passphraseGen;
    passphraseProfile.applyPassphraseSettings(&passphraseGen);

    QVERIFY(passphraseGen.isWordListValid());
    const auto phrase = passphraseGen.generatePassphrase();
    QCOMPARE(phrase.split('_').size(), 6);
    QCOMPARE(phrase, phrase.toUpper());

    // Test applying wrong profile type (should be safe)
    passwordProfile.applyPassphraseSettings(&passphraseGen); // Should do nothing
    passphraseProfile.applyPasswordSettings(&passwordGen); // Should do nothing
}

void TestPasswordProfile::testPersistence()
{
    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("testkey"));
    db->setKey(key);

    // Add some profiles
    PasswordProfile profile1("PersistTest1");
    profile1.setPasswordSettings(14, PasswordGenerator::LowerLetters, PasswordGenerator::DefaultFlags);

    PasswordProfile profile2("PersistTest2");
    profile2.setPassphraseSettings(4, PassphraseGenerator::UPPERCASE, "_");

    db->addPasswordProfile(profile1);
    db->addPasswordProfile(profile2);

    // Verify profiles exist
    QVERIFY(db->hasPasswordProfile("PersistTest1"));
    QVERIFY(db->hasPasswordProfile("PersistTest2"));
    QCOMPARE(db->passwordProfileNames().size(), 2);

    // Test that the custom data persists in the database structure
    QVERIFY(db->metadata()->customData()->contains("KPXC_PasswordProfiles"));

    // Verify the JSON structure is valid
    QString profilesData = db->metadata()->customData()->value("KPXC_PasswordProfiles");
    QJsonDocument doc = QJsonDocument::fromJson(profilesData.toUtf8());
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    QVariantMap profilesMap = doc.toVariant().toMap();
    QCOMPARE(profilesMap.size(), 2);
    QVERIFY(profilesMap.contains("PersistTest1"));
    QVERIFY(profilesMap.contains("PersistTest2"));

    // Test that profiles can be reconstructed from the stored data
    QList<PasswordProfile> allProfiles = db->passwordProfiles();
    QCOMPARE(allProfiles.size(), 2);

    bool foundProfile1 = false, foundProfile2 = false;
    for (const auto& profile : allProfiles) {
        if (profile.name() == "PersistTest1") {
            QCOMPARE(profile.type(), PasswordProfile::Password);
            foundProfile1 = true;
        } else if (profile.name() == "PersistTest2") {
            QCOMPARE(profile.type(), PasswordProfile::Passphrase);
            foundProfile2 = true;
        }
    }
    QVERIFY(foundProfile1);
    QVERIFY(foundProfile2);

    db->setDefaultPasswordProfile(profile2.id());
    auto entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(db->rootGroup());
    entry->setTitle("Entry with a profile");
    entry->customData()->set(CustomData::PasswordProfile, profile1.id().toString(QUuid::WithoutBraces));
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filename = dir.filePath("profiles.kdbx");
    QString error;
    QVERIFY2(db->saveAs(filename, Database::Atomic, {}, &error), qPrintable(error));
    Database reopened;
    QVERIFY2(reopened.open(filename, key, &error), qPrintable(error));
    QCOMPARE(reopened.passwordProfile("PersistTest1").toVariantMap(), profile1.toVariantMap());
    QCOMPARE(reopened.passwordProfile("PersistTest2").toVariantMap(), profile2.toVariantMap());
    QCOMPARE(reopened.defaultPasswordProfile().id(), profile2.id());
    QCOMPARE(reopened.rootGroup()->entries().first()->customData()->value(CustomData::PasswordProfile),
             entry->customData()->value(CustomData::PasswordProfile));
    QVERIFY(!reopened.publicCustomData().contains(CustomData::PasswordProfiles));
}
void TestPasswordProfile::testInvalidSettings()
{
    PasswordProfile valid("Test");
    auto settings = valid.toVariantMap();
    const QList<QPair<QString, QVariant>> invalid{{"passwordLength", -1},
                                                  {"passwordLength", 1000},
                                                  {"passwordLength", "20"},
                                                  {"passwordLength", 1.5},
                                                  {"type", 9},
                                                  {"charClasses", 4096},
                                                  {"charClasses", 0},
                                                  {"generatorFlags", 128},
                                                  {"id", "invalid"},
                                                  {"name", "  "},
                                                  {"excludedCharacterSet", 5}};
    for (const auto& change : invalid) {
        auto data = settings;
        data.insert(change.first, change.second);
        QVERIFY2(!PasswordProfile::fromVariantMap(data).isValid(), qPrintable(change.first));
    }
    settings.remove("passwordLength");
    QVERIFY(!PasswordProfile::fromVariantMap(settings).isValid());
    valid.setPasswordSettings(
        1, PasswordGenerator::LowerLetters | PasswordGenerator::Numbers, PasswordGenerator::CharFromEveryGroup);
    QVERIFY(!valid.isValid());
    valid.setPasswordSettings(20, PasswordGenerator::NoClass, PasswordGenerator::NoFlags, "abc", "abc");
    QVERIFY(!valid.isValid());
    valid.setPassphraseSettings(101, PassphraseGenerator::LOWERCASE, " ");
    QVERIFY(!valid.isValid());
    valid.setPassphraseSettings(4, static_cast<PassphraseGenerator::PassphraseWordCase>(99), " ");
    QVERIFY(!valid.isValid());
}

void TestPasswordProfile::testProfileIdentityAndIsolation()
{
    Database first;
    Database second;
    PasswordProfile profile("Same name");
    QVERIFY(first.addPasswordProfile(profile));
    QVERIFY(!second.hasPasswordProfile(profile.name()));
    first.setDefaultPasswordProfile(profile.id());
    QCOMPARE(first.defaultPasswordProfile().id(), profile.id());
    first.removePasswordProfile(profile.name());
    QVERIFY(!first.defaultPasswordProfile().isValid());
    PasswordProfile replacement(profile.name());
    QVERIFY(first.addPasswordProfile(replacement));
    QVERIFY(!first.passwordProfile(profile.id()).isValid());
    QVERIFY(second.addPasswordProfile(PasswordProfile(profile.name())));
    QVERIFY(!second.passwordProfile(replacement.id()).isValid());
}

void TestPasswordProfile::testPreserveUnknownData()
{
    Database db;
    PasswordProfile profile("Known");
    QVERIFY(db.addPasswordProfile(profile));
    auto data = QJsonDocument::fromJson(db.metadata()->customData()->value(CustomData::PasswordProfiles).toUtf8())
                    .toVariant()
                    .toMap();
    data.insert("Future profile", QVariantMap{{"type", 99}, {"future", "preserve"}});
    auto known = data.value("Known").toMap();
    known.insert("future", "preserve");
    data.insert("Known", known);
    db.metadata()->customData()->set(CustomData::PasswordProfiles,
                                     QString::fromUtf8(QJsonDocument::fromVariant(data).toJson()));
    QVERIFY(db.addPasswordProfile(profile));
    auto after = QJsonDocument::fromJson(db.metadata()->customData()->value(CustomData::PasswordProfiles).toUtf8())
                     .toVariant()
                     .toMap();
    QCOMPARE(after.value("Future profile"), data.value("Future profile"));
    QCOMPARE(after.value("Known").toMap().value("future"), QVariant("preserve"));
    QVERIFY(!db.addPasswordProfile(PasswordProfile("Future profile")));
    QCOMPARE(QJsonDocument::fromJson(db.metadata()->customData()->value(CustomData::PasswordProfiles).toUtf8())
                 .toVariant()
                 .toMap(),
             after);
    db.removePasswordProfile("Known");
    after = QJsonDocument::fromJson(db.metadata()->customData()->value(CustomData::PasswordProfiles).toUtf8())
                .toVariant()
                .toMap();
    QCOMPARE(after.value("Future profile"), data.value("Future profile"));
    db.metadata()->customData()->set(CustomData::PasswordProfiles, "broken JSON");
    QVERIFY(!db.addPasswordProfile(profile));
    QCOMPARE(db.metadata()->customData()->value(CustomData::PasswordProfiles), QString("broken JSON"));
}

void TestPasswordProfile::testMetadataMerge()
{
    auto* clock = new MockClock(2026, 9, 6, 12, 0, 0);
    MockClock::setup(clock);
    Database older;
    Database newer;
    PasswordProfile first("Older profile");
    PasswordProfile second("Newer profile");
    older.addPasswordProfile(first);
    clock->advanceSecond(10);
    newer.addPasswordProfile(second);
    newer.setDefaultPasswordProfile(second.id());
    // Profiles follow the existing database metadata merge policy: the newer collection wins.
    Merger merger(&newer, &older);
    merger.merge();
    MockClock::teardown();
    QVERIFY(older.hasPasswordProfile(second.name()));
    QVERIFY(!older.hasPasswordProfile(first.name()));
    QCOMPARE(older.defaultPasswordProfile().id(), second.id());
}

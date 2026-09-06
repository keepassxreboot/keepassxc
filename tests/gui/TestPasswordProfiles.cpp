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

#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordProfile.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/MessageBox.h"
#include "gui/PasswordGeneratorWidget.h"
#include "gui/PasswordWidget.h"
#include "gui/entry/EditEntryWidget.h"
#include <QCheckBox>
#include <QInputDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class TestPasswordProfiles : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void testSelectionAndIsolation();
    void testInvalidAndMissingProfiles();
    void testPassphrase();
    void testDraftApplyAndCancel();
    void testDefaultForNewEntry();
    void testEntryCancelAndCommit();
    void testReleaseDatabase();
    void testProfileManagement();
    void testAssociationOnlyChange();
    void testLockDuringProfileDialog();

private:
    QTemporaryDir m_configDir;
};

void TestPasswordProfiles::initTestCase()
{
    QVERIFY(Crypto::init());
    QVERIFY(m_configDir.isValid());
    Config::createConfigFromFile(m_configDir.filePath("config.ini"), {});
    Application::bootstrap();
}

void TestPasswordProfiles::testSelectionAndIsolation()
{
    config()->set(Config::PasswordGenerator_AdditionalChars, "global");
    config()->set(Config::PasswordGenerator_Length, 18);
    Database db;
    PasswordProfile profile("Only a and b");
    profile.setPasswordSettings(35, PasswordGenerator::NoClass, PasswordGenerator::AdvancedMode, "abc", "c");
    QVERIFY(db.addPasswordProfile(profile));
    db.setDefaultPasswordProfile(profile.id());
    PasswordGeneratorWidget widget;
    widget.setPasswordLength(8);
    widget.setDatabase(&db);
    QCOMPARE(widget.selectedProfile(), profile.id());
    QCOMPARE(widget.getGeneratedPassword().size(), 35);
    QVERIFY(QRegularExpression("^[ab]{35}$").match(widget.getGeneratedPassword()).hasMatch());
    widget.saveSettings();
    QCOMPARE(config()->get(Config::PasswordGenerator_AdditionalChars).toString(), QString("global"));
    QCOMPARE(config()->get(Config::PasswordGenerator_Length).toInt(), 18);
    auto* length = widget.findChild<QSpinBox*>("spinBoxLength");
    QVERIFY(length);
    length->setValue(36);
    QVERIFY(widget.selectedProfile().isNull());
    QCOMPARE(db.passwordProfile(profile.id()).toVariantMap().value("passwordLength").toInt(), 35);

    PasswordProfile basic("Digits");
    basic.setPasswordSettings(20, PasswordGenerator::Numbers, PasswordGenerator::NoFlags);
    QVERIFY(db.addPasswordProfile(basic));
    widget.setDatabase(&db, basic.id());
    QVERIFY(QRegularExpression("^[0-9]{20}$").match(widget.getGeneratedPassword()).hasMatch());
    QCOMPARE(widget.selectedProfile(), basic.id());
    QSignalSpy applied(&widget, &PasswordGeneratorWidget::appliedProfile);
    widget.applyPassword();
    QCOMPARE(applied.count(), 1);
    QCOMPARE(applied.first().first().toUuid(), basic.id());
}

void TestPasswordProfiles::testInvalidAndMissingProfiles()
{
    Database db;
    PasswordProfile profile("Test");
    QVERIFY(db.addPasswordProfile(profile));
    db.setDefaultPasswordProfile(profile.id());
    PasswordGeneratorWidget widget;
    widget.setDatabase(&db, QUuid::createUuid());
    QVERIFY(widget.getGeneratedPassword().isEmpty());
    QSignalSpy applied(&widget, &PasswordGeneratorWidget::appliedPassword);
    widget.applyPassword();
    QCOMPARE(applied.count(), 0);
    widget.findChild<QSpinBox*>("spinBoxLength")->setValue(21);
    QVERIFY(!widget.getGeneratedPassword().isEmpty());
    QVERIFY(widget.selectedProfile().isNull());
    widget.setDatabase(&db, profile.id());
    widget.findChild<QPushButton*>("checkBoxLower")->setChecked(false);
    widget.findChild<QPushButton*>("checkBoxUpper")->setChecked(false);
    QTest::mouseClick(widget.findChild<QPushButton*>("checkBoxNumbers"), Qt::LeftButton);
    QVERIFY(widget.getGeneratedPassword().isEmpty());
    widget.applyPassword();
    QCOMPARE(applied.count(), 0);
}

void TestPasswordProfiles::testPassphrase()
{
    Database db;
    PasswordProfile profile("Words");
    profile.setPassphraseSettings(7, PassphraseGenerator::UPPERCASE, "--", PassphraseGenerator::DefaultWordList);
    QVERIFY(db.addPasswordProfile(profile));
    PasswordGeneratorWidget widget;
    widget.setDatabase(&db, profile.id());
    auto phrase = widget.getGeneratedPassword();
    QCOMPARE(phrase.split("--").size(), 7);
    QCOMPARE(phrase, phrase.toUpper());
    profile.setPassphraseSettings(7, PassphraseGenerator::UPPERCASE, "--", "/missing/list.txt");
    QVERIFY(db.addPasswordProfile(profile));
    widget.setDatabase(&db, profile.id());
    QVERIFY(widget.getGeneratedPassword().isEmpty());
    QVERIFY(!widget.findChild<QPushButton*>("buttonApply")->isEnabled());
}

void TestPasswordProfiles::testDraftApplyAndCancel()
{
    Database db;
    PasswordProfile profile("Digits");
    profile.setPasswordSettings(40, PasswordGenerator::Numbers, PasswordGenerator::NoFlags);
    QVERIFY(db.addPasswordProfile(profile));
    CustomData draft;
    PasswordWidget password;
    password.setText("old");
    password.setGeneratorContext(&db, &draft);
    QVERIFY(QMetaObject::invokeMethod(&password, "popupPasswordGenerator"));
    auto* generator = password.findChild<PasswordGeneratorWidget*>();
    QVERIFY(generator);
    generator->setDatabase(&db, profile.id());
    generator->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!draft.contains(CustomData::PasswordProfile));
    QCOMPARE(password.text(), QString("old"));
    QVERIFY(QMetaObject::invokeMethod(&password, "popupPasswordGenerator"));
    generator = password.findChild<PasswordGeneratorWidget*>();
    QVERIFY(generator);
    generator->setDatabase(&db, profile.id());
    generator->applyPassword();
    QCOMPARE(password.text().size(), 40);
    QCOMPARE(QUuid(draft.value(CustomData::PasswordProfile)), profile.id());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void TestPasswordProfiles::testDefaultForNewEntry()
{
    auto db = QSharedPointer<Database>::create();
    PasswordProfile profile("New entry default");
    profile.setPassphraseSettings(5, PassphraseGenerator::UPPERCASE, "-");
    QVERIFY(db->addPasswordProfile(profile));
    db->setDefaultPasswordProfile(profile.id());
    config()->set(Config::AutoGeneratePasswordForNewEntries, true);
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(db->rootGroup());
    EditEntryWidget editor;
    editor.loadEntry(entry, true, false, "Database", db);
    auto* password = editor.findChild<PasswordWidget*>("passwordEdit");
    QVERIFY(password);
    QCOMPARE(password->text().split('-').size(), 5);
    QVERIFY(!entry->customData()->contains(CustomData::PasswordProfile));
    QVERIFY(QMetaObject::invokeMethod(&editor, "commitEntry"));
    QCOMPARE(QUuid(entry->customData()->value(CustomData::PasswordProfile)), profile.id());
    QCOMPARE(entry->password().split('-').size(), 5);
    config()->set(Config::AutoGeneratePasswordForNewEntries, false);
}

void TestPasswordProfiles::testEntryCancelAndCommit()
{
    auto db = QSharedPointer<Database>::create();
    PasswordProfile profile("Digits");
    profile.setPasswordSettings(42, PasswordGenerator::Numbers, PasswordGenerator::NoFlags);
    QVERIFY(db->addPasswordProfile(profile));
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(db->rootGroup());
    entry->setPassword("old");
    EditEntryWidget editor;
    for (bool save : {false, true}) {
        editor.loadEntry(entry, false, false, "Database", db);
        auto* password = editor.findChild<PasswordWidget*>("passwordEdit");
        QVERIFY(password);
        QVERIFY(QMetaObject::invokeMethod(password, "popupPasswordGenerator"));
        auto* generator = password->findChild<PasswordGeneratorWidget*>();
        QVERIFY(generator);
        generator->setDatabase(db.data(), profile.id());
        generator->applyPassword();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(password->text().size(), 42);
        QVERIFY(!entry->customData()->contains(CustomData::PasswordProfile));
        if (save) {
            QVERIFY(QMetaObject::invokeMethod(&editor, "commitEntry"));
            QCOMPARE(QUuid(entry->customData()->value(CustomData::PasswordProfile)), profile.id());
            QCOMPARE(entry->password().size(), 42);
        } else {
            MessageBox::setNextAnswer(MessageBox::Discard);
            QVERIFY(QMetaObject::invokeMethod(&editor, "cancel"));
            QCOMPARE(entry->password(), QString("old"));
            QVERIFY(!entry->customData()->contains(CustomData::PasswordProfile));
        }
    }
}

void TestPasswordProfiles::testReleaseDatabase()
{
    Database db;
    PasswordProfile profile("Private profile");
    QVERIFY(db.addPasswordProfile(profile));
    PasswordGeneratorWidget widget;
    widget.setStandaloneMode(true);
    widget.setDatabase(&db, profile.id());
    QVERIFY(!widget.getGeneratedPassword().isEmpty());
    db.releaseData();
    QCOMPARE(widget.findChild<QComboBox*>("profileComboBox")->count(), 1);
    QVERIFY(widget.getGeneratedPassword().isEmpty());
    QVERIFY(widget.selectedProfile().isNull());
}

void TestPasswordProfiles::testProfileManagement()
{
    Database db;
    PasswordGeneratorWidget widget;
    widget.setDatabase(&db);
    auto* length = widget.findChild<QSpinBox*>("spinBoxLength");
    length->setValue(28);
    auto answerName = [] {
        auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->setTextValue("Work");
        dialog->accept();
    };
    QTimer::singleShot(0, answerName);
    QVERIFY(QMetaObject::invokeMethod(&widget, "saveProfile"));
    QVERIFY(db.hasPasswordProfile("Work"));
    const auto id = db.passwordProfile("Work").id();
    QCOMPARE(widget.selectedProfile(), id);
    QTest::mouseClick(widget.findChild<QPushButton*>("defaultProfileButton"), Qt::LeftButton);
    QCOMPARE(db.defaultPasswordProfile().id(), id);
    length->setValue(29);
    QTimer::singleShot(0, answerName);
    MessageBox::setNextAnswer(MessageBox::Cancel);
    QVERIFY(QMetaObject::invokeMethod(&widget, "saveProfile"));
    QCOMPARE(db.passwordProfile("Work").toVariantMap().value("passwordLength").toInt(), 28);
    QTimer::singleShot(0, answerName);
    MessageBox::setNextAnswer(MessageBox::Yes);
    QVERIFY(QMetaObject::invokeMethod(&widget, "saveProfile"));
    QCOMPARE(db.passwordProfile("Work").toVariantMap().value("passwordLength").toInt(), 29);
    QCOMPARE(db.passwordProfile("Work").id(), id);
    MessageBox::setNextAnswer(MessageBox::Cancel);
    QVERIFY(QMetaObject::invokeMethod(&widget, "removeProfile"));
    QVERIFY(db.hasPasswordProfile("Work"));
    MessageBox::setNextAnswer(MessageBox::Remove);
    QVERIFY(QMetaObject::invokeMethod(&widget, "removeProfile"));
    QVERIFY(!db.hasPasswordProfile("Work"));
    QVERIFY(!db.defaultPasswordProfile().isValid());
    QVERIFY(widget.selectedProfile().isNull());
}

void TestPasswordProfiles::testAssociationOnlyChange()
{
    auto db = QSharedPointer<Database>::create();
    PasswordProfile profile("Deterministic test");
    profile.setPasswordSettings(1, PasswordGenerator::NoClass, PasswordGenerator::AdvancedMode, "x");
    QVERIFY(db->addPasswordProfile(profile));
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(db->rootGroup());
    entry->setPassword("x");
    EditEntryWidget editor;
    editor.loadEntry(entry, false, false, "Database", db);
    QVERIFY(!editor.isModified());
    auto* password = editor.findChild<PasswordWidget*>("passwordEdit");
    QVERIFY(QMetaObject::invokeMethod(password, "popupPasswordGenerator"));
    auto* generator = password->findChild<PasswordGeneratorWidget*>();
    QVERIFY(generator);
    generator->setDatabase(db.data(), profile.id());
    generator->applyPassword();
    QCOMPARE(password->text(), QString("x"));
    QVERIFY(editor.isModified());
    QVERIFY(QMetaObject::invokeMethod(&editor, "commitEntry"));
    QCOMPARE(QUuid(entry->customData()->value(CustomData::PasswordProfile)), profile.id());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void TestPasswordProfiles::testLockDuringProfileDialog()
{
    Database db;
    PasswordWidget password;
    CustomData draft;
    password.setGeneratorContext(&db, &draft);
    QVERIFY(QMetaObject::invokeMethod(&password, "popupPasswordGenerator"));
    QPointer<PasswordGeneratorWidget> generator = password.findChild<PasswordGeneratorWidget*>();
    QVERIFY(generator);
    QTimer::singleShot(0, [&db] {
        db.releaseData();
        // Process deletion while the nested naming dialog is running.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    QVERIFY(QMetaObject::invokeMethod(generator, "saveProfile"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(generator.isNull());
    QVERIFY(db.passwordProfiles().isEmpty());
    QVERIFY(!draft.contains(CustomData::PasswordProfile));
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setQuitOnLastWindowClosed(false);
    TestPasswordProfiles test;
    return QTest::qExec(&test, argc, argv);
}

#include "TestPasswordProfiles.moc"

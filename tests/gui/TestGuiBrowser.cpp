/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestGuiBrowser.h"
#include "gui/Application.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QTest>
#include <QToolBar>
#include <QTreeWidget>

#include "browser/BrowserPasskeysConfirmationDialog.h"
#include "browser/BrowserService.h"
#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/passkeys/PasskeyImportDialog.h"

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setApplicationVersion(KEEPASSXC_VERSION);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    app.applyTheme();
    TestGuiBrowser tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

void TestGuiBrowser::initTestCase()
{
    QVERIFY(Crypto::init());
    // Create temporary config file
    Config::createConfigFromFile(TemporaryFile::createTempConfigFile(), {});
    // Disable autosave so we can test the modified file indicator
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    // Enable the tray icon so we can test hiding/restoring the windowQByteArray
    config()->set(Config::GUI_ShowTrayIcon, true);
    // Disable the update check first time alert
    config()->set(Config::UpdateCheckMessageShown, true);
    // Disable quick unlock
    config()->set(Config::Security_QuickUnlock, false);

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();

    QLocale::setDefault(QLocale::c());
}

// Every test starts with opening the temp database
void TestGuiBrowser::init()
{
    m_dbFile.reset(new TemporaryFile());
    m_dbFile->copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabaseBrowser.kdbx"));

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    QApplication::processEvents();

    fileDialog()->setNextFileName(m_dbFile->fileName());
    triggerAction("actionDatabaseOpen");

    auto* databaseOpenWidget = m_tabWidget->currentDatabaseWidget()->findChild<QWidget*>("databaseOpenWidget");
    QVERIFY(databaseOpenWidget);
    auto* editPassword =
        databaseOpenWidget->findChild<PasswordWidget*>("editPassword")->findChild<QLineEdit*>("passwordEdit");
    QVERIFY(editPassword);
    editPassword->setFocus();

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

// Every test ends with closing the temp database without saving
void TestGuiBrowser::cleanup()
{
    // DO NOT save the database
    m_db->markAsClean();
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");
    QApplication::processEvents();
    MessageBox::setNextAnswer(MessageBox::NoButton);

    if (m_dbWidget) {
        delete m_dbWidget;
    }

    m_dbFile->remove();
}

void TestGuiBrowser::cleanupTestCase()
{
    m_dbFile->remove();
}

void TestGuiBrowser::testEntrySettings()
{
    // Enable the Browser Integration
    config()->set(Config::Browser_Enabled, true);

    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    entryView->setFocus();
    QVERIFY(entryView->hasFocus());

    // Select the first entry in the database
    QModelIndex entryItem = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(entryItem);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditEntryMode);
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");

    // Switch to Properties page and select all rows from the custom data table
    editEntryWidget->setCurrentPage(5);
    auto customDataTableView = editEntryWidget->findChild<QTableView*>("customDataTable");
    QVERIFY(customDataTableView);
    QTest::mouseClick(customDataTableView, Qt::LeftButton);
    QTest::keyClick(customDataTableView, 'a', Qt::ControlModifier);

    // Remove the data
    QCOMPARE(entry->customData()->size(), 2);
    auto* removeButton = editEntryWidget->findChild<QPushButton*>("removeCustomDataButton");
    QVERIFY(removeButton);
    MessageBox::setNextAnswer(MessageBox::Delete);
    QTest::mouseClick(removeButton, Qt::LeftButton);

    // Apply the removal
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    auto* okButton = editEntryWidgetButtonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    QTRY_VERIFY(okButton->isEnabled());
    QTest::mouseClick(okButton, Qt::LeftButton);
    QApplication::processEvents();

    QCOMPARE(entry->customData()->size(), 0);
}

void TestGuiBrowser::testAdditionalURLs()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    entryView->setFocus();
    QVERIFY(entryView->hasFocus());

    // Select the first entry in the database
    QModelIndex entryItem = entryView->model()->index(0, 1);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditEntryMode);
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");

    // Switch to Browser Integration page and add three URL's
    editEntryWidget->setCurrentPage(4);
    auto* addURLButton = editEntryWidget->findChild<QPushButton*>("addURLButton");
    QVERIFY(addURLButton);

    auto* urlList = editEntryWidget->findChild<QListView*>("additionalURLsView");
    QVERIFY(urlList);

    QStringList testURLs = {"https://example1.com", "https://example2.com", "https://example3.com"};

    for (const auto& url : testURLs) {
        QTest::mouseClick(addURLButton, Qt::LeftButton);
        QApplication::processEvents();
        QTest::keyClicks(urlList->focusWidget(), url);
    }

    // Check the values from attributesEdit
    editEntryWidget->setCurrentPage(1);
    auto* attributesView = editEntryWidget->findChild<QListView*>("attributesView");
    auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");

    // Go top of the list
    attributesView->setFocus();
    QTest::keyClick(attributesView->focusWidget(), Qt::Key_PageUp);

    for (const auto& url : testURLs) {
        QCOMPARE(attrTextEdit->toPlainText(), url);
        QTest::keyClick(attributesView->focusWidget(), Qt::Key_Down);
    }
}

void TestGuiBrowser::testGetDatabaseGroups()
{
    auto result = browserService()->getDatabaseGroups();
    QCOMPARE(result.length(), 1);

    auto groups = result["groups"].toArray();
    auto first = groups.at(0);
    auto children = first.toObject()["children"].toArray();
    QCOMPARE(first.toObject()["name"].toString(), QString("NewDatabase"));
    QCOMPARE(children.size(), 6);

    auto firstChild = children.at(0).toObject();
    auto secondChild = children.at(1).toObject();
    QCOMPARE(firstChild["name"].toString(), QString("General"));
    QCOMPARE(secondChild["name"].toString(), QString("Windows"));

    auto subGroups = firstChild["children"].toArray();
    QCOMPARE(subGroups.count(), 1);
    auto subGroupObj = subGroups.at(0).toObject();
    QCOMPARE(subGroupObj["name"].toString(), QString("SubGroup"));
}

void TestGuiBrowser::testPasskeyImportDialogSearch()
{
    auto* group = new Group();
    group->setUuid(QUuid::createUuid());
    group->setName("Passkey Test");
    group->setParent(m_db->rootGroup());

    auto* mailEntry = new Entry();
    mailEntry->setUuid(QUuid::createUuid());
    mailEntry->setTitle("Mail Account");
    mailEntry->setUsername("alice");
    mailEntry->setGroup(group);

    auto* bankEntry = new Entry();
    bankEntry->setUuid(QUuid::createUuid());
    bankEntry->setTitle("Bank Account");
    bankEntry->setUsername("bob");
    bankEntry->setGroup(m_db->rootGroup());

    PasskeyImportDialog dialog;
    dialog.setInfo("example.com", "alice", m_db, false);

    auto* searchEdit = dialog.findChild<QLineEdit*>("searchEntryLineEdit");
    auto* entryTree = dialog.findChild<QTreeWidget*>("entryTreeWidget");
    auto* groupComboBox = dialog.findChild<QComboBox*>("selectGroupComboBox");
    QVERIFY(searchEdit);
    QVERIFY(entryTree);
    QVERIFY(groupComboBox);

    // All entries are listed regardless of their group, "Create new entry" is selected by default
    const auto entryCount = m_db->rootGroup()->entriesRecursive().size();
    QCOMPARE(entryTree->topLevelItemCount(), entryCount + 1);
    QVERIFY(dialog.createNewEntry());
    QVERIFY(groupComboBox->isEnabled());

    // Searching selects the first match
    QTest::keyClicks(searchEdit, "account");
    QCOMPARE(entryTree->currentItem()->text(0), QString("Bank Account"));
    QCOMPARE(dialog.getSelectedEntryUuid(), bankEntry->uuid());
    QVERIFY(!dialog.createNewEntry());
    QVERIFY(!groupComboBox->isEnabled());

    int visibleCount = 0;
    for (int i = 0; i < entryTree->topLevelItemCount(); ++i) {
        if (!entryTree->topLevelItem(i)->isHidden()) {
            ++visibleCount;
        }
    }
    QCOMPARE(visibleCount, 3);

    // Arrow keys in the search field move the selection
    QTest::keyClick(searchEdit, Qt::Key_Down);
    QCOMPARE(dialog.getSelectedEntryUuid(), mailEntry->uuid());

    // Entries in subgroups can be found by username
    searchEdit->clear();
    QTest::keyClicks(searchEdit, "alice");
    QCOMPARE(dialog.getSelectedEntryUuid(), mailEntry->uuid());

    // Without matches, a new entry is created
    searchEdit->clear();
    QTest::keyClicks(searchEdit, "no such entry");
    QVERIFY(dialog.createNewEntry());
    QVERIFY(groupComboBox->isEnabled());
}

void TestGuiBrowser::testPasskeySuggestedEntries()
{
    const QString keyName("passkeytest");
    const QString key("testkey");
    m_db->metadata()->customData()->set(CustomData::getKeyWithPrefix(CustomData::BrowserKeyPrefix, keyName), key);
    const StringPairList keyList({{keyName, key}});

    auto* entryWithoutPasskey = new Entry();
    entryWithoutPasskey->setUuid(QUuid::createUuid());
    entryWithoutPasskey->setTitle("Example");
    entryWithoutPasskey->setUrl("https://example.com");
    entryWithoutPasskey->setGroup(m_db->rootGroup());

    auto* entryWithPasskey = new Entry();
    entryWithPasskey->setUuid(QUuid::createUuid());
    entryWithPasskey->setTitle("Example with passkey");
    entryWithPasskey->setUrl("https://example.com");
    entryWithPasskey->setGroup(m_db->rootGroup());
    browserService()->addPasskeyToEntry(
        entryWithPasskey, "example.com", "example.com", "user", "credId", "userHandle", "privateKey");

    auto* otherEntry = new Entry();
    otherEntry->setUuid(QUuid::createUuid());
    otherEntry->setTitle("Other");
    otherEntry->setUrl("https://other.org");
    otherEntry->setGroup(m_db->rootGroup());

    const auto suggestedEntries = browserService()->getPasskeySuggestedEntries("https://example.com", keyList);
    QCOMPARE(suggestedEntries.size(), 1);
    QCOMPARE(suggestedEntries.first(), entryWithoutPasskey);

    // Add to a suggested entry
    BrowserPasskeysConfirmationDialog dialog;
    dialog.registerCredential("user", "example.com", {}, suggestedEntries, 30000);
    auto* otherEntryButton = dialog.findChild<QPushButton*>("otherEntryButton");
    QVERIFY(otherEntryButton);
    QVERIFY(!otherEntryButton->isHidden());
    QTest::mouseClick(dialog.findChild<QPushButton*>("updateButton"), Qt::LeftButton);
    QVERIFY(dialog.isPasskeyUpdated());
    QCOMPARE(dialog.getSelectedEntry(), entryWithoutPasskey);

    // Choose another entry instead
    BrowserPasskeysConfirmationDialog otherDialog;
    otherDialog.registerCredential("user", "example.com", {}, suggestedEntries, 30000);
    QTest::mouseClick(otherDialog.findChild<QPushButton*>("otherEntryButton"), Qt::LeftButton);
    QVERIFY(otherDialog.isPasskeyUpdated());
    QVERIFY(!otherDialog.getSelectedEntry());

    // Without suggestions the button is hidden
    BrowserPasskeysConfirmationDialog emptyDialog;
    emptyDialog.registerCredential("user", "example.com", {}, {}, 30000);
    QVERIFY(emptyDialog.findChild<QPushButton*>("otherEntryButton")->isHidden());
}

void TestGuiBrowser::triggerAction(const QString& name)
{
    auto* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QApplication::processEvents();
}

void TestGuiBrowser::clickIndex(const QModelIndex& index,
                                QAbstractItemView* view,
                                Qt::MouseButton button,
                                Qt::KeyboardModifiers stateKey)
{
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}

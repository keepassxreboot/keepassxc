/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "TestGui.h"
#include "TestGlobal.h"
#include "gui/Application.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "config-keepassx-tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordHealth.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass2Reader.h"
#include "gui/ApplicationSettingsWidget.h"
#include "gui/CategoryListWidget.h"
#include "gui/CloneDialog.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/EntryPreviewWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordEdit.h"
#include "gui/PasswordGeneratorWidget.h"
#include "gui/SearchWidget.h"
#include "gui/TotpDialog.h"
#include "gui/TotpSetupDialog.h"
#include "gui/databasekey/KeyComponentWidget.h"
#include "gui/databasekey/KeyFileEditWidget.h"
#include "gui/databasekey/PasswordEditWidget.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/EditGroupWidget.h"
#include "gui/group/GroupModel.h"
#include "gui/group/GroupView.h"
#include "gui/wizard/NewDatabaseWizard.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

#define TEST_MODAL_NO_WAIT(TEST_CODE)                                                                                  \
    bool dialogFinished = false;                                                                                       \
    QTimer::singleShot(0, [&]() { TEST_CODE dialogFinished = true; })

#define TEST_MODAL(TEST_CODE)                                                                                          \
    TEST_MODAL_NO_WAIT(TEST_CODE);                                                                                     \
    QTRY_VERIFY(dialogFinished)

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
    TestGui tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

static QString dbFileName = QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx");

void TestGui::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    // Disable autosave so we can test the modified file indicator
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    // Enable the tray icon so we can test hiding/restoring the windowQByteArray
    config()->set(Config::GUI_ShowTrayIcon, true);
    // Disable advanced settings mode (activate within individual tests to test advanced settings)
    config()->set(Config::GUI_AdvancedSettings, false);
    // Disable the update check first time alert
    config()->set(Config::UpdateCheckMessageShown, true);

    Application::bootstrap();

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();
    m_mainWindow->resize(1024, 768);
}

// Every test starts with opening the temp database
void TestGui::init()
{
    // Copy the test database file to the temporary file
    QVERIFY(m_dbFile.copyFromFile(dbFileName));

    m_dbFileName = QFileInfo(m_dbFile.fileName()).fileName();
    m_dbFilePath = m_dbFile.fileName();

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    QApplication::processEvents();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");

    auto* databaseOpenWidget = m_tabWidget->currentDatabaseWidget()->findChild<QWidget*>("databaseOpenWidget");
    QVERIFY(databaseOpenWidget);
    auto* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);
    editPassword->setFocus();

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

// Every test ends with closing the temp database without saving
void TestGui::cleanup()
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
}

void TestGui::cleanupTestCase()
{
    m_dbFile.remove();
}

void TestGui::testSettingsDefaultTabOrder()
{
    // check application settings default tab order
    triggerAction("actionSettings");
    auto* settingsWidget = m_mainWindow->findChild<ApplicationSettingsWidget*>();
    QVERIFY(settingsWidget->isVisible());
    QCOMPARE(settingsWidget->findChild<CategoryListWidget*>("categoryList")->currentCategory(), 0);
    for (auto* w : settingsWidget->findChildren<QTabWidget*>()) {
        if (w->currentIndex() != 0) {
            QFAIL("Application settings contain QTabWidgets whose default index is not 0");
        }
    }
    QTest::keyClick(settingsWidget, Qt::Key::Key_Escape);

    // check database settings default tab order
    triggerAction("actionDatabaseSettings");
    auto* dbSettingsWidget = m_mainWindow->findChild<DatabaseSettingsDialog*>();
    QVERIFY(dbSettingsWidget->isVisible());
    QCOMPARE(dbSettingsWidget->findChild<CategoryListWidget*>("categoryList")->currentCategory(), 0);
    for (auto* w : dbSettingsWidget->findChildren<QTabWidget*>()) {
        if (w->currentIndex() != 0) {
            QFAIL("Database settings contain QTabWidgets whose default index is not 0");
        }
    }
    QTest::keyClick(dbSettingsWidget, Qt::Key::Key_Escape);
}

void TestGui::testCreateDatabase()
{
    TEST_MODAL_NO_WAIT(
        NewDatabaseWizard * wizard; QTRY_VERIFY(wizard = m_tabWidget->findChild<NewDatabaseWizard*>());

        QTest::keyClicks(wizard->currentPage()->findChild<QLineEdit*>("databaseName"), "Test Name");
        QTest::keyClicks(wizard->currentPage()->findChild<QLineEdit*>("databaseDescription"), "Test Description");
        QCOMPARE(wizard->currentId(), 0);

        QTest::keyClick(wizard, Qt::Key_Enter);
        QCOMPARE(wizard->currentId(), 1);

        auto decryptionTimeSlider = wizard->currentPage()->findChild<QSlider*>("decryptionTimeSlider");
        auto algorithmComboBox = wizard->currentPage()->findChild<QComboBox*>("algorithmComboBox");
        QTRY_VERIFY(decryptionTimeSlider->isVisible());
        QVERIFY(!algorithmComboBox->isVisible());
        auto advancedToggle = wizard->currentPage()->findChild<QPushButton*>("advancedSettingsButton");
        QTest::mouseClick(advancedToggle, Qt::MouseButton::LeftButton);
        QTRY_VERIFY(!decryptionTimeSlider->isVisible());
        QVERIFY(algorithmComboBox->isVisible());

        auto rounds = wizard->currentPage()->findChild<QSpinBox*>("transformRoundsSpinBox");
        QVERIFY(rounds);
        QVERIFY(rounds->isVisible());
        QTest::mouseClick(rounds, Qt::MouseButton::LeftButton);
        QTest::keyClick(rounds, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(rounds, "2");
        QTest::keyClick(rounds, Qt::Key_Tab);
        QTest::keyClick(rounds, Qt::Key_Tab);

        auto memory = wizard->currentPage()->findChild<QSpinBox*>("memorySpinBox");
        QVERIFY(memory);
        QVERIFY(memory->isVisible());
        QTest::mouseClick(memory, Qt::MouseButton::LeftButton);
        QTest::keyClick(memory, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(memory, "50");
        QTest::keyClick(memory, Qt::Key_Tab);

        auto parallelism = wizard->currentPage()->findChild<QSpinBox*>("parallelismSpinBox");
        QVERIFY(parallelism);
        QVERIFY(parallelism->isVisible());
        QTest::mouseClick(parallelism, Qt::MouseButton::LeftButton);
        QTest::keyClick(parallelism, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(parallelism, "1");
        QTest::keyClick(parallelism, Qt::Key_Enter);

        QCOMPARE(wizard->currentId(), 2);

        // enter password
        auto* passwordWidget = wizard->currentPage()->findChild<PasswordEditWidget*>();
        QCOMPARE(passwordWidget->visiblePage(), KeyFileEditWidget::Page::Edit);
        auto* passwordEdit = passwordWidget->findChild<QLineEdit*>("enterPasswordEdit");
        auto* passwordRepeatEdit = passwordWidget->findChild<QLineEdit*>("repeatPasswordEdit");
        QTRY_VERIFY(passwordEdit->isVisible());
        QTRY_VERIFY(passwordEdit->hasFocus());
        QTest::keyClicks(passwordEdit, "test");
        QTest::keyClick(passwordEdit, Qt::Key::Key_Tab);
        QTest::keyClicks(passwordRepeatEdit, "test");

        // add key file
        auto* additionalOptionsButton = wizard->currentPage()->findChild<QPushButton*>("additionalKeyOptionsToggle");
        auto* keyFileWidget = wizard->currentPage()->findChild<KeyFileEditWidget*>();
        QVERIFY(additionalOptionsButton->isVisible());
        QTest::mouseClick(additionalOptionsButton, Qt::MouseButton::LeftButton);
        QTRY_VERIFY(keyFileWidget->isVisible());
        QTRY_VERIFY(!additionalOptionsButton->isVisible());
        QCOMPARE(passwordWidget->visiblePage(), KeyFileEditWidget::Page::Edit);
        QTest::mouseClick(keyFileWidget->findChild<QPushButton*>("addButton"), Qt::MouseButton::LeftButton);
        auto* fileCombo = keyFileWidget->findChild<QComboBox*>("keyFileCombo");
        QTRY_VERIFY(fileCombo);
        QTRY_VERIFY(fileCombo->isVisible());
        fileDialog()->setNextFileName(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"));
        QTest::keyClick(keyFileWidget->findChild<QPushButton*>("addButton"), Qt::Key::Key_Enter);
        QVERIFY(fileCombo->hasFocus());
        auto* browseButton = keyFileWidget->findChild<QPushButton*>("browseKeyFileButton");
        QTest::keyClick(browseButton, Qt::Key::Key_Enter);
        QCOMPARE(fileCombo->currentText(), QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"));

        // save database to temporary file
        TemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.close();
        fileDialog()->setNextFileName(tmpFile.fileName());

        QTest::keyClick(fileCombo, Qt::Key::Key_Enter);
        tmpFile.remove(););

    triggerAction("actionDatabaseNew");

    // there is a new empty db
    m_db = m_tabWidget->currentDatabaseWidget()->database();
    QCOMPARE(m_db->rootGroup()->children().size(), 0);

    // check meta data
    QCOMPARE(m_db->metadata()->name(), QString("Test Name"));
    QCOMPARE(m_db->metadata()->description(), QString("Test Description"));

    // check key and encryption
    QCOMPARE(m_db->key()->keys().size(), 2);
    QCOMPARE(m_db->kdf()->rounds(), 2);
    QCOMPARE(m_db->kdf()->uuid(), KeePass2::KDF_ARGON2);
    QCOMPARE(m_db->cipher(), KeePass2::CIPHER_AES256);
    auto compositeKey = QSharedPointer<CompositeKey>::create();
    compositeKey->addKey(QSharedPointer<PasswordKey>::create("test"));
    auto fileKey = QSharedPointer<FileKey>::create();
    fileKey->load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"));
    compositeKey->addKey(fileKey);
    QCOMPARE(m_db->key()->rawKey(), compositeKey->rawKey());

    // close the new database
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");

    // Wait for dialog to terminate
    QTRY_VERIFY(dialogFinished);
}

void TestGui::testMergeDatabase()
{
    // It is safe to ignore the warning this line produces
    QSignalSpy dbMergeSpy(m_dbWidget.data(), SIGNAL(databaseMerged(QSharedPointer<Database>)));
    QApplication::processEvents();

    // set file to merge from
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx"));
    triggerAction("actionDatabaseMerge");

    QTRY_COMPARE(QApplication::focusWidget()->objectName(), QString("editPassword"));
    auto* editPasswordMerge = QApplication::focusWidget();
    QVERIFY(editPasswordMerge->isVisible());

    QTest::keyClicks(editPasswordMerge, "a");
    QTest::keyClick(editPasswordMerge, Qt::Key_Enter);

    QTRY_COMPARE(dbMergeSpy.count(), 1);
    QTRY_VERIFY(m_tabWidget->tabName(m_tabWidget->currentIndex()).contains("*"));

    m_db = m_tabWidget->currentDatabaseWidget()->database();

    // there are seven child groups of the root group
    QCOMPARE(m_db->rootGroup()->children().size(), 7);
    // the merged group should contain an entry
    QCOMPARE(m_db->rootGroup()->children().at(6)->entries().size(), 1);
    // the General group contains one entry merged from the other db
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
}

void TestGui::testAutoreloadDatabase()
{
    config()->set(Config::AutoReloadOnChange, false);

    // Test accepting new file in autoreload
    MessageBox::setNextAnswer(MessageBox::Yes);
    // Overwrite the current database with the temp data
    QVERIFY(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));

    QTRY_VERIFY(m_db != m_dbWidget->database());
    m_db = m_dbWidget->database();

    // the General group contains one entry from the new db data
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
    QVERIFY(!m_tabWidget->tabName(m_tabWidget->currentIndex()).endsWith("*"));

    // Reset the state
    cleanup();
    init();

    // Test rejecting new file in autoreload
    MessageBox::setNextAnswer(MessageBox::No);
    // Overwrite the current database with the temp data
    QVERIFY(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));

    // Ensure the merge did not take place
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 0);
    QTRY_VERIFY(m_tabWidget->tabName(m_tabWidget->currentIndex()).endsWith("*"));

    // Reset the state
    cleanup();
    init();

    // Test accepting a merge of edits into autoreload
    // Turn on autoload so we only get one messagebox (for the merge)
    config()->set(Config::AutoReloadOnChange, true);
    // Modify some entries
    testEditEntry();

    // This is saying yes to merging the entries
    MessageBox::setNextAnswer(MessageBox::Merge);
    // Overwrite the current database with the temp data
    QVERIFY(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));

    QTRY_VERIFY(m_db != m_dbWidget->database());
    m_db = m_dbWidget->database();

    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
    QTRY_VERIFY(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));
}

void TestGui::testTabs()
{
    QCOMPARE(m_tabWidget->count(), 1);
    QCOMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), m_dbFileName);
}

void TestGui::testEditEntry()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    entryView->setFocus();
    QVERIFY(entryView->hasFocus());

    // Select the first entry in the database
    QModelIndex entryItem = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(entryItem);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    // Confirm the edit action button is enabled
    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());

    // Record current history count
    int editCount = entry->historyItems().size();

    // Edit the first entry ("Sample Entry")
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "_test");

    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    auto* okButton = editEntryWidgetButtonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    auto* applyButton = editEntryWidgetButtonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton);

    // Apply the edit
    QTRY_VERIFY(applyButton->isEnabled());
    QTest::mouseClick(applyButton, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);
    QCOMPARE(entry->title(), QString("Sample Entry_test"));
    QCOMPARE(entry->historyItems().size(), ++editCount);
    QVERIFY(!applyButton->isEnabled());

    // Test the "known bad" checkbox
    editEntryWidget->setCurrentPage(1);
    auto knownBadCheckBox = editEntryWidget->findChild<QCheckBox*>("knownBadCheckBox");
    QVERIFY(knownBadCheckBox);
    QCOMPARE(knownBadCheckBox->isChecked(), false);
    knownBadCheckBox->setChecked(true);
    QTest::mouseClick(applyButton, Qt::LeftButton);
    QCOMPARE(entry->historyItems().size(), ++editCount);
    QCOMPARE(entry->customData()->contains(PasswordHealth::OPTION_KNOWN_BAD), true);
    QCOMPARE(entry->customData()->value(PasswordHealth::OPTION_KNOWN_BAD), TRUE_STR);

    // Test entry colors (simulate choosing a color)
    editEntryWidget->setCurrentPage(1);
    auto fgColor = QString("#FF0000");
    auto bgColor = QString("#0000FF");
    // Set foreground color
    auto colorButton = editEntryWidget->findChild<QPushButton*>("fgColorButton");
    auto colorCheckBox = editEntryWidget->findChild<QCheckBox*>("fgColorCheckBox");
    colorButton->setProperty("color", fgColor);
    colorCheckBox->setChecked(true);
    // Set background color
    colorButton = editEntryWidget->findChild<QPushButton*>("bgColorButton");
    colorCheckBox = editEntryWidget->findChild<QCheckBox*>("bgColorCheckBox");
    colorButton->setProperty("color", bgColor);
    colorCheckBox->setChecked(true);
    QTest::mouseClick(applyButton, Qt::LeftButton);
    QCOMPARE(entry->historyItems().size(), ++editCount);

    // Test protected attributes
    editEntryWidget->setCurrentPage(1);
    auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("addAttributeButton"), Qt::LeftButton);
    QString attrText = "TEST TEXT";
    QTest::keyClicks(attrTextEdit, attrText);
    QCOMPARE(attrTextEdit->toPlainText(), attrText);
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("protectAttributeButton"), Qt::LeftButton);
    QVERIFY(attrTextEdit->toPlainText().contains("PROTECTED"));
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("revealAttributeButton"), Qt::LeftButton);
    QCOMPARE(attrTextEdit->toPlainText(), attrText);
    editEntryWidget->setCurrentPage(0);

    // Save the edit (press OK)
    QTest::mouseClick(okButton, Qt::LeftButton);
    QApplication::processEvents();

    // Confirm edit was made
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    QCOMPARE(entry->title(), QString("Sample Entry_test"));
    QCOMPARE(entry->foregroundColor().toUpper(), fgColor.toUpper());
    QCOMPARE(entryItem.data(Qt::ForegroundRole), QVariant(fgColor));
    QCOMPARE(entry->backgroundColor().toUpper(), bgColor.toUpper());
    QCOMPARE(entryItem.data(Qt::BackgroundRole), QVariant(bgColor));
    QCOMPARE(entry->historyItems().size(), ++editCount);

    // Confirm modified indicator is showing
    QTRY_COMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), QString("%1*").arg(m_dbFileName));

    // Test copy & paste newline sanitization
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    okButton = editEntryWidgetButtonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);
    titleEdit->setText("multiline\ntitle");
    editEntryWidget->findChild<QComboBox*>("usernameComboBox")->lineEdit()->setText("multiline\nusername");
    editEntryWidget->findChild<QLineEdit*>("passwordEdit")->setText("multiline\npassword");
    editEntryWidget->findChild<QLineEdit*>("urlEdit")->setText("multiline\nurl");
    QTest::mouseClick(okButton, Qt::LeftButton);

    QCOMPARE(entry->title(), QString("multiline title"));
    QCOMPARE(entry->username(), QString("multiline username"));
    // here we keep newlines, so users can't lock themselves out accidentally
    QCOMPARE(entry->password(), QString("multiline\npassword"));
    QCOMPARE(entry->url(), QString("multiline url"));
}

void TestGui::testSearchEditEntry()
{
    // Regression test for Issue #1447 -- Uses example from issue description

    // Find buttons for group creation
    auto* editGroupWidget = m_dbWidget->findChild<EditGroupWidget*>("editGroupWidget");
    auto* nameEdit = editGroupWidget->findChild<QLineEdit*>("editName");
    auto* editGroupWidgetButtonBox = editGroupWidget->findChild<QDialogButtonBox*>("buttonBox");

    // Add groups "Good" and "Bad"
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "Good");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(m_db->rootGroup()); // Makes "Good" and "Bad" on the same level
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "Bad");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(m_db->rootGroup());

    // Find buttons for entry creation
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryNewWidget = toolBar->widgetForAction(m_mainWindow->findChild<QAction*>("actionEntryNew"));
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");

    // Create "Doggy" in "Good"
    Group* goodGroup = m_dbWidget->currentGroup()->findChildByName(QString("Good"));
    m_dbWidget->groupView()->setCurrentGroup(goodGroup);
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "Doggy");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    // Select "Bad" group in groupView
    Group* badGroup = m_db->rootGroup()->findChildByName(QString("Bad"));
    m_dbWidget->groupView()->setCurrentGroup(badGroup);

    // Search for "Doggy" entry
    auto* searchWidget = toolBar->findChild<SearchWidget*>("SearchWidget");
    auto* searchTextEdit = searchWidget->findChild<QLineEdit*>("searchEdit");
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTest::keyClicks(searchTextEdit, "Doggy");
    QTRY_VERIFY(m_dbWidget->isSearchActive());

    // Goto "Doggy"'s edit view
    QTest::keyClick(searchTextEdit, Qt::Key_Return);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Check the path in header is "parent-group > entry"
    QCOMPARE(m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget")->findChild<QLabel*>("headerLabel")->text(),
             QStringLiteral("Good \u2022 Doggy \u2022 Edit entry"));
}

void TestGui::testAddEntry()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    // Find the new entry action
    auto* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Add entry "test" and confirm added
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    auto* usernameComboBox = editEntryWidget->findChild<QComboBox*>("usernameComboBox");
    QVERIFY(usernameComboBox);
    QTest::mouseClick(usernameComboBox, Qt::LeftButton);
    QTest::keyClicks(usernameComboBox, "AutocompletionUsername");
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->username(), QString("AutocompletionUsername"));
    QCOMPARE(entry->historyItems().size(), 0);

    m_db->updateCommonUsernames();

    // Add entry "something 2"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 2");
    QTest::mouseClick(usernameComboBox, Qt::LeftButton);
    QTest::keyClicks(usernameComboBox, "Auto");
    QTest::keyPress(usernameComboBox, Qt::Key_Right);
    auto* passwordEdit = editEntryWidget->findChild<QLineEdit*>("passwordEdit");
    QTest::keyClicks(passwordEdit, "something 2");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    item = entryView->model()->index(1, 1);
    entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("something 2"));
    QCOMPARE(entry->username(), QString("AutocompletionUsername"));
    QCOMPARE(entry->historyItems().size(), 0);

    // Add entry "something 5" but click cancel button (does NOT add entry)
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 5");
    MessageBox::setNextAnswer(MessageBox::Discard);
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Cancel), Qt::LeftButton);

    QApplication::processEvents();

    // Confirm entry count
    QTRY_COMPARE(entryView->model()->rowCount(), 3);
}

void TestGui::testPasswordEntryEntropy()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Find the new entry action
    auto* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Add entry "test" and confirm added
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    // Open the password generator
    auto* passwordEdit = editEntryWidget->findChild<PasswordEdit*>();
    QVERIFY(passwordEdit);
    QTest::mouseClick(passwordEdit, Qt::LeftButton);

    QTest::keyClick(passwordEdit, Qt::Key_G, Qt::ControlModifier);

    TEST_MODAL(PasswordGeneratorWidget * pwGeneratorWidget;
               QTRY_VERIFY(pwGeneratorWidget = m_dbWidget->findChild<PasswordGeneratorWidget*>());

               // Type in some password
               auto* generatedPassword = pwGeneratorWidget->findChild<QLineEdit*>("editNewPassword");
               auto* entropyLabel = pwGeneratorWidget->findChild<QLabel*>("entropyLabel");
               auto* strengthLabel = pwGeneratorWidget->findChild<QLabel*>("strengthLabel");

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "hello");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 6.38 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "helloworld");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 13.10 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "password1");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 4.00 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "D0g..................");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 19.02 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "Tr0ub4dour&3");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 30.87 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "correcthorsebatterystaple");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 47.98 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Weak"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "YQC3kbXbjC652dTDH");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 95.83 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Good"));

               generatedPassword->setText("");
               QTest::keyClicks(generatedPassword, "Bs5ZFfthWzR8DGFEjaCM6bGqhmCT4km");
               QCOMPARE(entropyLabel->text(), QString("Entropy: 174.59 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Excellent"));

               QTest::mouseClick(generatedPassword, Qt::LeftButton);
               QTest::keyClick(generatedPassword, Qt::Key_Escape););
}

void TestGui::testDicewareEntryEntropy()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Find the new entry action
    auto* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Add entry "test" and confirm added
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    // Open the password generator
    auto* passwordEdit = editEntryWidget->findChild<PasswordEdit*>();
    QVERIFY(passwordEdit);
    QTest::mouseClick(passwordEdit, Qt::LeftButton);

    QTest::keyClick(passwordEdit, Qt::Key_G, Qt::ControlModifier);

    TEST_MODAL(PasswordGeneratorWidget * pwGeneratorWidget;
               QTRY_VERIFY(pwGeneratorWidget = m_dbWidget->findChild<PasswordGeneratorWidget*>());

               // Select Diceware
               auto* generatedPassword = pwGeneratorWidget->findChild<QLineEdit*>("editNewPassword");
               auto* tabWidget = pwGeneratorWidget->findChild<QTabWidget*>("tabWidget");
               auto* dicewareWidget = pwGeneratorWidget->findChild<QWidget*>("dicewareWidget");
               tabWidget->setCurrentWidget(dicewareWidget);

               auto* comboBoxWordList = dicewareWidget->findChild<QComboBox*>("comboBoxWordList");
               comboBoxWordList->setCurrentText("eff_large.wordlist");
               auto* spinBoxWordCount = dicewareWidget->findChild<QSpinBox*>("spinBoxWordCount");
               spinBoxWordCount->setValue(6);

               // Confirm a password was generated
               QVERIFY(!pwGeneratorWidget->getGeneratedPassword().isEmpty());

               // Verify entropy and strength
               auto* entropyLabel = pwGeneratorWidget->findChild<QLabel*>("entropyLabel");
               auto* strengthLabel = pwGeneratorWidget->findChild<QLabel*>("strengthLabel");

               QCOMPARE(entropyLabel->text(), QString("Entropy: 77.55 bit"));
               QCOMPARE(strengthLabel->text(), QString("Password Quality: Good"));

               QTest::mouseClick(generatedPassword, Qt::LeftButton);
               QTest::keyClick(generatedPassword, Qt::Key_Escape););
}

void TestGui::testTotp()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    QCOMPARE(entryView->model()->rowCount(), 1);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);
    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntrySetupTotp");

    auto* setupTotpDialog = m_dbWidget->findChild<TotpSetupDialog*>("TotpSetupDialog");

    QApplication::processEvents();

    QString exampleSeed = "gezd gnbvgY 3tqojqGEZdgnb vgy3tqoJq===";
    QString expectedFinalSeed = exampleSeed.toUpper().remove(" ").remove("=");
    auto* seedEdit = setupTotpDialog->findChild<QLineEdit*>("seedEdit");
    seedEdit->setText("");
    QTest::keyClicks(seedEdit, exampleSeed);

    auto* setupTotpButtonBox = setupTotpDialog->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(setupTotpButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTRY_VERIFY(!setupTotpDialog->isVisible());

    // Make sure the entryView is selected and active
    entryView->activateWindow();
    QApplication::processEvents();
    QTRY_VERIFY(entryView->hasFocus());

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    editEntryWidget->setCurrentPage(1);
    auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("revealAttributeButton"), Qt::LeftButton);
    QCOMPARE(attrTextEdit->toPlainText(), expectedFinalSeed);

    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    triggerAction("actionEntryTotp");

    auto* totpDialog = m_dbWidget->findChild<TotpDialog*>("TotpDialog");
    auto* totpLabel = totpDialog->findChild<QLabel*>("totpLabel");

    QCOMPARE(totpLabel->text().replace(" ", ""), entry->totp());
}

void TestGui::testSearch()
{
    // Add canned entries for consistent testing
    Q_UNUSED(addCannedEntries());

    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    auto* searchWidget = toolBar->findChild<SearchWidget*>("SearchWidget");
    QVERIFY(searchWidget->isEnabled());
    auto* searchTextEdit = searchWidget->findChild<QLineEdit*>("searchEdit");

    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QVERIFY(entryView->isVisible());

    QVERIFY(searchTextEdit->isClearButtonEnabled());

    auto* helpButton = searchWidget->findChild<QAction*>("helpIcon");
    auto* helpPanel = searchWidget->findChild<QWidget*>("SearchHelpWidget");
    QVERIFY(helpButton->isVisible());
    QVERIFY(!helpPanel->isVisible());

    // Enter search
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    // Show/Hide search help
    helpButton->trigger();
    QTRY_VERIFY(helpPanel->isVisible());
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(helpPanel->isVisible());
    helpButton->trigger();
    QTRY_VERIFY(!helpPanel->isVisible());
    // Search for "ZZZ"
    QTest::keyClicks(searchTextEdit, "ZZZ");
    QTRY_COMPARE(searchTextEdit->text(), QString("ZZZ"));
    QTRY_VERIFY(m_dbWidget->isSearchActive());
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // Press the search clear button
    searchTextEdit->clear();
    QTRY_VERIFY(searchTextEdit->text().isEmpty());
    QTRY_VERIFY(searchTextEdit->hasFocus());
    // Escape clears searchedit and retains focus
    QTest::keyClicks(searchTextEdit, "ZZZ");
    QTest::keyClick(searchTextEdit, Qt::Key_Escape);
    QTRY_VERIFY(searchTextEdit->text().isEmpty());
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    // Search for "some"
    QTest::keyClicks(searchTextEdit, "some");
    QTRY_VERIFY(m_dbWidget->isSearchActive());
    QTRY_COMPARE(entryView->model()->rowCount(), 3);
    // Search for "someTHING"
    QTest::keyClicks(searchTextEdit, "THING");
    QTRY_COMPARE(entryView->model()->rowCount(), 2);
    // Press Down to focus on the entry view
    QTest::keyClick(searchTextEdit, Qt::Key_Right, Qt::ControlModifier);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTest::keyClick(searchTextEdit, Qt::Key_Down);
    QTRY_VERIFY(entryView->hasFocus());
    auto* searchedEntry = entryView->currentEntry();
    // Restore focus using F3 key and search text selection
    QTest::keyClick(m_mainWindow.data(), Qt::Key_F3);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTRY_COMPARE(searchTextEdit->selectedText(), QString("someTHING"));

    searchedEntry->setPassword("password");
    QClipboard* clipboard = QApplication::clipboard();

    // Attempt password copy with selected test (should fail)
    QTest::keyClick(searchTextEdit, Qt::Key_C, Qt::ControlModifier);
    QVERIFY(clipboard->text() != searchedEntry->password());
    // Deselect text and confirm password copies
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->selectedText().isEmpty());
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTest::keyClick(searchTextEdit, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(searchedEntry->password(), clipboard->text());
    // Ensure Down focuses on entry view when search text is selected
    QTest::keyClick(searchTextEdit, Qt::Key_A, Qt::ControlModifier);
    QTest::keyClick(searchTextEdit, Qt::Key_Down);
    QTRY_VERIFY(entryView->hasFocus());
    QCOMPARE(entryView->currentEntry(), searchedEntry);
    // Test that password copies with entry focused
    QTest::keyClick(entryView, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(searchedEntry->password(), clipboard->text());
    // Refocus back to search edit
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    // Test that password does not copy
    searchTextEdit->selectAll();
    QTest::keyClick(searchTextEdit, Qt::Key_C, Qt::ControlModifier);
    QTRY_COMPARE(clipboard->text(), QString("someTHING"));

    // Test case sensitive search
    searchWidget->setCaseSensitive(true);
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    searchWidget->setCaseSensitive(false);
    QTRY_COMPARE(entryView->model()->rowCount(), 2);

    // Test group search
    searchWidget->setLimitGroup(false);
    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QModelIndex rootGroupIndex = groupView->model()->index(0, 0);
    clickIndex(groupView->model()->index(0, 0, rootGroupIndex), groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup()->name(), QString("General"));
    // Selecting a group should cancel search
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // Restore search
    QTest::keyClick(m_mainWindow.data(), Qt::Key_F, Qt::ControlModifier);
    QTest::keyClicks(searchTextEdit, "someTHING");
    QTRY_COMPARE(entryView->model()->rowCount(), 2);
    // Enable group limiting
    searchWidget->setLimitGroup(true);
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // Selecting another group should NOT cancel search
    clickIndex(rootGroupIndex, groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QTRY_COMPARE(entryView->model()->rowCount(), 2);

    // reset
    searchWidget->setLimitGroup(false);
    clickIndex(rootGroupIndex, groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QVERIFY(!m_dbWidget->isSearchActive());

    // Try to edit the first entry from the search view
    // Refocus back to search edit
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTest::keyClicks(searchTextEdit, "someTHING");
    QTRY_VERIFY(m_dbWidget->isSearchActive());

    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);
    QTest::keyClick(searchTextEdit, Qt::Key_Return);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Perform the edit and save it
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QString origTitle = titleEdit->text();
    QTest::keyClicks(titleEdit, "_edited");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm the edit was made and we are back in search mode
    QTRY_VERIFY(m_dbWidget->isSearchActive());
    QCOMPARE(entry->title(), origTitle.append("_edited"));

    // Cancel search, should return to normal view
    QTest::keyClick(m_mainWindow.data(), Qt::Key_Escape);
    QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
}

void TestGui::testDeleteEntry()
{
    // Add canned entries for consistent testing
    Q_UNUSED(addCannedEntries());

    auto* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");
    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);
    entryView->setFocus();

    // Move one entry to the recycling bin
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    clickIndex(entryView->model()->index(1, 1), entryView, Qt::LeftButton);
    QVERIFY(entryDeleteWidget->isVisible());
    QVERIFY(entryDeleteWidget->isEnabled());
    QVERIFY(!m_db->metadata()->recycleBin());

    MessageBox::setNextAnswer(MessageBox::Move);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    // Select multiple entries and move them to the recycling bin
    clickIndex(entryView->model()->index(1, 1), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(2, 1), entryView, Qt::LeftButton, Qt::ControlModifier);
    QCOMPARE(entryView->selectionModel()->selectedRows().size(), 2);

    MessageBox::setNextAnswer(MessageBox::Cancel);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    MessageBox::setNextAnswer(MessageBox::Move);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 1);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    // Go to the recycling bin
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QModelIndex rootGroupIndex = groupView->model()->index(0, 0);
    clickIndex(groupView->model()->index(groupView->model()->rowCount(rootGroupIndex) - 1, 0, rootGroupIndex),
               groupView,
               Qt::LeftButton);
    QCOMPARE(groupView->currentGroup()->name(), m_db->metadata()->recycleBin()->name());

    // Delete one entry from the bin
    clickIndex(entryView->model()->index(0, 1), entryView, Qt::LeftButton);
    MessageBox::setNextAnswer(MessageBox::Cancel);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    MessageBox::setNextAnswer(MessageBox::Delete);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 2);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 2);

    // Select the remaining entries and delete them
    clickIndex(entryView->model()->index(0, 1), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(1, 1), entryView, Qt::LeftButton, Qt::ControlModifier);
    MessageBox::setNextAnswer(MessageBox::Delete);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 0);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 0);

    // Ensure the entry preview widget shows the recycling group since all entries are deleted
    auto* previewWidget = m_dbWidget->findChild<EntryPreviewWidget*>("previewWidget");
    QVERIFY(previewWidget);
    auto* groupTitleLabel = previewWidget->findChild<QLabel*>("groupTitleLabel");
    QVERIFY(groupTitleLabel);

    QTRY_VERIFY(groupTitleLabel->isVisible());
    QVERIFY(groupTitleLabel->text().contains(m_db->metadata()->recycleBin()->name()));

    // Go back to the root group
    clickIndex(groupView->model()->index(0, 0), groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
}

void TestGui::testCloneEntry()
{
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    entryView->setFocus();

    QCOMPARE(entryView->model()->rowCount(), 1);

    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entryOrg = entryView->entryFromIndex(item);
    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntryClone");

    auto* cloneDialog = m_dbWidget->findChild<CloneDialog*>("CloneDialog");
    auto* cloneButtonBox = cloneDialog->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(cloneButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 2);
    Entry* entryClone = entryView->entryFromIndex(entryView->model()->index(1, 1));
    QVERIFY(entryOrg->uuid() != entryClone->uuid());
    QCOMPARE(entryClone->title(), entryOrg->title() + QString(" - Clone"));
}

void TestGui::testEntryPlaceholders()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    // Find the new entry action
    auto* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    // Add entry "test" and confirm added
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    QComboBox* usernameComboBox = editEntryWidget->findChild<QComboBox*>("usernameComboBox");
    QTest::keyClicks(usernameComboBox, "john");
    QLineEdit* urlEdit = editEntryWidget->findChild<QLineEdit*>("urlEdit");
    QTest::keyClicks(urlEdit, "{TITLE}.{USERNAME}");
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 2);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->url(), QString("{TITLE}.{USERNAME}"));

    // Test password copy
    QClipboard* clipboard = QApplication::clipboard();
    m_dbWidget->copyURL();
    QTRY_COMPARE(clipboard->text(), QString("test.john"));
}

void TestGui::testDragAndDropEntry()
{
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    auto* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    QAbstractItemModel* groupModel = groupView->model();

    QModelIndex sourceIndex = entryView->model()->index(0, 1);
    QModelIndex targetIndex = groupModel->index(0, 0, groupModel->index(0, 0));
    QVERIFY(sourceIndex.isValid());
    QVERIFY(targetIndex.isValid());

    QMimeData mimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    Entry* entry = entryView->entryFromIndex(sourceIndex);
    stream << entry->group()->database()->uuid() << entry->uuid();
    mimeData.setData("application/x-keepassx-entry", encoded);

    QVERIFY(groupModel->dropMimeData(&mimeData, Qt::MoveAction, -1, 0, targetIndex));
    QCOMPARE(entry->group()->name(), QString("General"));
}

void TestGui::testDragAndDropGroup()
{
    QAbstractItemModel* groupModel = m_dbWidget->findChild<GroupView*>("groupView")->model();
    QModelIndex rootIndex = groupModel->index(0, 0);

    dragAndDropGroup(groupModel->index(0, 0, rootIndex), groupModel->index(1, 0, rootIndex), -1, true, "Windows", 0);

    // dropping parent on child is supposed to fail
    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     groupModel->index(0, 0, groupModel->index(0, 0, rootIndex)),
                     -1,
                     false,
                     "NewDatabase",
                     0);

    dragAndDropGroup(groupModel->index(1, 0, rootIndex), rootIndex, 0, true, "NewDatabase", 0);

    dragAndDropGroup(groupModel->index(0, 0, rootIndex), rootIndex, -1, true, "NewDatabase", 4);
}

void TestGui::testSaveAs()
{
    QFileInfo fileInfo(m_dbFilePath);
    QDateTime lastModified = fileInfo.lastModified();

    m_db->metadata()->setName("testSaveAs");

    // open temporary file so it creates a filename
    TemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QString tmpFileName = tmpFile.fileName();
    tmpFile.remove();

    fileDialog()->setNextFileName(tmpFileName);

    triggerAction("actionDatabaseSaveAs");

    QCOMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), QString("testSaveAs"));

    checkDatabase(tmpFileName);

    fileInfo.refresh();
    QCOMPARE(fileInfo.lastModified(), lastModified);
    tmpFile.remove();
}

void TestGui::testSaveBackup()
{
    m_db->metadata()->setName("testSaveBackup");

    QFileInfo fileInfo(m_dbFilePath);
    QDateTime lastModified = fileInfo.lastModified();

    // open temporary file so it creates a filename
    TemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QString tmpFileName = tmpFile.fileName();
    tmpFile.remove();

    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("testSaveBackup*"));

    fileDialog()->setNextFileName(tmpFileName);

    triggerAction("actionDatabaseSaveBackup");

    QCOMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), QString("testSaveBackup*"));

    checkDatabase(tmpFileName);

    fileInfo.refresh();
    QCOMPARE(fileInfo.lastModified(), lastModified);
    tmpFile.remove();
}

void TestGui::testSave()
{
    m_db->metadata()->setName("testSave");

    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("testSave*"));

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), QString("testSave"));

    checkDatabase();
}

void TestGui::testDatabaseSettings()
{
    m_db->metadata()->setName("testDatabaseSettings");
    triggerAction("actionDatabaseSettings");
    auto* dbSettingsDialog = m_dbWidget->findChild<QWidget*>("databaseSettingsDialog");
    auto* transformRoundsSpinBox = dbSettingsDialog->findChild<QSpinBox*>("transformRoundsSpinBox");
    auto advancedToggle = dbSettingsDialog->findChild<QCheckBox*>("advancedSettingsToggle");

    advancedToggle->setChecked(true);
    QApplication::processEvents();

    QVERIFY(transformRoundsSpinBox != nullptr);
    transformRoundsSpinBox->setValue(123456);
    QTest::keyClick(transformRoundsSpinBox, Qt::Key_Enter);
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("testDatabaseSettings*"));
    QCOMPARE(m_db->kdf()->rounds(), 123456);

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("testDatabaseSettings"));

    advancedToggle->setChecked(false);
    QApplication::processEvents();

    checkDatabase();
}

void TestGui::testKeePass1Import()
{
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb"));
    triggerAction("actionImportKeePass1");

    auto* keepass1OpenWidget = m_tabWidget->currentDatabaseWidget()->findChild<QWidget*>("keepass1OpenWidget");
    auto* editPassword = keepass1OpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "masterpw");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QTRY_COMPARE(m_tabWidget->count(), 2);
    QTRY_COMPARE(m_tabWidget->tabName(m_tabWidget->currentIndex()), QString("basic [New Database]*"));

    // Close the KeePass1 Database
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");
    QApplication::processEvents();
}

void TestGui::testDatabaseLocking()
{
    QString origDbName = m_tabWidget->tabText(0);

    MessageBox::setNextAnswer(MessageBox::Cancel);
    triggerAction("actionLockDatabases");

    QCOMPARE(m_tabWidget->tabName(0), origDbName + " [Locked]");

    auto* actionDatabaseMerge = m_mainWindow->findChild<QAction*>("actionDatabaseMerge", Qt::FindChildrenRecursively);
    QCOMPARE(actionDatabaseMerge->isEnabled(), false);
    auto* actionDatabaseSave = m_mainWindow->findChild<QAction*>("actionDatabaseSave", Qt::FindChildrenRecursively);
    QCOMPARE(actionDatabaseSave->isEnabled(), false);

    DatabaseWidget* dbWidget = m_tabWidget->currentDatabaseWidget();
    QVERIFY(dbWidget->isLocked());
    auto* unlockDatabaseWidget = dbWidget->findChild<QWidget*>("databaseOpenWidget");
    QWidget* editPassword = unlockDatabaseWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QVERIFY(!dbWidget->isLocked());
    QCOMPARE(m_tabWidget->tabName(0), origDbName);

    actionDatabaseMerge = m_mainWindow->findChild<QAction*>("actionDatabaseMerge", Qt::FindChildrenRecursively);
    QCOMPARE(actionDatabaseMerge->isEnabled(), true);
}

void TestGui::testDragAndDropKdbxFiles()
{
    const int openedDatabasesCount = m_tabWidget->count();

    const QString badDatabaseFilePath(QString(KEEPASSX_TEST_DATA_DIR).append("/NotDatabase.notkdbx"));
    QMimeData badMimeData;
    badMimeData.setUrls({QUrl::fromLocalFile(badDatabaseFilePath)});
    QDragEnterEvent badDragEvent(QPoint(1, 1), Qt::LinkAction, &badMimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->notify(m_mainWindow.data(), &badDragEvent);
    QCOMPARE(badDragEvent.isAccepted(), false);

    QDropEvent badDropEvent(QPoint(1, 1), Qt::LinkAction, &badMimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->notify(m_mainWindow.data(), &badDropEvent);
    QCOMPARE(badDropEvent.isAccepted(), false);

    QCOMPARE(m_tabWidget->count(), openedDatabasesCount);

    QMimeData goodMimeData;
    goodMimeData.setUrls({QUrl::fromLocalFile(dbFileName)});
    QDragEnterEvent goodDragEvent(QPoint(1, 1), Qt::LinkAction, &goodMimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->notify(m_mainWindow.data(), &goodDragEvent);
    QCOMPARE(goodDragEvent.isAccepted(), true);

    QDropEvent goodDropEvent(QPoint(1, 1), Qt::LinkAction, &goodMimeData, Qt::LeftButton, Qt::NoModifier);
    qApp->notify(m_mainWindow.data(), &goodDropEvent);
    QCOMPARE(goodDropEvent.isAccepted(), true);

    QCOMPARE(m_tabWidget->count(), openedDatabasesCount + 1);

    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");

    QTRY_COMPARE(m_tabWidget->count(), openedDatabasesCount);
}

void TestGui::testSortGroups()
{
    auto* editGroupWidget = m_dbWidget->findChild<EditGroupWidget*>("editGroupWidget");
    auto* nameEdit = editGroupWidget->findChild<QLineEdit*>("editName");
    auto* editGroupWidgetButtonBox = editGroupWidget->findChild<QDialogButtonBox*>("buttonBox");

    // Create some sub-groups
    Group* rootGroup = m_db->rootGroup();
    Group* internetGroup = rootGroup->findGroupByPath("Internet");
    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "Google");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "eBay");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "Amazon");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    m_dbWidget->createGroup();
    QTest::keyClicks(nameEdit, "Facebook");
    QTest::mouseClick(editGroupWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    m_dbWidget->groupView()->setCurrentGroup(rootGroup);

    triggerAction("actionGroupSortAsc");
    QList<Group*> children = rootGroup->children();
    QCOMPARE(children[0]->name(), QString("eMail"));
    QCOMPARE(children[1]->name(), QString("General"));
    QCOMPARE(children[2]->name(), QString("Homebanking"));
    QCOMPARE(children[3]->name(), QString("Internet"));
    QCOMPARE(children[4]->name(), QString("Network"));
    QCOMPARE(children[5]->name(), QString("Windows"));
    QList<Group*> subChildren = internetGroup->children();
    QCOMPARE(subChildren[0]->name(), QString("Amazon"));
    QCOMPARE(subChildren[1]->name(), QString("eBay"));
    QCOMPARE(subChildren[2]->name(), QString("Facebook"));
    QCOMPARE(subChildren[3]->name(), QString("Google"));

    triggerAction("actionGroupSortDesc");
    children = rootGroup->children();
    QCOMPARE(children[0]->name(), QString("Windows"));
    QCOMPARE(children[1]->name(), QString("Network"));
    QCOMPARE(children[2]->name(), QString("Internet"));
    QCOMPARE(children[3]->name(), QString("Homebanking"));
    QCOMPARE(children[4]->name(), QString("General"));
    QCOMPARE(children[5]->name(), QString("eMail"));
    subChildren = internetGroup->children();
    QCOMPARE(subChildren[0]->name(), QString("Google"));
    QCOMPARE(subChildren[1]->name(), QString("Facebook"));
    QCOMPARE(subChildren[2]->name(), QString("eBay"));
    QCOMPARE(subChildren[3]->name(), QString("Amazon"));

    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    triggerAction("actionGroupSortAsc");
    children = rootGroup->children();
    QCOMPARE(children[0]->name(), QString("Windows"));
    QCOMPARE(children[1]->name(), QString("Network"));
    QCOMPARE(children[2]->name(), QString("Internet"));
    QCOMPARE(children[3]->name(), QString("Homebanking"));
    QCOMPARE(children[4]->name(), QString("General"));
    QCOMPARE(children[5]->name(), QString("eMail"));
    subChildren = internetGroup->children();
    QCOMPARE(subChildren[0]->name(), QString("Amazon"));
    QCOMPARE(subChildren[1]->name(), QString("eBay"));
    QCOMPARE(subChildren[2]->name(), QString("Facebook"));
    QCOMPARE(subChildren[3]->name(), QString("Google"));

    m_dbWidget->groupView()->setCurrentGroup(rootGroup);
    triggerAction("actionGroupSortAsc");
    m_dbWidget->groupView()->setCurrentGroup(internetGroup);
    triggerAction("actionGroupSortDesc");
    children = rootGroup->children();
    QCOMPARE(children[0]->name(), QString("eMail"));
    QCOMPARE(children[1]->name(), QString("General"));
    QCOMPARE(children[2]->name(), QString("Homebanking"));
    QCOMPARE(children[3]->name(), QString("Internet"));
    QCOMPARE(children[4]->name(), QString("Network"));
    QCOMPARE(children[5]->name(), QString("Windows"));
    subChildren = internetGroup->children();
    QCOMPARE(subChildren[0]->name(), QString("Google"));
    QCOMPARE(subChildren[1]->name(), QString("Facebook"));
    QCOMPARE(subChildren[2]->name(), QString("eBay"));
    QCOMPARE(subChildren[3]->name(), QString("Amazon"));
}

void TestGui::testTrayRestoreHide()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QSKIP("QSystemTrayIcon::isSystemTrayAvailable() = false, skipping tray restore/hide test...");
    }

    m_mainWindow->hideWindow();
    QVERIFY(!m_mainWindow->isVisible());

    auto* trayIcon = m_mainWindow->findChild<QSystemTrayIcon*>();
    QVERIFY(trayIcon);

    trayIcon->activated(QSystemTrayIcon::Trigger);
    QTRY_VERIFY(m_mainWindow->isVisible());

    trayIcon->activated(QSystemTrayIcon::Trigger);
    QTRY_VERIFY(!m_mainWindow->isVisible());

    trayIcon->activated(QSystemTrayIcon::MiddleClick);
    QTRY_VERIFY(m_mainWindow->isVisible());

    trayIcon->activated(QSystemTrayIcon::MiddleClick);
    QTRY_VERIFY(!m_mainWindow->isVisible());

    trayIcon->activated(QSystemTrayIcon::DoubleClick);
    QTRY_VERIFY(m_mainWindow->isVisible());

    trayIcon->activated(QSystemTrayIcon::DoubleClick);
    QTRY_VERIFY(!m_mainWindow->isVisible());
}

int TestGui::addCannedEntries()
{
    int entries_added = 0;

    // Find buttons
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryNewWidget = toolBar->widgetForAction(m_mainWindow->findChild<QAction*>("actionEntryNew"));
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    auto* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    auto* passwordEdit = editEntryWidget->findChild<QLineEdit*>("passwordEdit");

    // Add entry "test" and confirm added
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "test");
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    ++entries_added;

    // Add entry "something 2"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 2");
    QTest::keyClicks(passwordEdit, "something 2");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    ++entries_added;

    // Add entry "something 3"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 3");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    ++entries_added;

    return entries_added;
}

void TestGui::checkDatabase(QString dbFileName)
{
    if (dbFileName.isEmpty()) {
        dbFileName = m_dbFilePath;
    }

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    auto dbSaved = QSharedPointer<Database>::create();
    QVERIFY(dbSaved->open(dbFileName, key, nullptr, false));
    QCOMPARE(dbSaved->metadata()->name(), m_db->metadata()->name());
}

void TestGui::triggerAction(const QString& name)
{
    auto* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QApplication::processEvents();
}

void TestGui::dragAndDropGroup(const QModelIndex& sourceIndex,
                               const QModelIndex& targetIndex,
                               int row,
                               bool expectedResult,
                               const QString& expectedParentName,
                               int expectedPos)
{
    QVERIFY(sourceIndex.isValid());
    QVERIFY(targetIndex.isValid());

    auto groupModel = qobject_cast<GroupModel*>(m_dbWidget->findChild<GroupView*>("groupView")->model());

    QMimeData mimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    Group* group = groupModel->groupFromIndex(sourceIndex);
    stream << group->database()->uuid() << group->uuid();
    mimeData.setData("application/x-keepassx-group", encoded);

    QCOMPARE(groupModel->dropMimeData(&mimeData, Qt::MoveAction, row, 0, targetIndex), expectedResult);
    QCOMPARE(group->parentGroup()->name(), expectedParentName);
    QCOMPARE(group->parentGroup()->children().indexOf(group), expectedPos);
}

void TestGui::clickIndex(const QModelIndex& index,
                         QAbstractItemView* view,
                         Qt::MouseButton button,
                         Qt::KeyboardModifiers stateKey)
{
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}

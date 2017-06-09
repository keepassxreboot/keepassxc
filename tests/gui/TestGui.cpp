/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestGui.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QTemporaryFile>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QSignalSpy>
#include <QClipboard>
#include <QDebug>

#include "config-keepassx-tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/CloneDialog.h"
#include "gui/TotpDialog.h"
#include "gui/SetupTotpDialog.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/SearchWidget.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/GroupModel.h"
#include "gui/group/GroupView.h"
#include "keys/PasswordKey.h"

void TestGui::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    m_mainWindow = new MainWindow();
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();
    m_mainWindow->activateWindow();
    Tools::wait(50);

    // Load the NewDatabase.kdbx file into temporary storage
    QByteArray tmpData;
    QFile sourceDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, m_dbData));
    sourceDbFile.close();
}

// Every test starts with opening the temp database
void TestGui::init()
{
    // Write the temp storage to a temp database file for use in our tests
    QVERIFY(m_dbFile.open());
    QCOMPARE(m_dbFile.write(m_dbData), static_cast<qint64>((m_dbData.size())));
    m_dbFile.close();

    m_dbFileName = m_dbFile.fileName();
    m_dbFilePath = m_dbFile.filePath();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");

    QWidget* databaseOpenWidget = m_mainWindow->findChild<QWidget*>("databaseOpenWidget");
    QLineEdit* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);
    Tools::wait(100);

    QVERIFY(m_tabWidget->currentDatabaseWidget());

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

// Every test ends with closing the temp database without saving
void TestGui::cleanup()
{
    // DO NOT save the database
    MessageBox::setNextAnswer(QMessageBox::No);
    triggerAction("actionDatabaseClose");
    Tools::wait(100);

    m_db = nullptr;
    m_dbWidget = nullptr;
}

void TestGui::testMergeDatabase()
{
    // It is safe to ignore the warning this line produces
    QSignalSpy dbMergeSpy(m_dbWidget, SIGNAL(databaseMerged(Database*)));

    // set file to merge from
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx"));
    triggerAction("actionDatabaseMerge");

    QWidget* databaseOpenMergeWidget = m_mainWindow->findChild<QWidget*>("databaseOpenMergeWidget");
    QLineEdit* editPasswordMerge = databaseOpenMergeWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPasswordMerge->isVisible());

    m_tabWidget->currentDatabaseWidget()->setCurrentWidget(databaseOpenMergeWidget);

    QTest::keyClicks(editPasswordMerge, "a");
    QTest::keyClick(editPasswordMerge, Qt::Key_Enter);

    QTRY_COMPARE(dbMergeSpy.count(), 1);
    QTRY_VERIFY(m_tabWidget->tabText(m_tabWidget->currentIndex()).contains("*"));

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
    config()->set("AutoReloadOnChange", false);

    // Load the MergeDatabase.kdbx file into temporary storage
    QByteArray tmpData;
    QFile mergeDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx"));
    QVERIFY(mergeDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&mergeDbFile, tmpData));
    mergeDbFile.close();

    // Test accepting new file in autoreload
    MessageBox::setNextAnswer(QMessageBox::Yes);
    // Overwrite the current database with the temp data
    QVERIFY(m_dbFile.open());
    QVERIFY(m_dbFile.write(tmpData, static_cast<qint64>(tmpData.size())));
    m_dbFile.close();
    Tools::wait(1500);

    m_db = m_dbWidget->database();

    // the General group contains one entry from the new db data
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
    QVERIFY(! m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));

    // Reset the state
    cleanup();
    init();

    // Test rejecting new file in autoreload
    MessageBox::setNextAnswer(QMessageBox::No);
    // Overwrite the current temp database with a new file
    m_dbFile.open();
    QVERIFY(m_dbFile.write(tmpData, static_cast<qint64>(tmpData.size())));
    m_dbFile.close();
    Tools::wait(1500);

    m_db = m_dbWidget->database();

    // Ensure the merge did not take place
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 0);
    QVERIFY(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));

    // Reset the state
    cleanup();
    init();

     // Test accepting a merge of edits into autoreload
    // Turn on autoload so we only get one messagebox (for the merge)
    config()->set("AutoReloadOnChange", true);

    // Modify some entries
    testEditEntry();

    // This is saying yes to merging the entries
    MessageBox::setNextAnswer(QMessageBox::Yes);
    // Overwrite the current database with the temp data
    QVERIFY(m_dbFile.open());
    QVERIFY(m_dbFile.write(tmpData, static_cast<qint64>(tmpData.size())));
    m_dbFile.close();
    Tools::wait(1500);

    m_db = m_dbWidget->database();

    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
    QVERIFY(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));
}

void TestGui::testTabs()
{
    QCOMPARE(m_tabWidget->count(), 1);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), m_dbFileName);
}

void TestGui::testEditEntry()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Select the first entry in the database
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QModelIndex entryItem = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(entryItem);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    // Confirm the edit action button is enabled
    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());

    // Edit the first entry ("Sample Entry")
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "_test");

    // Apply the edit
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Apply), Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    QCOMPARE(entry->title(), QString("Sample Entry_test"));
    QCOMPARE(entry->historyItems().size(), 1);

    // Test protected attributes
    editEntryWidget->setCurrentPage(1);
    QPlainTextEdit* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
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
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm edit was made
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QCOMPARE(entry->title(), QString("Sample Entry_test"));
    QCOMPARE(entry->historyItems().size(), 2);

    // Confirm modified indicator is showing
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("%1*").arg(m_dbFileName));
}

void TestGui::testAddEntry()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->historyItems().size(), 0);

    // Add entry "something 2"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 2");
    QLineEdit* passwordEdit = editEntryWidget->findChild<QLineEdit*>("passwordEdit");
    QLineEdit* passwordRepeatEdit = editEntryWidget->findChild<QLineEdit*>("passwordRepeatEdit");
    QTest::keyClicks(passwordEdit, "something 2");
    QTest::keyClicks(passwordRepeatEdit, "something 2");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Add entry "something 3"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 3");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QApplication::processEvents();

    // Confirm that 4 entries now exist
    QTRY_COMPARE(entryView->model()->rowCount(), 4);
}

void TestGui::testPasswordEntryEntropy()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    // Open the password generator
    QToolButton* generatorButton = editEntryWidget->findChild<QToolButton*>("togglePasswordGeneratorButton");
    QTest::mouseClick(generatorButton, Qt::LeftButton);

    // Type in some password
    QLineEdit* editNewPassword = editEntryWidget->findChild<QLineEdit*>("editNewPassword");
    QLabel* entropyLabel = editEntryWidget->findChild<QLabel*>("entropyLabel");
    QLabel* strengthLabel = editEntryWidget->findChild<QLabel*>("strengthLabel");

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "hello");
    QCOMPARE(entropyLabel->text(), QString("Entropy: 6.38 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "helloworld");
    QCOMPARE(entropyLabel->text(), QString("Entropy: 13.10 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "password1");
    QCOMPARE(entropyLabel->text(), QString("Entropy: 4.00 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "D0g..................");
    QCOMPARE(entropyLabel->text(), QString("Entropy: 19.02 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "Tr0ub4dour&3");
    QCOMPARE(entropyLabel->text(), QString("Entropy: 30.87 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Poor"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "correcthorsebatterystaple");
    QCOMPARE(entropyLabel->text(),  QString("Entropy: 47.98 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Weak"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "YQC3kbXbjC652dTDH");
    QCOMPARE(entropyLabel->text(),  QString("Entropy: 96.07 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Good"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "Bs5ZFfthWzR8DGFEjaCM6bGqhmCT4km");
    QCOMPARE(entropyLabel->text(),  QString("Entropy: 174.59 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Excellent"));

    // We are done
}

void TestGui::testDicewareEntryEntropy()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    // Open the password generator
    QToolButton* generatorButton = editEntryWidget->findChild<QToolButton*>("togglePasswordGeneratorButton");
    QTest::mouseClick(generatorButton, Qt::LeftButton);

    // Select Diceware
    QTabWidget* tabWidget = editEntryWidget->findChild<QTabWidget*>("tabWidget");
    QWidget* dicewareWidget = editEntryWidget->findChild<QWidget*>("dicewareWidget");
    tabWidget->setCurrentWidget(dicewareWidget);

    QComboBox* comboBoxWordList = dicewareWidget->findChild<QComboBox*>("comboBoxWordList");
    comboBoxWordList->setCurrentText("eff_large.wordlist");
    QSpinBox* spinBoxWordCount = dicewareWidget->findChild<QSpinBox*>("spinBoxWordCount");
    spinBoxWordCount->setValue(6);

    // Type in some password
    QLabel* entropyLabel = editEntryWidget->findChild<QLabel*>("entropyLabel");
    QLabel* strengthLabel = editEntryWidget->findChild<QLabel*>("strengthLabel");

    QCOMPARE(entropyLabel->text(),  QString("Entropy: 77.55 bit"));
    QCOMPARE(strengthLabel->text(), QString("Password Quality: Good"));
}

void TestGui::testTotp()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    QCOMPARE(entryView->model()->rowCount(), 1);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);

    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntrySetupTotp");

    SetupTotpDialog* setupTotpDialog = m_dbWidget->findChild<SetupTotpDialog*>("SetupTotpDialog");

    Tools::wait(100);

    QLineEdit* seedEdit = setupTotpDialog->findChild<QLineEdit*>("seedEdit");

    QString exampleSeed = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";
    QTest::keyClicks(seedEdit, exampleSeed);

    QDialogButtonBox* setupTotpButtonBox = setupTotpDialog->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(setupTotpButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");

    editEntryWidget->setCurrentPage(1);
    QPlainTextEdit* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("revealAttributeButton"), Qt::LeftButton);
    QCOMPARE(attrTextEdit->toPlainText(), exampleSeed);

    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    triggerAction("actionEntryTotp");

    TotpDialog* totpDialog = m_dbWidget->findChild<TotpDialog*>("TotpDialog");
    QLabel* totpLabel = totpDialog->findChild<QLabel*>("totpLabel");

    QCOMPARE(totpLabel->text().replace(" ", ""), entry->totp());
}

void TestGui::testSearch()
{
    // Add canned entries for consistent testing
    testAddEntry();

    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    SearchWidget* searchWidget = toolBar->findChild<SearchWidget*>("SearchWidget");
    QVERIFY(searchWidget->isEnabled());
    QLineEdit* searchTextEdit = searchWidget->findChild<QLineEdit*>("searchEdit");

    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QVERIFY(entryView->isVisible());

    QAction* clearButton = searchWidget->findChild<QAction*>("clearIcon");
    QVERIFY(!clearButton->isVisible());

    // Enter search
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTRY_VERIFY(!clearButton->isVisible());
    // Search for "ZZZ"
    QTest::keyClicks(searchTextEdit, "ZZZ");
    QTRY_COMPARE(searchTextEdit->text(), QString("ZZZ"));
    QTRY_VERIFY(clearButton->isVisible());
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // Press the search clear button
    clearButton->trigger();
    QTRY_VERIFY(searchTextEdit->text().isEmpty());
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTRY_VERIFY(!clearButton->isVisible());
    // Escape clears searchedit and retains focus
    QTest::keyClicks(searchTextEdit, "ZZZ");
    QTest::keyClick(searchTextEdit, Qt::Key_Escape);
    QTRY_VERIFY(searchTextEdit->text().isEmpty());
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    // Search for "some"
    QTest::keyClicks(searchTextEdit, "some");
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QTRY_COMPARE(entryView->model()->rowCount(), 3);
    // Search for "someTHING"
    QTest::keyClicks(searchTextEdit, "THING");
    QTRY_COMPARE(entryView->model()->rowCount(), 2);
    // Press Down to focus on the entry view
    QTest::keyClick(searchTextEdit, Qt::Key_Right, Qt::ControlModifier);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QTest::keyClick(searchTextEdit, Qt::Key_Down);
    QTRY_VERIFY(entryView->hasFocus());
    // Restore focus and search text selection
    QTest::keyClick(m_mainWindow, Qt::Key_F, Qt::ControlModifier);
    QTRY_COMPARE(searchTextEdit->selectedText(), QString("someTHING"));
    // Ensure Down focuses on entry view when search text is selected
    QTest::keyClick(searchTextEdit, Qt::Key_Down);
    QTRY_VERIFY(entryView->hasFocus());
    // Refocus back to search edit
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    // Test password copy
    QClipboard *clipboard = QApplication::clipboard();
    QTest::keyClick(searchTextEdit, Qt::Key_C, Qt::ControlModifier);
    QModelIndex searchedItem = entryView->model()->index(0, 1);
    Entry* searchedEntry = entryView->entryFromIndex(searchedItem);
    QTRY_COMPARE(searchedEntry->password(), clipboard->text());

    // Test case sensitive search
    searchWidget->setCaseSensitive(true);
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    searchWidget->setCaseSensitive(false);
    QTRY_COMPARE(entryView->model()->rowCount(), 2);

    // Test group search
    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QModelIndex rootGroupIndex = groupView->model()->index(0, 0);
    clickIndex(groupView->model()->index(0, 0, rootGroupIndex), groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup()->name(), QString("General"));
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // reset
    clickIndex(rootGroupIndex, groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());

    // Try to edit the first entry from the search view
    // Refocus back to search edit
    QTest::mouseClick(searchTextEdit, Qt::LeftButton);
    QTRY_VERIFY(searchTextEdit->hasFocus());
    QVERIFY(m_dbWidget->isInSearchMode());

    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);
    QTest::keyClick(searchTextEdit, Qt::Key_Return);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Perform the edit and save it
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QString origTitle = titleEdit->text();
    QTest::keyClicks(titleEdit, "_edited");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm the edit was made and we are back in search mode
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QCOMPARE(entry->title(), origTitle.append("_edited"));

    // Cancel search, should return to normal view
    QTest::keyClick(m_mainWindow, Qt::Key_Escape);
    QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
}

void TestGui::testDeleteEntry()
{
    // Add canned entries for consistent testing
    testAddEntry();

    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QAction* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");
    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton);
    QVERIFY(entryDeleteWidget->isVisible());
    QVERIFY(entryDeleteWidget->isEnabled());
    QVERIFY(!m_db->metadata()->recycleBin());

    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(2, 0), entryView, Qt::LeftButton, Qt::ControlModifier);
    QCOMPARE(entryView->selectionModel()->selectedRows().size(), 2);

    MessageBox::setNextAnswer(QMessageBox::No);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 1);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
    QModelIndex rootGroupIndex = groupView->model()->index(0, 0);
    clickIndex(groupView->model()->index(groupView->model()->rowCount(rootGroupIndex) - 1, 0, rootGroupIndex),
               groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup()->name(), m_db->metadata()->recycleBin()->name());

    clickIndex(entryView->model()->index(0, 0), entryView, Qt::LeftButton);
    MessageBox::setNextAnswer(QMessageBox::No);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 2);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 2);

    clickIndex(entryView->model()->index(0, 0), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton, Qt::ControlModifier);
    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 0);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 0);

    clickIndex(groupView->model()->index(0, 0),
               groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
}

void TestGui::testCloneEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    QCOMPARE(entryView->model()->rowCount(), 1);

    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entryOrg = entryView->entryFromIndex(item);
    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntryClone");

     CloneDialog* cloneDialog = m_dbWidget->findChild<CloneDialog*>("CloneDialog");
     QDialogButtonBox* cloneButtonBox = cloneDialog->findChild<QDialogButtonBox*>("buttonBox");
     QTest::mouseClick(cloneButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 2);
    Entry* entryClone = entryView->entryFromIndex(entryView->model()->index(1, 1));
    QVERIFY(entryOrg->uuid() != entryClone->uuid());
    QCOMPARE(entryClone->title(), entryOrg->title() + QString(" - Clone"));
}

void TestGui::testEntryPlaceholders()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    QLineEdit* usernameEdit = editEntryWidget->findChild<QLineEdit*>("usernameEdit");
    QTest::keyClicks(usernameEdit, "john");
    QLineEdit* urlEdit = editEntryWidget->findChild<QLineEdit*>("urlEdit");
    QTest::keyClicks(urlEdit, "{TITLE}.{USERNAME}");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 2);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->url(), QString("{TITLE}.{USERNAME}"));

    // Test password copy
    QClipboard *clipboard = QApplication::clipboard();
    m_dbWidget->copyURL();
    QTRY_COMPARE(clipboard->text(), QString("test.john"));
}

void TestGui::testDragAndDropEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
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

    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     groupModel->index(1, 0, rootIndex),
                     -1, true, "Windows", 0);

    // dropping parent on child is supposed to fail
    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     groupModel->index(0, 0, groupModel->index(0, 0, rootIndex)),
                     -1, false, "NewDatabase", 0);

    dragAndDropGroup(groupModel->index(1, 0, rootIndex),
                     rootIndex,
                     0, true, "NewDatabase", 0);

    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     rootIndex,
                     -1, true, "NewDatabase", 4);
}

void TestGui::testSaveAs()
{
    QFileInfo fileInfo(m_dbFilePath);
    QDateTime lastModified = fileInfo.lastModified();

    m_db->metadata()->setName("SaveAs");

    // open temporary file so it creates a filename
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QString tmpFileName = tmpFile.fileName();
    tmpFile.remove();

    fileDialog()->setNextFileName(tmpFileName);

    triggerAction("actionDatabaseSaveAs");

    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("SaveAs"));

    checkDatabase(tmpFileName);

    fileInfo.refresh();
    QCOMPARE(fileInfo.lastModified(), lastModified);
}

void TestGui::testSave()
{
    m_db->metadata()->setName("Save");
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    checkDatabase();
}

void TestGui::testDatabaseSettings()
{
    m_db->metadata()->setName("Save");
    triggerAction("actionChangeDatabaseSettings");
    QWidget* dbSettingsWidget = m_dbWidget->findChild<QWidget*>("databaseSettingsWidget");
    QSpinBox* transformRoundsSpinBox = dbSettingsWidget->findChild<QSpinBox*>("transformRoundsSpinBox");
    transformRoundsSpinBox->setValue(100);
    QTest::keyClick(transformRoundsSpinBox, Qt::Key_Enter);
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));
    QCOMPARE(m_db->transformRounds(), Q_UINT64_C(100));

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    checkDatabase();
}

void TestGui::testKeePass1Import()
{
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb"));
    triggerAction("actionImportKeePass1");

    QWidget* keepass1OpenWidget = m_mainWindow->findChild<QWidget*>("keepass1OpenWidget");
    QLineEdit* editPassword = keepass1OpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "masterpw");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QCOMPARE(m_tabWidget->count(), 2);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("basic [New database]*"));

    // Close the KeePass1 Database
    MessageBox::setNextAnswer(QMessageBox::No);
    triggerAction("actionDatabaseClose");
    Tools::wait(100);

}

void TestGui::testDatabaseLocking()
{
    QString origDbName = m_tabWidget->tabText(0);

    MessageBox::setNextAnswer(QMessageBox::Cancel);
    triggerAction("actionLockDatabases");

    QCOMPARE(m_tabWidget->tabText(0).remove('&'), origDbName + " [locked]");

    QWidget* dbWidget = m_tabWidget->currentDatabaseWidget();
    QWidget* unlockDatabaseWidget = dbWidget->findChild<QWidget*>("unlockDatabaseWidget");
    QWidget* editPassword = unlockDatabaseWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QCOMPARE(m_tabWidget->tabText(0).remove('&'), origDbName);
}

void TestGui::cleanupTestCase()
{
    delete m_mainWindow;
}

void TestGui::checkDatabase(QString dbFileName)
{
    if (dbFileName.isEmpty())
        dbFileName = m_dbFilePath;

    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    QScopedPointer<Database> dbSaved(reader.readDatabase(dbFileName, key));
    QVERIFY(dbSaved);
    QVERIFY(!reader.hasError());
    QCOMPARE(dbSaved->metadata()->name(), m_db->metadata()->name());
}

void TestGui::triggerAction(const QString& name)
{
    QAction* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
}

void TestGui::dragAndDropGroup(const QModelIndex& sourceIndex, const QModelIndex& targetIndex, int row,
                               bool expectedResult, const QString& expectedParentName, int expectedPos)
{
    QVERIFY(sourceIndex.isValid());
    QVERIFY(targetIndex.isValid());

    GroupModel* groupModel = qobject_cast<GroupModel*>(m_dbWidget->findChild<GroupView*>("groupView")->model());

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

void TestGui::clickIndex(const QModelIndex& index, QAbstractItemView* view, Qt::MouseButton button,
                         Qt::KeyboardModifiers stateKey)
{
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}

QTEST_MAIN(TestGui)

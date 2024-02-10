/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#include "../streams.h" // for printable logs on QStrings verification

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "config-keepassx-tests.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"

#include <QApplication>
#include <QComboBox>
#include <QPushButton>
#include <QTest>

FixtureWithDb::FixtureWithDb()
    : FixtureBase()
    , m_tabWidget(m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget"))
{
    // Copy the test database file to a temporary file
    auto origFilePath = QDir(KEEPASSX_TEST_DATA_DIR).absoluteFilePath("NewDatabase.kdbx");
    REQUIRE(m_dbFile.copyFromFile(origFilePath));

    m_dbFilePath = m_dbFile.fileName();
    m_dbFileName = QFileInfo(m_dbFilePath).fileName();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");
    QApplication::processEvents();

    m_dbWidget = m_tabWidget->currentDatabaseWidget();

    unlockDbByPassword("a");
    REQUIRE_FALSE(m_dbWidget->isLocked());
    m_db = m_dbWidget->database();
}

FixtureWithDb::~FixtureWithDb()
{
    // DO NOT save the database
    m_db->markAsClean();
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");
    QApplication::processEvents();
    MessageBox::setNextAnswer(MessageBox::NoButton);

    delete m_dbWidget;
    m_dbFile.remove();
}

void FixtureWithDb::unlockDbByPassword(const QString& password)
{
    auto* databaseOpenWidget = m_dbWidget->findChild<QWidget*>("databaseOpenWidget");
    REQUIRE(databaseOpenWidget);

    auto* pPasswordEdit = databaseOpenWidget->findChild<QLineEdit*>("passwordEdit");
    REQUIRE(pPasswordEdit);
    pPasswordEdit->setFocus();

    QTest::keyClicks(pPasswordEdit, password);
    QTest::keyClick(pPasswordEdit, Qt::Key_Enter);

    QApplication::processEvents();
}

Entry* FixtureWithDb::newEntry(const QString& title, const QString& user, const QString& pwd)
{
    auto* pEntryView = m_dbWidget->findChild<EntryView*>("entryView");

    clickToolbarButton("actionEntryNew");
    REQUIRE(m_dbWidget->currentMode() == DatabaseWidget::Mode::EditMode);

    auto* pEditEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    REQUIRE(pEditEntryWidget);

    auto* pTitleEdit = pEditEntryWidget->findChild<QLineEdit*>("titleEdit");
    REQUIRE(pTitleEdit);
    QTest::keyClicks(pTitleEdit, title);

    auto* pUsernameComboBox = pEditEntryWidget->findChild<QComboBox*>("usernameComboBox");
    REQUIRE(pUsernameComboBox);
    QTest::mouseClick(pUsernameComboBox, Qt::LeftButton);
    QTest::keyClicks(pUsernameComboBox, user);

    auto* pPasswordEdit = pEditEntryWidget->findChild<PasswordWidget*>()->findChild<QLineEdit*>("passwordEdit");
    REQUIRE(pPasswordEdit);
    QTest::keyClicks(pPasswordEdit, pwd);

    auto* pEditEntryWidgetButtonBox = pEditEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    REQUIRE(pEditEntryWidgetButtonBox);
    QTest::mouseClick(pEditEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    auto* pCurrentEntry = pEntryView->currentEntry();
    REQUIRE(pCurrentEntry);
    REQUIRE(pCurrentEntry->password() == pwd);

    return pCurrentEntry;
}

Entry* FixtureWithDb::getCurrentEntry()
{
    auto* pEntryView = m_dbWidget->findChild<EntryView*>("entryView");
    REQUIRE(pEntryView);

    auto* pCurrentEntry = pEntryView->currentEntry();
    REQUIRE(pCurrentEntry);

    return pCurrentEntry;
}

void FixtureWithDb::checkDatabase(const QString& filePath, const QString& expectedDbName)
{
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    auto dbSaved = QSharedPointer<Database>::create();

    REQUIRE(dbSaved->open(filePath, key, nullptr));
    REQUIRE(dbSaved->metadata()->name() == expectedDbName);
}

void FixtureWithDb::checkDatabase(const QString& filePath)
{
    checkDatabase(filePath.isEmpty() ? m_dbFilePath : filePath, m_db->metadata()->name());
}

void FixtureWithDb::saveAndCheckDatabase(int attempts)
{
    REQUIRE(m_db->isModified());
    REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));

    // Attempt to save the database many times to overcome transient file errors.
    int i = 0;
    do {
        triggerAction("actionDatabaseSave");
        if (!m_db->isModified()) {
            checkDatabase();
            return;
        }
        WARN("Failed to save database, trying again...");
        wait(250);
    } while (++i < attempts);

    FAIL("Could not save database.");
}

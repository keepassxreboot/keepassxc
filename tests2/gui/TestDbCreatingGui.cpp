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
#include "../streams.h" // for printable logs on QStrings verification
#include "config-keepassx-tests.h"

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/databasekey/KeyComponentWidget.h"
#include "gui/databasekey/KeyFileEditWidget.h"
#include "gui/databasekey/PasswordEditWidget.h"
#include "gui/wizard/NewDatabaseWizard.h"
#include "keys/FileKey.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTest>

#define TEST_MODAL_NO_WAIT(TEST_CODE)                                                                                  \
    bool dialogFinished = false;                                                                                       \
    QTimer::singleShot(0, [&]() { TEST_CODE dialogFinished = true; })

// The test was migrated "as is" due of custom TEST_MODAL_NO_WAIT solution it does not fit into WHEN/THEN workflow.
SCENARIO_METHOD(FixtureWithDb, "Test New Database creation", "[gui]")
{
    TEST_MODAL_NO_WAIT(
        NewDatabaseWizard * wizard; QTRY_VERIFY(wizard = m_tabWidget->findChild<NewDatabaseWizard*>());

        QTest::keyClicks(wizard->currentPage()->findChild<QLineEdit*>("databaseName"), "Test Name");
        QTest::keyClicks(wizard->currentPage()->findChild<QLineEdit*>("databaseDescription"), "Test Description");
        REQUIRE(wizard->currentId() == 0);

        QTest::keyClick(wizard, Qt::Key_Enter);
        REQUIRE(wizard->currentId() == 1);

        // Check that basic encryption settings are visible
        auto decryptionTimeSlider = wizard->currentPage()->findChild<QSlider*>("decryptionTimeSlider");
        auto algorithmComboBox = wizard->currentPage()->findChild<QComboBox*>("algorithmComboBox");
        REQUIRE(decryptionTimeSlider->isVisible());
        REQUIRE_FALSE(algorithmComboBox->isVisible());

        // Set the encryption settings to the advanced view
        auto encryptionSettings = wizard->currentPage()->findChild<QTabWidget*>("encryptionSettingsTabWidget");
        auto advancedTab = encryptionSettings->findChild<QWidget*>("advancedTab");
        encryptionSettings->setCurrentWidget(advancedTab);
        REQUIRE_FALSE(decryptionTimeSlider->isVisible());
        REQUIRE(algorithmComboBox->isVisible());

        auto rounds = wizard->currentPage()->findChild<QSpinBox*>("transformRoundsSpinBox");
        REQUIRE(rounds->isVisible());
        QTest::mouseClick(rounds, Qt::MouseButton::LeftButton);
        QTest::keyClick(rounds, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(rounds, "2");
        QTest::keyClick(rounds, Qt::Key_Tab);
        QTest::keyClick(rounds, Qt::Key_Tab);

        auto memory = wizard->currentPage()->findChild<QSpinBox*>("memorySpinBox");
        REQUIRE(memory->isVisible());
        QTest::mouseClick(memory, Qt::MouseButton::LeftButton);
        QTest::keyClick(memory, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(memory, "50");
        QTest::keyClick(memory, Qt::Key_Tab);

        auto parallelism = wizard->currentPage()->findChild<QSpinBox*>("parallelismSpinBox");
        REQUIRE(parallelism->isVisible());
        QTest::mouseClick(parallelism, Qt::MouseButton::LeftButton);
        QTest::keyClick(parallelism, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(parallelism, "1");
        QTest::keyClick(parallelism, Qt::Key_Enter);
        wait(250);

        REQUIRE(wizard->currentId() == 2);

        // enter password
        auto* passwordWidget = wizard->currentPage()->findChild<PasswordEditWidget*>();
        REQUIRE(passwordWidget->visiblePage() == KeyFileEditWidget::Page::Edit);
        auto* passwordEdit =
            passwordWidget->findChild<PasswordWidget*>("enterPasswordEdit")->findChild<QLineEdit*>("passwordEdit");
        auto* passwordRepeatEdit =
            passwordWidget->findChild<PasswordWidget*>("repeatPasswordEdit")->findChild<QLineEdit*>("passwordEdit");
        REQUIRE(passwordEdit->isVisible());
        REQUIRE(passwordEdit->hasFocus());
        QTest::keyClicks(passwordEdit, "test");
        QTest::keyClick(passwordEdit, Qt::Key::Key_Tab);
        QTest::keyClicks(passwordRepeatEdit, "test");

        // add key file
        auto* additionalOptionsButton = wizard->currentPage()->findChild<QPushButton*>("additionalKeyOptionsToggle");
        auto* keyFileWidget = wizard->currentPage()->findChild<KeyFileEditWidget*>();
        QVERIFY(additionalOptionsButton->isVisible());
        QTest::mouseClick(additionalOptionsButton, Qt::MouseButton::LeftButton);
        REQUIRE(keyFileWidget->isVisible());
        REQUIRE_FALSE(additionalOptionsButton->isVisible());
        REQUIRE(passwordWidget->visiblePage() == KeyFileEditWidget::Page::Edit);
        QTest::mouseClick(keyFileWidget->findChild<QPushButton*>("addButton"), Qt::MouseButton::LeftButton);
        wait(250);
        auto* fileEdit = keyFileWidget->findChild<QLineEdit*>("keyFileLineEdit");
        REQUIRE(fileEdit->isVisible());
        auto fileName = QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key");
        fileDialog()->setNextFileName(fileName);
        QTest::keyClick(keyFileWidget->findChild<QPushButton*>("addButton"), Qt::Key::Key_Enter);
        REQUIRE(fileEdit->hasFocus());
        auto* browseButton = keyFileWidget->findChild<QPushButton*>("browseKeyFileButton");
        QTest::keyClick(browseButton, Qt::Key::Key_Enter);
        REQUIRE(fileEdit->text() == fileName);

        // save database to temporary file
        TemporaryFile tmpFile;
        REQUIRE(tmpFile.open());
        tmpFile.close();
        fileDialog()->setNextFileName(tmpFile.fileName());

        // click Continue on the warning due to weak password
        MessageBox::setNextAnswer(MessageBox::ContinueWithWeakPass);
        QTest::keyClick(fileEdit, Qt::Key::Key_Enter);

        tmpFile.remove(););
    REQUIRE(m_tabWidget->count() == 1);
    REQUIRE(m_tabWidget->tabText(0) == m_dbFileName);

    triggerAction("actionDatabaseNew");

    REQUIRE(m_tabWidget->count() == 2);
    checkStatusBarText("0 Ent");

    // there is a new empty db
    m_db = m_tabWidget->currentDatabaseWidget()->database();
    REQUIRE(m_db->rootGroup()->children().isEmpty());

    // check meta data
    REQUIRE(m_db->metadata()->name() == QString("Test Name"));
    REQUIRE(m_db->metadata()->description() == QString("Test Description"));

    // check key and encryption
    REQUIRE(m_db->key()->keys().size() == 2);
    REQUIRE(m_db->kdf()->rounds() == 2);
    REQUIRE(m_db->kdf()->uuid() == KeePass2::KDF_ARGON2D);
    REQUIRE(m_db->cipher() == KeePass2::CIPHER_AES256);
    auto compositeKey = QSharedPointer<CompositeKey>::create();
    compositeKey->addKey(QSharedPointer<PasswordKey>::create("test"));
    auto fileKey = QSharedPointer<FileKey>::create();
    fileKey->load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"));
    compositeKey->addKey(fileKey);
    REQUIRE(m_db->key()->rawKey() == compositeKey->rawKey());

    checkStatusBarText("0 Ent");

    // Test the switching to other DB tab
    m_tabWidget->setCurrentIndex(0);
    checkStatusBarText("1 Ent");

    m_tabWidget->setCurrentIndex(1);
    checkStatusBarText("0 Ent");

    // close the new database
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");

    // Wait for dialog to terminate
    REQUIRE(dialogFinished);
}

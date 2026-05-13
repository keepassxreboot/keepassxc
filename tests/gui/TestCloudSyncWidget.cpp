/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "TestCloudSyncWidget.h"

#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

#include "config-keepassx-tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/CategoryListWidget.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"
#include "gui/remote/DatabaseSettingsWidgetCloudSync.h"
#include "gui/remote/RemoteSettings.h"
#include "gui/remote/dropbox/DropboxCloudSyncPage.h"
#include "gui/remote/nextcloud/NextcloudCloudSyncPage.h"
#include "mock/MockDropboxLoginFlow.h"

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    app.applyTheme();
    QTEST_DISABLE_KEYPAD_NAVIGATION
    TestCloudSyncWidget tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

void TestCloudSyncWidget::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createConfigFromFile(TemporaryFile::createTempConfigFile(), {});

    QLocale::setDefault(QLocale::c());
    Application::bootstrap();

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    QVERIFY(m_tabWidget);
    m_mainWindow->show();
    m_mainWindow->resize(1024, 768);
}

void TestCloudSyncWidget::init()
{
    // Reset config and quiet down the first-run-only modals fired from
    // MainWindow's ctor / openDatabase path. Mirrors TestGui::init.
    config()->resetToDefaults();
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    config()->set(Config::UpdateCheckMessageShown, true);
    config()->set(Config::Security_QuickUnlock, false);
    config()->set(Config::UseAtomicSaves, false);
    config()->set(Config::GUI_ShowExpiredEntriesOnDatabaseUnlock, false);
    config()->set(Config::OpenPreviousDatabasesOnStartup, false);

    // Copy the canonical test database (same one TestGui uses) into a
    // temporary file so that each test mutates a fresh on-disk copy. The
    // database has a real CompositeKey, so the database-key page's
    // saveSettings short-circuits at its has-key early-return -- without
    // this we'd land on its raw QMessageBox "no password" modal which is
    // not wired to MessageBox::setNextAnswer and would hang the test.
    auto origFilePath = QDir(KEEPASSX_TEST_DATA_DIR).absoluteFilePath("NewDatabase.kdbx");
    QVERIFY(m_dbFile.copyFromFile(origFilePath));
    m_dbFilePath = m_dbFile.fileName();

    m_mainWindow->activateWindow();
    QApplication::processEvents();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");
    QApplication::processEvents();

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    QVERIFY(m_dbWidget);
    auto* databaseOpenWidget = m_dbWidget->findChild<QWidget*>("databaseOpenWidget");
    QVERIFY(databaseOpenWidget);
    auto* editPassword =
        databaseOpenWidget->findChild<PasswordWidget*>("editPassword")->findChild<QLineEdit*>("passwordEdit");
    QVERIFY(editPassword);
    editPassword->setFocus();
    QTRY_VERIFY(editPassword->hasFocus());
    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QTRY_VERIFY(!m_dbWidget->isLocked());
    m_db = m_dbWidget->database();
    QApplication::processEvents();

    openCloudSyncSettings();
}

// Open the Database Settings dialog through the same action a user would
// trigger from the menu, then navigate to the Cloud Sync category via the
// dialog's CategoryListWidget. Mirrors TestGui (triggerAction +
// setCurrentCategory at TestGui.cpp:199, :1717) rather than the
// programmatic shortcut DatabaseWidget::switchToCloudSyncSettings, so the
// test exercises the actionDatabaseSettings -> tabWidget::showDatabaseSettings
// -> DatabaseWidget::switchToDatabaseSettings wiring end to end.
void TestCloudSyncWidget::openCloudSyncSettings()
{
    triggerAction("actionDatabaseSettings");
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::DatabaseSettingsMode);

    auto* dialog = m_dbWidget->findChild<DatabaseSettingsDialog*>("databaseSettingsDialog");
    QVERIFY(dialog);
    QTRY_VERIFY(dialog->isVisible());

    m_widget = dialog->findChild<DatabaseSettingsWidgetCloudSync*>();
    QVERIFY(m_widget);

    // Navigate to the Cloud Sync category exactly the way TestGui navigates
    // categories (TestGui.cpp:1717). The page index is discovered via
    // EditWidget::pageIndex so the test stays correct when feature flags
    // shift the page ordering (Browser/KeeShare/FdoSecrets between Cloud
    // Sync and Maintenance).
    auto* categoryList = dialog->findChild<CategoryListWidget*>("categoryList");
    QVERIFY(categoryList);
    const int cloudSyncIndex = dialog->pageIndex(m_widget);
    QVERIFY(cloudSyncIndex >= 0);
    categoryList->setCurrentCategory(cloudSyncIndex);

    // CRITICAL: prove the cloud-sync widget is actually rendered on screen,
    // not just constructed under the QStackedWidget. A regression that leaves
    // the page hidden (wrong setCurrentPage index, unparented widget, page
    // ordering shift) would fail here -- without this assertion every other
    // check in the file would still pass against an invisible widget.
    QTRY_VERIFY(m_widget->isVisible());

    auto* buttonBox = dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(m_applyButton);
    QVERIFY(m_applyButton->isVisible());
}

void TestCloudSyncWidget::cleanup()
{
    if (m_tabWidget && m_tabWidget->isVisible() && m_dbWidget) {
        // DO NOT save the database; saveAllSettings can mark it dirty (the
        // General page always re-stamps SettingsChanged), so suppress the
        // "save before close?" prompt by clearing the dirty flag first.
        m_db->markAsClean();
        MessageBox::setNextAnswer(MessageBox::No);
        triggerAction("actionDatabaseClose");
        QApplication::processEvents();
        MessageBox::setNextAnswer(MessageBox::NoButton);
        delete m_dbWidget;
    }
    m_widget = nullptr;
    m_applyButton = nullptr;
    m_db.reset();
}

void TestCloudSyncWidget::cleanupTestCase()
{
    m_dbFile.remove();
}

void TestCloudSyncWidget::triggerAction(const QString& name)
{
    auto* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY2(action, qPrintable(QString("Action doesn't exist: %1").arg(name)));
    QVERIFY2(action->isEnabled(), qPrintable(QString("Action is disabled: %1").arg(name)));
    action->trigger();
    QApplication::processEvents();
}

// Helper: lookup scoped to the dropboxPage subtree, falling back to the widget
// root for parent-level controls (providerComboBox, etc.). The two provider
// pages share several widget object names (remotePathEdit, authorizeButton,
// authStatusLabel, ...) so a top-level findChild would be ambiguous.
template <typename T> static T* findInDropboxPage(QWidget* widget, const char* name)
{
    QWidget* dropboxPage = widget->findChild<QWidget*>(QStringLiteral("dropboxPage"));
    if (dropboxPage) {
        T* hit = dropboxPage->template findChild<T*>(QString::fromLatin1(name));
        if (hit) {
            return hit;
        }
    }
    return widget->findChild<T*>(QString::fromLatin1(name));
}

template <typename T> static T* findInNextcloudPage(QWidget* widget, const char* name)
{
    QWidget* nextcloudPage = widget->findChild<QWidget*>(QStringLiteral("nextcloudPage"));
    if (nextcloudPage) {
        return nextcloudPage->template findChild<T*>(QString::fromLatin1(name));
    }
    return nullptr;
}

// Check that once a provider is set, exploring other providers' forms
// does not erase the current provider.
void TestCloudSyncWidget::CloudSettingNotImpactedWhileExploringOtherProviders()
{
    auto* comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    auto* dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    auto* nextcloudPage = m_widget->findChild<NextcloudCloudSyncPage*>(QStringLiteral("nextcloudPage"));
    auto* appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    auto* dropboxRemotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    auto* authStatusLabel = findInDropboxPage<QLabel>(m_widget, "authStatusLabel");
    auto* serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    auto* nextcloudRemotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(nextcloudPage);
    QVERIFY(appKeyEdit);
    QVERIFY(dropboxRemotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(authStatusLabel);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(nextcloudRemotePathEdit);

    // ---- Step 1: initial state -------------------------------------------
    // Default provider is Dropbox, no fields filled, Apply button grayed.
    QCOMPARE(comboBox->currentIndex(), 0);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    QVERIFY(appKeyEdit->text().isEmpty());
    QVERIFY(dropboxRemotePathEdit->text().isEmpty());
    QCOMPARE(dropboxPage->isModified(), false);
    // CRITICAL: literal Apply button, not a proxy. If the initial empty
    // loadFromConfig accidentally fires modified() (e.g. a future change
    // drops the QSignalBlockers around setText), this assertion catches it.
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 2: fill conf, authorize, apply -----------------------------
    QTest::keyClicks(appKeyEdit, QStringLiteral("test-app-key"));
    // CRITICAL: typing into a field must enable Apply (the dirty signal
    // path: appKeyEdit textChanged -> markModified -> emit modified() ->
    // settingsModified -> setModified(true) -> Apply enabled). QTRY because
    // textChanged may deliver across an event-loop boundary.
    QTRY_VERIFY(m_applyButton->isEnabled());

    QTest::keyClicks(dropboxRemotePathEdit, QStringLiteral("/test/path.kdbx"));
    QTRY_VERIFY(m_applyButton->isEnabled());

    // Inject the login-flow mock BEFORE clicking Authorize so the page's
    // lazy-construct in ensureLoginFlow is a no-op (the test seam wins).
    auto* loginFlow = new MockDropboxLoginFlow();
    loginFlow->setCannedTokens(QStringLiteral("tok-123"), QStringLiteral("rtok-456"), 99999999999LL);
    loginFlow->setNextStartOutcome(MockDropboxLoginFlow::StartOutcome::Completed);
    dropboxPage->setLoginFlowForTest(loginFlow);

    QVERIFY(authorizeButton->isVisible());
    QVERIFY(authorizeButton->isEnabled());
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    QTRY_VERIFY(authStatusLabel->text().contains(QStringLiteral("Authorized")));
    // CRITICAL: authorization completion calls mergeAndPersistTokens which
    // emits modified() again. Apply must stay enabled until the user clicks
    // it -- otherwise the freshly-acquired tokens would be unsaveable.
    QTRY_VERIFY(m_applyButton->isEnabled());

    // "Tu apply" -- actually click the Apply button (do NOT call
    // saveSettings() directly). This exercises the real button-click ->
    // EditWidget::apply() -> DatabaseSettingsDialog::applySettings ->
    // saveAllSettings -> setModified(false) chain.
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);
    // CRITICAL: a successful Apply must re-gray the button. If the handler's
    // setModified(false) is short-circuited (e.g. saveSettings returns false)
    // or a stale modified() fires after, this catches it. Plain QVERIFY
    // (not QTRY) -- saveAllSettings -> setModified(false) is synchronous on
    // the Apply mouseClick, matching TestGui:637.
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 3: switch to Nextcloud -------------------------------------
    comboBox->setCurrentIndex(1);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Nextcloud"));
    // Nextcloud page has never been edited -- both fields empty.
    QVERIFY(serverBaseUrlEdit->text().isEmpty());
    QVERIFY(nextcloudRemotePathEdit->text().isEmpty());
    QCOMPARE(nextcloudPage->isModified(), false);
    // CRITICAL: switching providers in the combobox is NOT a user edit. If
    // onProviderChanged ever emits modified() (or any side effect that
    // bubbles to settingsModified), Apply re-enables here and we trip.
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 4: switch back to Dropbox ----------------------------------
    comboBox->setCurrentIndex(0);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    // The whole point of this test: typed values must survive the round-trip
    // through the Nextcloud page. If a future change wipes m_config or
    // re-loads the page from RemoteSettings on every switch, this fails.
    QCOMPARE(appKeyEdit->text(), QStringLiteral("test-app-key"));
    QCOMPARE(dropboxRemotePathEdit->text(), QStringLiteral("/test/path.kdbx"));
    // CRITICAL: Apply must still be grayed -- coming back to Dropbox is also
    // not a user edit. Same contract as step 3.
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 5: persisted JSON ------------------------------------------
    // Fresh RemoteSettings reads from the database's CustomData -- closes
    // the loop from "I clicked Apply" all the way to bytes on disk.
    RemoteSettings verifySettings(m_db, nullptr);
    QJsonObject config =
        verifySettings.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default"));
    QCOMPARE(config[QStringLiteral("type")].toString(), QStringLiteral("dropbox"));
    QCOMPARE(config[QStringLiteral("name")].toString(), QStringLiteral("dropbox-default"));
    QCOMPARE(config[QStringLiteral("appKey")].toString(), QStringLiteral("test-app-key"));
    QCOMPARE(config[QStringLiteral("remotePath")].toString(), QStringLiteral("/test/path.kdbx"));
    QCOMPARE(config[QStringLiteral("accessToken")].toString(), QStringLiteral("tok-123"));
    QCOMPARE(config[QStringLiteral("refreshToken")].toString(), QStringLiteral("rtok-456"));
    QCOMPARE(verifySettings.activeProvider(), QStringLiteral("dropbox"));
    // No Nextcloud record persisted -- we only ever filled Dropbox.
    QVERIFY(verifySettings.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default"))
                .isEmpty());
}

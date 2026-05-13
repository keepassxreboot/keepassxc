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
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStatusBar>
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
#include "gui/MessageWidget.h"
#include "gui/PasswordWidget.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"
#include "gui/remote/DatabaseSettingsWidgetCloudSync.h"
#include "gui/remote/RemoteSettings.h"
#include "gui/remote/dropbox/DropboxCloudSyncPage.h"
#include "gui/remote/nextcloud/NextcloudCloudSyncPage.h"
#include "mock/MockDropboxLoginFlow.h"
#include "mock/MockDropboxSyncProvider.h"
#include "mock/MockNextcloudLoginFlow.h"
#include "mock/MockNextcloudSyncProvider.h"
#include "remotesync/RemoteSyncProvider.h"

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

    // Install the test factory override BEFORE constructing MainWindow.
    // DatabaseSettingsWidgetCloudSync::registerPage calls
    // RemoteSyncProvider::create("dropbox", ...) / ("nextcloud", ...) once
    // during page construction (the result is stored on
    // DropboxCloudSyncPage::m_dropboxProvider / NextcloudCloudSyncPage::
    // m_nextcloudProvider and never refreshed) -- so installing the override
    // after `new MainWindow()` would leave the pages bound to real providers
    // and make every subsequent Test Connection / Remove / sync click hit
    // live HTTPS.
    //
    // The mocks delegate isAuthorized() to the real base class (gated on a
    // kill-switch static), so the older paste-creds tests
    // (CloudSettingSwitchProviderRemoveOldOne / CloudSettingMenuEntry) see
    // identical "is this config authorized?" semantics to before -- only the
    // network-fronted methods change. Override returns nullptr for unknown
    // types so the default factory dispatch still produces real
    // CommandSyncProvider for the script-sync path.
    RemoteSyncProvider::setFactoryOverrideForTest([](const QString& type, QObject* parent) -> RemoteSyncProvider* {
        if (type == QStringLiteral("dropbox")) {
            return new MockDropboxSyncProvider(parent);
        }
        if (type == QStringLiteral("nextcloud")) {
            return new MockNextcloudSyncProvider(parent);
        }
        return nullptr;
    });

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
    // Clear the factory override before any later test binary in the same
    // process inherits a stale Dropbox->Mock binding. Symmetric with the
    // initTestCase install.
    RemoteSyncProvider::clearFactoryOverrideForTest();
    MockDropboxSyncProvider::setDownloadSourcePath(QString());
    MockDropboxSyncProvider::resetCallCounts();
    MockNextcloudSyncProvider::setDownloadSourcePath(QString());
    MockNextcloudSyncProvider::setIsAuthorizedOverride(true);
    MockNextcloudSyncProvider::resetCallCounts();
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

// Check that switching to a different provider, filling it, and clicking Apply
// removes the previously-configured provider from the database JSON. Encodes
// the single-provider model enforced by
// DatabaseSettingsWidgetCloudSync::saveSettings: when the active page is
// authorized, every other page's config is wiped from RemoteSettings and the
// active page becomes the database's only cloud-sync provider.
void TestCloudSyncWidget::CloudSettingSwitchProviderRemoveOldOne()
{
    auto* comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    auto* dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    auto* nextcloudPage = m_widget->findChild<NextcloudCloudSyncPage*>(QStringLiteral("nextcloudPage"));
    auto* appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    auto* dropboxRemotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    auto* serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    auto* nextcloudRemotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* loginNameEdit = findInNextcloudPage<QLineEdit>(m_widget, "loginNameEdit");
    auto* appPasswordEdit = findInNextcloudPage<QLineEdit>(m_widget, "appPasswordEdit");
    auto* appPasswordGroupBox = findInNextcloudPage<QGroupBox>(m_widget, "appPasswordGroupBox");
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(nextcloudPage);
    QVERIFY(appKeyEdit);
    QVERIFY(dropboxRemotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(nextcloudRemotePathEdit);
    QVERIFY(loginNameEdit);
    QVERIFY(appPasswordEdit);
    QVERIFY(appPasswordGroupBox);

    // ---- Step 1: configure + Apply Dropbox -------------------------------
    QTest::keyClicks(appKeyEdit, QStringLiteral("test-app-key"));
    QTest::keyClicks(dropboxRemotePathEdit, QStringLiteral("/old.kdbx"));

    auto* loginFlow = new MockDropboxLoginFlow();
    loginFlow->setCannedTokens(QStringLiteral("dropbox-tok"), QStringLiteral("dropbox-rtok"), 99999999999LL);
    loginFlow->setNextStartOutcome(MockDropboxLoginFlow::StartOutcome::Completed);
    dropboxPage->setLoginFlowForTest(loginFlow);
    QVERIFY(authorizeButton->isVisible());
    QVERIFY(authorizeButton->isEnabled());
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    QTRY_VERIFY(m_applyButton->isEnabled());

    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);
    QVERIFY(!m_applyButton->isEnabled());

    // Intermediate state: Dropbox persisted, no Nextcloud record yet.
    // Asserting this in-line proves that step 2's wipe assertion later is
    // actually testing "Dropbox WAS there before Apply" -- not just "Dropbox
    // was never there to begin with."
    {
        RemoteSettings rs(m_db, nullptr);
        QJsonObject dropboxConfig =
            rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default"));
        QCOMPARE(dropboxConfig[QStringLiteral("type")].toString(), QStringLiteral("dropbox"));
        QCOMPARE(dropboxConfig[QStringLiteral("appKey")].toString(), QStringLiteral("test-app-key"));
        QCOMPARE(dropboxConfig[QStringLiteral("remotePath")].toString(), QStringLiteral("/old.kdbx"));
        QCOMPARE(dropboxConfig[QStringLiteral("accessToken")].toString(), QStringLiteral("dropbox-tok"));
        QCOMPARE(dropboxConfig[QStringLiteral("refreshToken")].toString(), QStringLiteral("dropbox-rtok"));
        QCOMPARE(rs.activeProvider(), QStringLiteral("dropbox"));
        QVERIFY(rs.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default")).isEmpty());
    }

    // ---- Step 2: switch to Nextcloud and fill it -------------------------
    comboBox->setCurrentIndex(1);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Nextcloud"));
    // Switching providers is not a user edit -- Apply stays grayed.
    QVERIFY(!m_applyButton->isEnabled());

    QTest::keyClicks(serverBaseUrlEdit, QStringLiteral("https://cloud.example.com"));
    // CRITICAL: first user edit after the page-switch must enable Apply.
    QTRY_VERIFY(m_applyButton->isEnabled());
    QTest::keyClicks(nextcloudRemotePathEdit, QStringLiteral("/Passwords/Database.kdbx"));

    // Expand the App Password sub-panel and type the credentials. This is
    // the "paste-without-Authorize" path the page explicitly supports:
    // saveToConfig reads loginNameEdit / appPasswordEdit when non-empty,
    // bypassing onAppPasswordAuthorizeClicked (which would call
    // NextcloudSyncProvider::testConnection -- real network, no mock
    // available on this branch). Filling the line edits matches what a
    // user would type; what we skip is the optional pre-flight test, not
    // the persistence path being verified.
    appPasswordGroupBox->setChecked(true);
    QTest::keyClicks(loginNameEdit, QStringLiteral("alice"));
    QTest::keyClicks(appPasswordEdit, QStringLiteral("app-pw-123"));

    // ---- Step 3: Apply -- the single-provider wipe must fire here --------
    QTRY_VERIFY(m_applyButton->isEnabled());
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 4: final on-disk state -------------------------------------
    // The point of the test. The wipe is gated on
    // probe->isAuthorized(config) being true for the new active page; that
    // is why we fill all 4 Nextcloud-isAuthorized fields above (loginName +
    // appPassword + serverBaseUrl + remotePath).
    RemoteSettings verifySettings(m_db, nullptr);

    // CRITICAL: Dropbox config must be gone. Regressions this catches:
    //   * dropping the if(authorized) wipe loop in saveSettings,
    //   * wiping only m_remoteSettings in-memory but not calling saveSettings,
    //   * setProviderConfig running before removeProviderConfig on the
    //     wrong provider key (would leave both entries on disk).
    QVERIFY(verifySettings.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default")).isEmpty());

    QJsonObject nextcloudConfig =
        verifySettings.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default"));
    QCOMPARE(nextcloudConfig[QStringLiteral("type")].toString(), QStringLiteral("nextcloud"));
    QCOMPARE(nextcloudConfig[QStringLiteral("name")].toString(), QStringLiteral("nextcloud-default"));
    QCOMPARE(nextcloudConfig[QStringLiteral("serverBaseUrl")].toString(), QStringLiteral("https://cloud.example.com"));
    QCOMPARE(nextcloudConfig[QStringLiteral("remotePath")].toString(), QStringLiteral("/Passwords/Database.kdbx"));
    QCOMPARE(nextcloudConfig[QStringLiteral("loginName")].toString(), QStringLiteral("alice"));
    QCOMPARE(nextcloudConfig[QStringLiteral("appPassword")].toString(), QStringLiteral("app-pw-123"));
    QCOMPARE(verifySettings.activeProvider(), QStringLiteral("nextcloud"));

    // ---- Step 5: the displaced Dropbox page's UI was also reset ----------
    // saveSettings calls loadFromConfig({}) on every non-active page when
    // the new active page is authorized. Switching back to Dropbox here
    // must show empty fields, NOT the values typed before the wipe --
    // otherwise the UI would visually contradict the on-disk single-provider
    // state (the comment at DatabaseSettingsWidgetCloudSync.cpp:215-223
    // calls out exactly this case).
    comboBox->setCurrentIndex(0);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    QVERIFY(appKeyEdit->text().isEmpty());
    QVERIFY(dropboxRemotePathEdit->text().isEmpty());
    QCOMPARE(dropboxPage->isModified(), false);
    QVERIFY(!m_applyButton->isEnabled());
}

// Check that the Database > Remote Sync menu's "Trigger <Provider> Sync"
// entry tracks the currently-configured cloud provider:
//   * No provider configured -> no Trigger entry at all.
//   * Nextcloud configured + Apply -> "Trigger Nextcloud Sync" appears,
//     "Trigger Dropbox Sync" must NOT appear.
//   * Switch to Dropbox + Apply -> "Trigger Dropbox Sync" appears,
//     "Trigger Nextcloud Sync" must NOT appear (the previous entry was
//     wiped along with the old provider config under the single-provider
//     model).
//
// The exact string comes from MainWindow.cpp:1290 --
//     tr("Trigger %1 Sync").arg(providerName)
// where providerName is RemoteSyncProvider::displayName() ("Dropbox" or
// "Nextcloud", untranslated brand identifiers).
void TestCloudSyncWidget::CloudSettingMenuEntry()
{
    // The cloud-sync widget's Apply path writes to m_db's CustomData; that
    // fires Database::modified, which DatabaseWidget connects to its own
    // onDatabaseModified slot (DatabaseWidget.cpp:1565), and that slot calls
    // m_remoteSettings->loadSettings() to reread. So the menu reads the
    // post-Apply state on the next updateRemoteSyncMenuEntries call.
    auto* menuRemoteSync = m_mainWindow->findChild<QMenu*>(QStringLiteral("menuRemoteSync"));
    QVERIFY(menuRemoteSync);

    // CRITICAL: this lambda reads the real QMenu the user sees. It does NOT
    // call isCloudSyncAuthorized() or getCloudSyncProviderDisplayName()
    // directly -- if a regression broke the link between those contracts
    // and the visible menu (e.g. updateRemoteSyncMenuEntries stops being
    // wired to aboutToShow, or it stops calling addAction), this test
    // would fail where a contract-level test would still pass. The
    // isVisible() filter matches what a user can actually click -- an
    // action that's in the QMenu's action list but hidden does not appear
    // in the popup.
    auto hasTriggerEntryFor = [menuRemoteSync](const QString& providerDisplayName) {
        const QString expected = QStringLiteral("Trigger %1 Sync").arg(providerDisplayName);
        for (auto* action : menuRemoteSync->actions()) {
            if (action->isVisible() && action->text() == expected) {
                return true;
            }
        }
        return false;
    };

    // ---- Step 1: fresh DB, no cloud sync configured ----------------------
    // Trigger the production rebuild path by actually popping the menu --
    // the QMenu::aboutToShow signal that MainWindow.cpp:175 wires to the
    // (private) updateRemoteSyncMenuEntries slot fires inside popup().
    // Same pattern as TestGui::prepareAndTriggerRemoteSync.
    menuRemoteSync->popup({0, 0});
    QApplication::processEvents();
    menuRemoteSync->close();
    // CRITICAL: neither Trigger entry must be present before any provider
    // is configured. If isCloudSyncAuthorized returns true for an empty
    // RemoteSettings, this fails.
    QVERIFY(!hasTriggerEntryFor(QStringLiteral("Dropbox")));
    QVERIFY(!hasTriggerEntryFor(QStringLiteral("Nextcloud")));

    // ---- Step 2: configure Nextcloud via the cloud-sync widget + Apply --
    auto* comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    auto* serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    auto* nextcloudRemotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* loginNameEdit = findInNextcloudPage<QLineEdit>(m_widget, "loginNameEdit");
    auto* appPasswordEdit = findInNextcloudPage<QLineEdit>(m_widget, "appPasswordEdit");
    auto* appPasswordGroupBox = findInNextcloudPage<QGroupBox>(m_widget, "appPasswordGroupBox");
    QVERIFY(comboBox);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(nextcloudRemotePathEdit);
    QVERIFY(loginNameEdit);
    QVERIFY(appPasswordEdit);
    QVERIFY(appPasswordGroupBox);

    comboBox->setCurrentIndex(1);
    QTest::keyClicks(serverBaseUrlEdit, QStringLiteral("https://cloud.example.com"));
    QTest::keyClicks(nextcloudRemotePathEdit, QStringLiteral("/Passwords/Database.kdbx"));
    appPasswordGroupBox->setChecked(true);
    QTest::keyClicks(loginNameEdit, QStringLiteral("alice"));
    QTest::keyClicks(appPasswordEdit, QStringLiteral("app-pw-123"));
    QTRY_VERIFY(m_applyButton->isEnabled());
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);

    // Database::markAsModified starts a 150ms QTimer (Database.cpp:1074)
    // before emitting modified(); DatabaseWidget::onDatabaseModified, the
    // slot that reloads m_remoteSettings, runs after that signal fires.
    // QTRY_VERIFY polls until the dbWidget actually reports the cloud-sync
    // provider as authorized -- no fixed delay (TestGui pattern: zero
    // qWait/qSleep usages in the entire file).
    QTRY_VERIFY(m_dbWidget->isCloudSyncAuthorized());
    QCOMPARE(m_dbWidget->getCloudSyncProviderDisplayName(), QStringLiteral("Nextcloud"));
    // Pop the menu for real: aboutToShow fires from inside QMenu::popup,
    // which is what MainWindow.cpp:175 wires to updateRemoteSyncMenuEntries.
    menuRemoteSync->popup({0, 0});
    QApplication::processEvents();
    menuRemoteSync->close();
    // CRITICAL: this is the visible menu state on the user's screen after
    // they click Apply. Regressions caught: the Apply path not persisting
    // an "authorized" config, dbWidget->m_remoteSettings not reloading on
    // Database::modified, or updateRemoteSyncMenuEntries not honoring the
    // provider's displayName.
    QVERIFY(hasTriggerEntryFor(QStringLiteral("Nextcloud")));
    QVERIFY(!hasTriggerEntryFor(QStringLiteral("Dropbox")));

    // ---- Step 3: switch to Dropbox + Apply -------------------------------
    comboBox->setCurrentIndex(0);
    auto* dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    auto* appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    auto* dropboxRemotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    QVERIFY(dropboxPage);
    QVERIFY(appKeyEdit);
    QVERIFY(dropboxRemotePathEdit);
    QVERIFY(authorizeButton);

    QTest::keyClicks(appKeyEdit, QStringLiteral("test-app-key"));
    QTest::keyClicks(dropboxRemotePathEdit, QStringLiteral("/test/path.kdbx"));

    auto* loginFlow = new MockDropboxLoginFlow();
    loginFlow->setCannedTokens(QStringLiteral("tok-123"), QStringLiteral("rtok-456"), 99999999999LL);
    loginFlow->setNextStartOutcome(MockDropboxLoginFlow::StartOutcome::Completed);
    dropboxPage->setLoginFlowForTest(loginFlow);
    // Dropbox page was re-shown by the combobox switch above; assert that
    // before clicking into it so a regression that keeps Nextcloud visible
    // (or hides both) fails here rather than on a confusing downstream check.
    QTRY_VERIFY(dropboxPage->isVisible());
    QVERIFY(authorizeButton->isVisible());
    QVERIFY(authorizeButton->isEnabled());
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    QTRY_VERIFY(m_applyButton->isEnabled());
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);

    // Same 150ms-timer dance as step 2 -- poll instead of waiting a fixed
    // duration. The displayName flip from "Nextcloud" to "Dropbox" is the
    // observable that proves dbWidget reloaded its RemoteSettings.
    QTRY_COMPARE(m_dbWidget->getCloudSyncProviderDisplayName(), QStringLiteral("Dropbox"));
    QVERIFY(m_dbWidget->isCloudSyncAuthorized());
    // Pop the menu for real (aboutToShow fires inside popup) -- same as
    // the step 2 invocation.
    menuRemoteSync->popup({0, 0});
    QApplication::processEvents();
    menuRemoteSync->close();
    QVERIFY(hasTriggerEntryFor(QStringLiteral("Dropbox")));
    // CRITICAL: switching providers must replace the menu entry, not stack
    // them. updateRemoteSyncMenuEntries' menuRemoteSync->clear() at the top
    // of the slot is what enforces this; if it gets dropped or guarded out,
    // the "Trigger Nextcloud Sync" entry from step 2 would still be there.
    QVERIFY(!hasTriggerEntryFor(QStringLiteral("Nextcloud")));
}

// Click OK on the database settings dialog (the QDialogButtonBox::Ok button)
// and wait for the dialog to actually close. Going through the real button
// click exercises buttonBox accepted() -> EditWidget::accepted ->
// DatabaseSettingsDialog::save -> saveAllSettings + editFinished(true) ->
// DatabaseWidget::switchToMainView. Calling save() directly would skip the
// button-state-machine half of that contract.
void TestCloudSyncWidget::closeDatabaseSettingsViaOk()
{
    auto* dialog = m_dbWidget->findChild<DatabaseSettingsDialog*>("databaseSettingsDialog");
    QVERIFY(dialog);
    auto* buttonBox = dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox);
    auto* okButton = buttonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    QVERIFY(okButton->isVisible());
    QTest::mouseClick(okButton, Qt::LeftButton);
    QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    // The cached widget pointers belong to the now-hidden dialog page; null
    // them so any stale dereference upstream is a crash, not a silent miss.
    m_widget = nullptr;
    m_applyButton = nullptr;
}

// End-to-end Dropbox happy-path lifecycle: validation banners on empty
// state, Authorize through MockDropboxLoginFlow, Test Connection through
// MockDropboxSyncProvider::download, Apply -> autosave -> sync-on-save
// trigger -> "Remote sync 'Dropbox' completed!" + status bar timestamp,
// close/reopen DB to verify CustomData persistence, Remove + final OK
// to verify the displaced-provider single-provider invariant and that
// the post-Remove save runs WITHOUT a remote sync.
//
// Pre-arrangement:
//   * MockDropboxSyncProvider is installed via initTestCase's factory
//     override (lines ~70-90).
//   * MockDropboxLoginFlow is injected per-Authorize-click in this test
//     (NOT at fixture level -- the existing tests inject their own per
//     test, and a shared fixture instance would leak state across tests).
void TestCloudSyncWidget::CloudSettingAddAndRemoveDropboxFullWorkflow()
{
    // Production default for AutoSaveAfterEveryChange is true (Config.cpp:59);
    // init() overrode it to false to keep the other tests deterministic. This
    // workflow specifically exercises the "Apply -> CustomData modified ->
    // autosave -> databaseSaved -> onDatabaseSavedTriggerSync -> syncWithCloud"
    // chain, so we restore the production default and restore False at the
    // end of the test so cleanup() and any subsequent test see the same
    // fixture defaults the rest of the file relies on.
    config()->set(Config::AutoSaveAfterEveryChange, true);
    auto restoreAutoSave = qScopeGuard([] { config()->set(Config::AutoSaveAfterEveryChange, false); });

    // Reset mock state -- earlier tests in this binary may have already hit
    // the static counters / download source / kill switch.
    MockDropboxSyncProvider::resetCallCounts();
    MockDropboxSyncProvider::setDownloadSourcePath(QString());
    MockDropboxSyncProvider::setNextDownloadFailure(QString());
    MockDropboxSyncProvider::setIsAuthorizedOverride(true);
    // Symmetric restore at end of test so subsequent tests in the same
    // binary inherit defaults, not whatever this test last set.
    auto restoreKillSwitch =
        qScopeGuard([] { MockDropboxSyncProvider::setIsAuthorizedOverride(true); });

    // The mock's download() copies its source to a temp path. We CANNOT use
    // m_dbFilePath as that source: that file is the live database the test
    // is operating on, and on Windows it is held open with a sharing lock
    // for the whole duration the database is unlocked -- QFile::copy of a
    // locked source returns false and the page would show a red error
    // banner ("MockDropboxSyncProvider: failed to copy canned source").
    // The canonical read-only test data file is held by nobody, so
    // QFile::copy of it always succeeds across all 4 platforms. We use it
    // ONLY for Test Connection (which just QFile::remove's the result -- no
    // open, no parse). For SyncEngine-driven syncs we set source="" to take
    // the first-sync branch (SyncEngine.cpp:154), avoiding the merge step
    // entirely.
    const QString canonicalKdbxPath = QDir(KEEPASSX_TEST_DATA_DIR).absoluteFilePath(QStringLiteral("NewDatabase.kdbx"));
    QVERIFY(QFileInfo::exists(canonicalKdbxPath));

    auto* banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    auto* comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    auto* dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    auto* appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    auto* remotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    auto* testConnectionButton = findInDropboxPage<QPushButton>(m_widget, "testConnectionButton");
    auto* removeButton = findInDropboxPage<QPushButton>(m_widget, "removeButton");
    auto* authStatusLabel = findInDropboxPage<QLabel>(m_widget, "authStatusLabel");
    auto* syncOnSaveCheckBox = findInDropboxPage<QCheckBox>(m_widget, "syncOnSaveCheckBox");
    auto* syncOnOpenCheckBox = findInDropboxPage<QCheckBox>(m_widget, "syncOnOpenCheckBox");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(appKeyEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);
    QVERIFY(syncOnSaveCheckBox);
    QVERIFY(syncOnOpenCheckBox);

    auto* statusBarLabel = m_mainWindow->findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    QVERIFY(statusBarLabel);

    // ---- Step 0: cloud-sync settings opened on Dropbox -------------------
    // openCloudSyncSettings() in init() already navigated us here. Confirm
    // the default landing state matches a fresh database with no provider.
    QCOMPARE(comboBox->currentIndex(), 0);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    // CRITICAL: dropboxPage must actually be the visible page in the
    // QStackedWidget, not just constructed under it. A regression that leaves
    // the page hidden (wrong initial setCurrentIndex, unparented widget)
    // would fail here.
    QVERIFY(dropboxPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));

    // ---- Step 1: Authorize with no app key -> warning banner -------------
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    // CRITICAL: the production guard at DropboxCloudSyncPage::onAuthorizeClicked
    // (cpp:228) rejects an empty app key BEFORE constructing the login flow.
    // We assert on the banner the user actually sees, not on m_authState --
    // the banner is the regression surface a user can perceive.
    QTRY_VERIFY(banner->isVisible());
    QCOMPARE(banner->text(), QStringLiteral("App Key is required for authorization."));
    QCOMPARE(banner->messageType(), KMessageWidget::Warning);
    // No login flow should have been constructed at this point.
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));

    // ---- Step 2: Test Connection with no token -> warning banner ---------
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    // CRITICAL: onTestConnectionClicked's pre-flight at cpp:296 rejects an
    // unauthorized config BEFORE calling provider->download(). If a
    // regression dropped that early-return, the mock provider's download
    // counter below would increment.
    QTRY_COMPARE(banner->text(), QStringLiteral("Authorize first before testing the connection."));
    QCOMPARE(banner->messageType(), KMessageWidget::Warning);
    QCOMPARE(MockDropboxSyncProvider::downloadCallCount(), 0);

    // ---- Step 3: syncOnSave / syncOnOpen both default-checked ------------
    // CRITICAL: these are the defaults the Apply step (and the post-reopen
    // sync-on-unlock step) rely on. If a regression flips either default,
    // the autosave-driven sync below would not fire and this test would
    // start failing at the QSignalSpy.wait().
    QVERIFY(syncOnSaveCheckBox->isChecked());
    QVERIFY(syncOnOpenCheckBox->isChecked());

    // ---- Step 4: fill fields + Authorize -> success banner + Authorized -
    QTest::keyClicks(appKeyEdit, QStringLiteral("test-app-key"));
    QTest::keyClicks(remotePathEdit, QStringLiteral("/test/path.kdbx"));

    // Inject the login-flow mock BEFORE clicking Authorize so the page's
    // lazy-construct in ensureLoginFlow is a no-op. setLoginFlowForTest
    // reparents the flow to dropboxPage.
    auto* loginFlow = new MockDropboxLoginFlow();
    loginFlow->setCannedTokens(QStringLiteral("tok-123"), QStringLiteral("rtok-456"), 99999999999LL);
    loginFlow->setNextStartOutcome(MockDropboxLoginFlow::StartOutcome::Completed);
    dropboxPage->setLoginFlowForTest(loginFlow);

    // Configure the mock provider so download() returns a copy of the
    // canonical (never-opened) kdbx for the upcoming Test Connection click.
    // SyncEngine also calls download() during the post-Apply sync; for that
    // flow we want filePath="" (= remote not found) so doSave runs without
    // trying to merge. We toggle that by clearing the source between the
    // two flows.
    MockDropboxSyncProvider::setDownloadSourcePath(canonicalKdbxPath);

    QVERIFY(authorizeButton->isEnabled());
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    // CRITICAL: the visible status label and banner are what the user reads
    // post-Authorize. authStatusLabel is updated by updateAuthStatus(Authorized);
    // banner is fired by onAuthorizationCompleted. Both must match exactly
    // or the user sees an inconsistent state.
    QTRY_COMPARE(authStatusLabel->text(), QStringLiteral("Authorized"));
    QCOMPARE(banner->text(), QStringLiteral("Authorization successful, click Apply to save."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    // Authorize -> mergeAndPersistTokens marks the page modified, which
    // bubbles to Apply via settingsModified -> setModified(true).
    QTRY_VERIFY(m_applyButton->isEnabled());

    // ---- Step 5: Test Connection -> "Remote file found." -----------------
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    // CRITICAL: the green banner is the user-visible proof that download()
    // returned success AND a non-empty filePath. If the page ever stops
    // distinguishing "found" from "will-be-created" (cpp:357-364), this
    // assertion catches it.
    QTRY_COMPARE(banner->text(), QStringLiteral("Connected. Remote file found."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(MockDropboxSyncProvider::downloadCallCount(), 1);

    // ---- Step 6: Apply + OK -> autosave -> sync-on-save -> "completed!" --
    // Switch the mock to first-sync mode for the SyncEngine pipeline that
    // will fire from the autosave's databaseSaved signal. With filePath=""
    // SyncEngine takes the doSave-only branch (cpp:154 in SyncEngine), which
    // matches a "remote file doesn't exist yet" first sync and avoids
    // re-running the merge against m_dbFilePath which the local save just
    // overwrote.
    MockDropboxSyncProvider::setDownloadSourcePath(QString());

    QSignalSpy syncCompletedSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);
    QSignalSpy databaseSavedSpy(m_db.data(), &Database::databaseSaved);
    // Snapshot the mock's call counters BEFORE Apply so the post-Apply
    // assertions are deltas, not absolutes. Test Connection (step 5)
    // already called refreshAuth + download once each -- testing in
    // absolutes would falsely require us to know prior history.
    const int refreshBeforeApply = MockDropboxSyncProvider::refreshAuthCallCount();
    const int downloadBeforeApply = MockDropboxSyncProvider::downloadCallCount();
    const int uploadBeforeApply = MockDropboxSyncProvider::uploadCallCount();

    QVERIFY(m_applyButton->isEnabled());
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);
    // CRITICAL: Apply must re-gray the button synchronously (TestGui:637 pattern).
    QVERIFY(!m_applyButton->isEnabled());

    // Click OK immediately -- if we wait for sync to complete here, the OK
    // click's saveAllSettings (which the General page always re-stamps via
    // setSettingsChanged) would fire a SECOND sync after the first finishes.
    // Keeping the two clicks back-to-back means both markAsModified calls
    // collapse into the same already-running 150ms timer (Database.cpp:1074
    // refuses to restart an active timer), so the autosave + sync fires
    // exactly once. Matches a user who hits Apply then OK in rapid succession.
    closeDatabaseSettingsViaOk();

    // Apply + OK -> saveAllSettings (twice) -> RemoteSettings::saveSettings
    // writes to m_db CustomData -> Database::markAsModified (150ms timer
    // collapsed) -> onDatabaseModified -> autosave (AutoSaveAfterEveryChange
    // is on) -> performSave -> databaseSaved -> onDatabaseSavedTriggerSync
    // -> syncWithCloud -> SyncEngine pipeline -> databaseSyncCompleted.
    // The chain is several event-loop turns long, so we wait on the
    // terminal signal rather than poll an intermediate state.
    QVERIFY(syncCompletedSpy.wait(5000));
    // ENGAGE the kill switch IMMEDIATELY -- no event loop iteration
    // between wait() returning and this line, so the queued
    // onDatabaseSavedTriggerSync (sync N's own save -> next sync trigger,
    // see Database.cpp:316 RandomSlug-on-every-save) is still pending in
    // the queue. By engaging the kill switch before any subsequent
    // QTRY_*/wait spins the loop, the queued slot's
    // isCloudSyncAuthorized() check returns false (mock isAuthorized
    // returns false), so no sync 2 starts. This pins the count assertions
    // below to "exactly one sync" -- the user-observable contract of
    // "Apply triggers a sync."
    MockDropboxSyncProvider::setIsAuthorizedOverride(false);
    // CRITICAL: exactly one sync fired with displayName "Dropbox". One
    // sync, not zero (regression: Apply doesn't trigger sync-on-save) and
    // not two+ (regression: Apply emits cloudSyncTriggered AND triggers
    // the autosave chain, double-firing).
    QCOMPARE(syncCompletedSpy.count(), 1);
    QCOMPARE(syncCompletedSpy.at(0).at(0).toString(), QStringLiteral("Dropbox"));
    // databaseSaved fires twice in this step: once from the autosave that
    // runs after Apply+OK's CustomData mod (the trigger for sync 1), and
    // once from sync 1's own doSave (SyncEngine.cpp:218 m_saveFn ->
    // performSave -> m_db->save -> markAsClean -> emit databaseSaved).
    QCOMPARE(databaseSavedSpy.count(), 2);
    // SyncEngine's pipeline calls each of refreshAuth/download/upload
    // exactly once per sync (SyncEngine.cpp doAuthenticate/doDownload/
    // doUpload). One sync -> one of each.
    QCOMPARE(MockDropboxSyncProvider::refreshAuthCallCount() - refreshBeforeApply, 1);
    QCOMPARE(MockDropboxSyncProvider::downloadCallCount() - downloadBeforeApply, 1);
    QCOMPARE(MockDropboxSyncProvider::uploadCallCount() - uploadBeforeApply, 1);

    // ---- Step 7: post-OK -> banner + status bar on main view -------------
    // CRITICAL: the "Remote sync 'X' completed!" banner is set by
    // DatabaseWidget::showMessage from inside the SyncEngine::syncFinished
    // lambda (DatabaseWidget.cpp:1425) -- the user sees it on the main
    // database view, not inside the now-closed settings dialog. The
    // "databaseWidgetMessageWidget" objectName is the testability hook we
    // added on m_messageWidget; without it, findChild<MessageWidget*> would
    // return some unrelated descendant (Edit pages each carry their own
    // unnamed messageWidget) and pick the wrong one.
    auto* mainMessage = m_dbWidget->findChild<MessageWidget*>(QStringLiteral("databaseWidgetMessageWidget"));
    QVERIFY(mainMessage);
    QTRY_VERIFY(mainMessage->text().contains(QStringLiteral("Remote sync 'Dropbox' completed!")));
    QCOMPARE(mainMessage->messageType(), KMessageWidget::Positive);
    // The status bar caption is set by MainWindow::updateSyncStatusBar
    // (cpp:1695-1707) via the databaseSyncCompleted slot. Format is
    // "<provider>: Synced h:mm AP" (MainWindow.cpp:1697). We assert the
    // brand prefix and the structural "h:mm AM" / "h:mm PM" tail rather
    // than a literal clock time, which would race the wall clock.
    QTRY_VERIFY(statusBarLabel->text().startsWith(QStringLiteral("Dropbox: Synced ")));
    // QRegularExpression (not QRegExp -- the latter is deprecated in Qt 5.15
    // and removed in Qt 6). The anchors ^ / $ make it an exact match.
    QRegularExpression clockRegex(QStringLiteral("^Dropbox: Synced \\d{1,2}:\\d{2} (AM|PM)$"));
    QVERIFY2(clockRegex.match(statusBarLabel->text()).hasMatch(),
             qPrintable(QString("statusBarLabel text doesn't match 'Dropbox: Synced h:mm AM/PM' shape: %1")
                            .arg(statusBarLabel->text())));

    // ---- Step 8: reopen Cloud Sync -> Dropbox + Test Connection green ---
    // Going back through the menu action exercises the same code path as a
    // user clicking Database > Settings -> Cloud Sync, including initialize()
    // re-reading the on-disk RemoteSettings from m_db's CustomData.
    openCloudSyncSettings();
    // Need to re-acquire widget pointers under the now-rebuilt page tree.
    banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    remotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    testConnectionButton = findInDropboxPage<QPushButton>(m_widget, "testConnectionButton");
    removeButton = findInDropboxPage<QPushButton>(m_widget, "removeButton");
    authStatusLabel = findInDropboxPage<QLabel>(m_widget, "authStatusLabel");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(appKeyEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);

    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    QVERIFY(dropboxPage->isVisible());
    // CRITICAL: after a save+reload-from-CustomData round trip, the page must
    // show the persisted state (not an empty form). If loadFromConfig stops
    // populating any of these fields, every downstream test would still pass
    // against an empty UI -- the user-visible state would be wrong though.
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized"));
    QCOMPARE(appKeyEdit->text(), QStringLiteral("test-app-key"));
    QCOMPARE(remotePathEdit->text(), QStringLiteral("/test/path.kdbx"));
    // Switch the mock back to "remote file found" so Test Connection's
    // download returns a non-empty filePath again. Canonical-kdbx, not
    // m_dbFilePath -- m_dbFilePath is held open and unfit as a copy source.
    MockDropboxSyncProvider::setDownloadSourcePath(canonicalKdbxPath);
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    QTRY_COMPARE(banner->text(), QStringLiteral("Connected. Remote file found."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized"));

    // Capture the persisted JSON for the post-reopen equality check below.
    QJsonObject dropboxConfigBeforeClose;
    {
        RemoteSettings rs(m_db, nullptr);
        dropboxConfigBeforeClose =
            rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default"));
        QCOMPARE(dropboxConfigBeforeClose[QStringLiteral("type")].toString(), QStringLiteral("dropbox"));
        QCOMPARE(dropboxConfigBeforeClose[QStringLiteral("appKey")].toString(), QStringLiteral("test-app-key"));
        QCOMPARE(dropboxConfigBeforeClose[QStringLiteral("remotePath")].toString(), QStringLiteral("/test/path.kdbx"));
        QCOMPARE(dropboxConfigBeforeClose[QStringLiteral("accessToken")].toString(), QStringLiteral("tok-123"));
        QCOMPARE(dropboxConfigBeforeClose[QStringLiteral("refreshToken")].toString(), QStringLiteral("rtok-456"));
        QCOMPARE(rs.activeProvider(), QStringLiteral("dropbox"));
    }

    // ---- Step 9: close + reopen db, JSON survives unchanged --------------
    // Close the settings dialog via Cancel so we land back on the main view
    // (closing via OK would re-trigger a save+sync round we've already
    // verified). triggerAction("actionDatabaseClose") then closes the db.
    {
        auto* dialog = m_dbWidget->findChild<DatabaseSettingsDialog*>("databaseSettingsDialog");
        QVERIFY(dialog);
        auto* buttonBox = dialog->findChild<QDialogButtonBox*>();
        QVERIFY(buttonBox);
        auto* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
        QVERIFY(cancelButton);
        QTest::mouseClick(cancelButton, Qt::LeftButton);
        QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
        m_widget = nullptr;
        m_applyButton = nullptr;
    }

    // Reset spy on the OLD m_dbWidget before close, then replace.
    {
        // DO NOT autosave during close -- the cloud-sync state is already
        // persisted, and an extra save would trigger another sync we don't
        // want to wait on.
        //
        // Tradeoff acknowledged: markAsClean here would mask a regression
        // that legitimately left the CustomData write from step 6 unsaved.
        // The downstream step-9 JSON check (constructing a fresh
        // RemoteSettings from the reopened m_db and asserting it matches
        // the pre-close snapshot) catches that case indirectly -- if the
        // step-6 write hadn't been autosaved to disk, the reopened db
        // would lack the provider and the JSON-equality assertion would
        // fail.
        m_db->markAsClean();
        MessageBox::setNextAnswer(MessageBox::No);
        triggerAction("actionDatabaseClose");
        QApplication::processEvents();
        MessageBox::setNextAnswer(MessageBox::NoButton);
        delete m_dbWidget;
        m_db.reset();
    }

    // Flip the mock back to first-sync mode BEFORE the reopen. Once the
    // database is reopened it grabs a Windows sharing lock on m_dbFilePath,
    // and the sync-on-open trigger that fires inside SyncEngine would call
    // download() which would try to copy the still-locked m_dbFilePath if
    // the source were stale from step 8 above -- failing the sync with a
    // red "failed to copy canned source" banner. Empty source means
    // SyncEngine takes the "remote file not found / first sync" branch
    // (SyncEngine.cpp:154) and never touches the filesystem source.
    MockDropboxSyncProvider::setDownloadSourcePath(QString());
    // RE-ARM the kill switch: we want the legitimate sync-on-open to fire
    // when the reopened db unlocks. We'll engage the kill switch again
    // right after we verify the reopen sync completed.
    MockDropboxSyncProvider::setIsAuthorizedOverride(true);

    // Reopen. onDatabaseUnlockedTriggerSync will fire syncOnOpen -- we have
    // to wait for it BEFORE we touch the JSON, otherwise the sync's local
    // save races our assertions. Manual inline reopen (rather than the
    // reopenDatabaseAfterClose helper) so the QSignalSpy lands BEFORE the
    // password Enter -- the mock provider is fast enough that the sync can
    // complete between Enter-keypress and a spy created post-helper, making
    // a wait() call on that spy time out forever.
    {
        m_mainWindow->activateWindow();
        QApplication::processEvents();
        fileDialog()->setNextFileName(m_dbFilePath);
        triggerAction("actionDatabaseOpen");
        QApplication::processEvents();

        m_dbWidget = m_tabWidget->currentDatabaseWidget();
        QVERIFY(m_dbWidget);
        // Spy goes here -- BEFORE the password Enter that triggers
        // databaseUnlocked -> QTimer::singleShot(0, syncWithCloud).
        QSignalSpy reopenSyncSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);

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
        // CRITICAL: poll for the sync-on-unlock to complete. If syncOnOpen
        // is ever flipped off by default, or onDatabaseUnlockedTriggerSync
        // drops its dispatch, this would time out -- pinning the contract
        // that opening an authorized database triggers a sync.
        if (reopenSyncSpy.count() == 0) {
            QVERIFY(reopenSyncSpy.wait(5000));
        }
        // ENGAGE the kill switch IMMEDIATELY (same rationale as after
        // step 6's wait): no event loop iteration between wait() returning
        // and here, so the queued slot from sync-on-open's own doSave is
        // still pending in the queue. Engage now -> sync 2 won't start.
        MockDropboxSyncProvider::setIsAuthorizedOverride(false);
        // CRITICAL: exactly one sync. The regression we're catching is
        // "reopen triggers zero syncs" (broken syncOnOpen wiring) and
        // "reopen triggers more than one" (the chain leaking past the
        // kill switch -> means the kill switch broke).
        QCOMPARE(reopenSyncSpy.count(), 1);
        QCOMPARE(reopenSyncSpy.at(0).at(0).toString(), QStringLiteral("Dropbox"));
    }

    // CRITICAL: the persisted JSON read from the freshly-opened db must
    // match what was on disk before close. A regression that drops fields
    // during save/load round-trip (e.g. tokens not written, refreshToken
    // missing) would fail here.
    {
        RemoteSettings rs(m_db, nullptr);
        QJsonObject after =
            rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default"));
        QCOMPARE(after[QStringLiteral("type")].toString(), dropboxConfigBeforeClose[QStringLiteral("type")].toString());
        QCOMPARE(after[QStringLiteral("appKey")].toString(),
                 dropboxConfigBeforeClose[QStringLiteral("appKey")].toString());
        QCOMPARE(after[QStringLiteral("remotePath")].toString(),
                 dropboxConfigBeforeClose[QStringLiteral("remotePath")].toString());
        QCOMPARE(after[QStringLiteral("accessToken")].toString(),
                 dropboxConfigBeforeClose[QStringLiteral("accessToken")].toString());
        QCOMPARE(after[QStringLiteral("refreshToken")].toString(),
                 dropboxConfigBeforeClose[QStringLiteral("refreshToken")].toString());
        QCOMPARE(rs.activeProvider(), QStringLiteral("dropbox"));
    }

    // ---- Step 10: reopen Cloud Sync after db reopen ----------------------
    openCloudSyncSettings();
    banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    remotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    authorizeButton = findInDropboxPage<QPushButton>(m_widget, "authorizeButton");
    testConnectionButton = findInDropboxPage<QPushButton>(m_widget, "testConnectionButton");
    removeButton = findInDropboxPage<QPushButton>(m_widget, "removeButton");
    authStatusLabel = findInDropboxPage<QLabel>(m_widget, "authStatusLabel");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(appKeyEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);

    // CRITICAL: combobox text + index together, not just one. A regression
    // that desynchronizes provider-stacked-widget index from the combobox
    // selection (initialize() at DatabaseSettingsWidgetCloudSync.cpp:135-144)
    // could leave the combobox showing "Dropbox" while the displayed page is
    // Nextcloud, or vice versa.
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    QCOMPARE(comboBox->currentIndex(), 0);
    QVERIFY(dropboxPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized"));

    // Canonical-kdbx source for Test Connection (m_dbFilePath is locked open).
    MockDropboxSyncProvider::setDownloadSourcePath(canonicalKdbxPath);
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    QTRY_COMPARE(banner->text(), QStringLiteral("Connected. Remote file found."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized"));

    // ---- Step 11: Remove -> banner + UI cleared + JSON gone --------------
    QCOMPARE(MockDropboxSyncProvider::revokeTokenCallCount(), 0);
    QTest::mouseClick(removeButton, Qt::LeftButton);
    // CRITICAL: the green confirmation banner is the post-Remove user-facing
    // proof. The exact text comes from DropboxCloudSyncPage::onRemoveClicked
    // (cpp:431) -- a regression that drops the emit would leave the user
    // wondering whether Remove worked at all.
    QTRY_COMPARE(banner->text(), QStringLiteral("Cloud sync configuration removed."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    // The auth status label must flip back to "Not authorized" -- the
    // reverse of step 4's transition.
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));
    // The UI fields must be empty (cleared under QSignalBlockers in
    // onRemoveClicked cpp:421-425).
    QVERIFY(appKeyEdit->text().isEmpty());
    QVERIFY(remotePathEdit->text().isEmpty());
    // CRITICAL: Apply must be grayed after Remove. m_modified is reset to
    // false in onRemoveClicked (cpp:435), and the dialog's overall modified
    // flag is driven by the page's modified() signal -- if the Remove path
    // accidentally re-emits modified() (e.g. via the cleared() chain), this
    // assertion catches the leak. Apply re-enable here would be especially
    // bad: a subsequent Apply would have nothing to save (saveToConfig
    // returns empty for fresh-no-edit) but would still re-stamp other
    // widgets' state.
    QVERIFY(!m_applyButton->isEnabled());
    // The displaced revokeToken was called once -- the page's onRemoveClicked
    // dispatches revoke through m_dropboxProvider for the still-authorized
    // path (cpp:393-403).
    QCOMPARE(MockDropboxSyncProvider::revokeTokenCallCount(), 1);
    // JSON must be gone from CustomData -- onRemoveClicked persisted the
    // removal via m_remoteSettings->removeProviderConfig + saveSettings.
    {
        RemoteSettings rs(m_db, nullptr);
        QVERIFY(rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default")).isEmpty());
    }

    // ---- Step 12: OK -> save fires, NO remote sync triggered ------------
    // After Remove the persisted CustomData no longer has a Dropbox entry,
    // and the kill switch is engaged so SyncEngine's isAuthorized check
    // returns false. The OK click triggers saveAllSettings -> the General
    // page re-stamps SettingsChanged -> Database modified -> autosave
    // (because AutoSaveAfterEveryChange is on) -> Database::databaseSaved.
    // The queued onDatabaseSavedTriggerSync runs but isCloudSyncAuthorized
    // returns false (kill switch -> mock isAuthorized returns false), so
    // no sync starts.
    QSignalSpy postRemoveSavedSpy(m_db.data(), &Database::databaseSaved);
    QSignalSpy postRemoveSyncSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);
    closeDatabaseSettingsViaOk();
    // CRITICAL: the save itself must run -- this confirms the cleared
    // CustomData entry actually lands on disk, not just in memory. A
    // regression that drops the autosave (e.g. an over-eager
    // m_blockAutoSave) would silently leave the dropbox CustomData entry
    // on disk and the next open would re-resurrect the provider.
    QVERIFY(postRemoveSavedSpy.wait(5000));
    QVERIFY(postRemoveSavedSpy.count() >= 1);
    // CRITICAL: zero syncs after Remove + OK. The kill switch ensures the
    // chain breaker takes effect at the SyncEngine entry point, so this
    // assertion is meaningful: it locks in that OK after Remove must not
    // construct a sync that PROCEEDS past isAuthorized. A regression that
    // makes the queued slot bypass its isCloudSyncAuthorized check, or
    // adds a NEW sync trigger that bypasses isAuthorized, would tick this
    // spy and trip the assertion. Drain pending events for a beat so any
    // queued slot has a chance to run before we check.
    QTest::qWait(200);
    QCOMPARE(postRemoveSyncSpy.count(), 0);
    // CRITICAL: the on-disk JSON has no dropbox entry. A regression that
    // somehow re-adds the provider during OK's saveAllSettings (e.g. the
    // cloud-sync widget's saveSettings calling setProviderConfig on an
    // empty config instead of skipping) would re-resurrect dropbox here.
    {
        RemoteSettings rs(m_db, nullptr);
        QVERIFY(rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default")).isEmpty());
        QVERIFY(rs.activeProvider().isEmpty());
    }

    // ---- Step 13: reopen Cloud Sync -> still empty, JSON still empty -----
    openCloudSyncSettings();
    comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    dropboxPage = m_widget->findChild<DropboxCloudSyncPage*>(QStringLiteral("dropboxPage"));
    appKeyEdit = findInDropboxPage<QLineEdit>(m_widget, "appKeyEdit");
    remotePathEdit = findInDropboxPage<QLineEdit>(m_widget, "remotePathEdit");
    authStatusLabel = findInDropboxPage<QLabel>(m_widget, "authStatusLabel");
    QVERIFY(comboBox);
    QVERIFY(dropboxPage);
    QVERIFY(appKeyEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authStatusLabel);

    // CRITICAL: a regression that re-persists an empty/zombie provider
    // entry on save would show "Dropbox / Not authorized / empty fields"
    // here -- visually the same as the truly-removed state but with a
    // CustomData record still present. The JSON check below is what
    // distinguishes those two outcomes.
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));
    QVERIFY(dropboxPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));
    QVERIFY(appKeyEdit->text().isEmpty());
    QVERIFY(remotePathEdit->text().isEmpty());
    {
        RemoteSettings rs(m_db, nullptr);
        QVERIFY(rs.getProviderConfig(QStringLiteral("dropbox"), QStringLiteral("dropbox-default")).isEmpty());
    }

    // Reset the mock so it doesn't bleed into other tests in this binary.
    MockDropboxSyncProvider::setDownloadSourcePath(QString());
}

// End-to-end Nextcloud happy-path lifecycle, structurally parallel to
// CloudSettingAddAndRemoveDropboxFullWorkflow: validation banners on empty
// state, Authorize through MockNextcloudLoginFlow, Test Connection through
// MockNextcloudSyncProvider::testConnection, Apply -> autosave -> sync-on-save
// trigger -> "Remote sync 'Nextcloud' completed!" + status bar timestamp,
// close/reopen DB to verify CustomData persistence, Remove + final OK to
// verify the post-Remove save runs WITHOUT a remote sync.
//
// One block intentionally fails: after closing and reopening the dialog,
// loadFromConfig auto-checks the appPasswordGroupBox when both loginName and
// appPassword are persisted. The test asserts the box is STILL unchecked at
// that point per the user's intended UX (the box should only auto-open when
// the user explicitly chose the paste-creds path). Locking that contract via a
// failing test now means the eventual fix flips this test green without
// needing to add new assertions.
//
// Pre-arrangement:
//   * MockNextcloudSyncProvider is installed via initTestCase's factory
//     override (lines ~70-95).
//   * MockNextcloudLoginFlow is injected per-Authorize-click via the test
//     seam NextcloudCloudSyncPage::setLoginFlowForTest (mirrors Dropbox).
void TestCloudSyncWidget::CloudSettingAddAndRemoveNextCloudFullWorkflow()
{
    // Same AutoSaveAfterEveryChange dance as the Dropbox workflow: the test
    // exercises the "Apply -> CustomData modified -> autosave -> databaseSaved
    // -> onDatabaseSavedTriggerSync -> syncWithCloud" chain, so we restore
    // the production default for the duration of this test and put it back
    // at end so subsequent tests inherit the fixture's deterministic False.
    config()->set(Config::AutoSaveAfterEveryChange, true);
    auto restoreAutoSave = qScopeGuard([] { config()->set(Config::AutoSaveAfterEveryChange, false); });

    // Reset mock state -- earlier tests in this binary may have already hit
    // the static counters / download source / kill switch.
    MockNextcloudSyncProvider::resetCallCounts();
    MockNextcloudSyncProvider::setDownloadSourcePath(QString());
    MockNextcloudSyncProvider::setNextDownloadFailure(QString());
    MockNextcloudSyncProvider::setIsAuthorizedOverride(true);
    auto restoreKillSwitch =
        qScopeGuard([] { MockNextcloudSyncProvider::setIsAuthorizedOverride(true); });

    // Canonical (read-only, never-opened) kdbx serves as the source for Test
    // Connection clicks that should report "Nextcloud connection successful."
    // (= file found). We cannot use m_dbFilePath as source while the database
    // is unlocked: on Windows it's held with a sharing lock and QFile::copy
    // would fail. For SyncEngine-driven syncs we set source="" so the
    // first-sync branch (SyncEngine.cpp:154) runs and avoids re-merging the
    // canonical kdbx every iteration (same chain-breaker rationale as the
    // Dropbox workflow).
    const QString canonicalKdbxPath = QDir(KEEPASSX_TEST_DATA_DIR).absoluteFilePath(QStringLiteral("NewDatabase.kdbx"));
    QVERIFY(QFileInfo::exists(canonicalKdbxPath));

    auto* banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    auto* comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    auto* nextcloudPage = m_widget->findChild<NextcloudCloudSyncPage*>(QStringLiteral("nextcloudPage"));
    auto* serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    auto* remotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    auto* authorizeButton = findInNextcloudPage<QPushButton>(m_widget, "authorizeButton");
    auto* testConnectionButton = findInNextcloudPage<QPushButton>(m_widget, "testConnectionButton");
    auto* removeButton = findInNextcloudPage<QPushButton>(m_widget, "removeButton");
    auto* authStatusLabel = findInNextcloudPage<QLabel>(m_widget, "authStatusLabel");
    auto* appPasswordGroupBox = findInNextcloudPage<QGroupBox>(m_widget, "appPasswordGroupBox");
    auto* loginNameEdit = findInNextcloudPage<QLineEdit>(m_widget, "loginNameEdit");
    auto* appPasswordEdit = findInNextcloudPage<QLineEdit>(m_widget, "appPasswordEdit");
    auto* syncOnSaveCheckBox = findInNextcloudPage<QCheckBox>(m_widget, "syncOnSaveCheckBox");
    auto* syncOnOpenCheckBox = findInNextcloudPage<QCheckBox>(m_widget, "syncOnOpenCheckBox");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(nextcloudPage);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);
    QVERIFY(appPasswordGroupBox);
    QVERIFY(loginNameEdit);
    QVERIFY(appPasswordEdit);
    QVERIFY(syncOnSaveCheckBox);
    QVERIFY(syncOnOpenCheckBox);

    auto* statusBarLabel = m_mainWindow->findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    QVERIFY(statusBarLabel);

    // ---- Step 0: cloud-sync settings opened on Dropbox (fresh DB) --------
    // openCloudSyncSettings() in init() lands on the Dropbox page since the
    // database has no cloud-sync provider configured yet. Confirm before
    // switching to Nextcloud so a regression that changes the default landing
    // page would fail here, not on a confusing downstream assertion.
    QCOMPARE(comboBox->currentIndex(), 0);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Dropbox"));

    // ---- Step 1: switch combobox to Nextcloud ----------------------------
    comboBox->setCurrentIndex(1);
    QCOMPARE(comboBox->currentText(), QStringLiteral("Nextcloud"));
    // CRITICAL: the providerStackedWidget must follow the combobox -- a
    // desync (one shows Nextcloud, the other still Dropbox) would let every
    // downstream assertion run against the wrong page widgets and silently
    // miss real regressions.
    QTRY_VERIFY(nextcloudPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));
    // Switching providers is not a user edit -- Apply stays grayed.
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 2: Authorize with empty fields -> warning banner -----------
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    // CRITICAL: validateAndCanonicalizeServerUrl's Empty branch is the
    // production guard at NextcloudCloudSyncPage::onAuthorizeClicked. The
    // banner is the user-visible regression surface; assert on it, not on
    // m_authState.
    QTRY_VERIFY(banner->isVisible());
    QCOMPARE(banner->text(), QStringLiteral("Enter the Nextcloud server URL first."));
    QCOMPARE(banner->messageType(), KMessageWidget::Warning);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));

    // ---- Step 3: Test Connection with no auth -> warning banner ----------
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    // CRITICAL: onTestConnectionClicked's pre-flight rejects an unauthorized
    // config BEFORE calling provider->testConnection. If a regression dropped
    // that early-return, the mock provider's testConnection counter below
    // would increment.
    QTRY_COMPARE(banner->text(), QStringLiteral("Authorize Nextcloud first to test the connection."));
    QCOMPARE(banner->messageType(), KMessageWidget::Warning);
    QCOMPARE(MockNextcloudSyncProvider::testConnectionCallCount(), 0);

    // ---- Step 4: syncOnSave / syncOnOpen both default-checked ------------
    // CRITICAL: these are the defaults the Apply step (and the post-reopen
    // sync-on-unlock step) rely on. If a regression flips either default,
    // the autosave-driven sync below would not fire and this test would
    // start failing at the QSignalSpy.wait().
    QVERIFY(syncOnSaveCheckBox->isChecked());
    QVERIFY(syncOnOpenCheckBox->isChecked());

    // ---- Step 5: fill fields + Authorize -> success banner + Authorized --
    QTest::keyClicks(serverBaseUrlEdit, QStringLiteral("https://cloud.example.com"));
    QTRY_VERIFY(m_applyButton->isEnabled());
    QTest::keyClicks(remotePathEdit, QStringLiteral("/Passwords/Database.kdbx"));

    // Inject the login-flow mock BEFORE clicking Authorize so the page's
    // lazy-construct in onAuthorizeClicked is a no-op (setLoginFlowForTest
    // wins). setLoginFlowForTest reparents the flow to nextcloudPage.
    auto* loginFlow = new MockNextcloudLoginFlow();
    loginFlow->setCannedCreds(QStringLiteral("test-login-alice"), QStringLiteral("canned-app-pw-456"));
    loginFlow->setNextStartOutcome(MockNextcloudLoginFlow::StartOutcome::Completed);
    nextcloudPage->setLoginFlowForTest(loginFlow);

    QVERIFY(authorizeButton->isEnabled());
    QTest::mouseClick(authorizeButton, Qt::LeftButton);
    // CRITICAL: visible status + banner are what the user reads post-Authorize.
    // authStatusLabel is rendered by updateAuthStatus(Authorized) as
    // "Authorized as <loginName>"; banner is emitted by onLoginCompleted.
    QTRY_VERIFY(authStatusLabel->text().startsWith(QStringLiteral("Authorized")));
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));
    QCOMPARE(banner->text(), QStringLiteral("Authorization successful, click Apply to save."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    // Authorize -> onLoginCompleted -> emit modified() -> Apply enabled.
    QTRY_VERIFY(m_applyButton->isEnabled());

    // ---- Step 6: Test Connection -> "File not found ..." first-sync ------
    // Source path NOT set on the mock -> testConnection returns success with
    // empty filePath -> page distinguishes that as the file-not-found branch
    // and emits the "will be created on first sync" banner. Snapshot the
    // counter BEFORE the click so the delta-of-1 assertion is robust to
    // earlier tests in the binary having driven the counter.
    const int testConnBeforeFirst = MockNextcloudSyncProvider::testConnectionCallCount();
    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    // CRITICAL: the green banner is the user-visible proof the page took the
    // empty-filePath branch. If the page ever stops distinguishing "found"
    // from "will-be-created", this assertion catches it.
    QTRY_COMPARE(banner->text(),
                 QStringLiteral("Connected. File not found -- it will be created on first sync."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(MockNextcloudSyncProvider::testConnectionCallCount() - testConnBeforeFirst, 1);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));

    // ---- Step 7: appPasswordGroupBox unchecked + fields filled & grayed --
    // CRITICAL: onLoginCompleted persists loginName/appPassword into the line
    // edits but does NOT call appPasswordGroupBox->setChecked(true). User
    // mental model for the Login Flow v2 path: the box stays "closed" -- the
    // user authorized via the browser handshake, not via paste. The fields
    // are populated but visually grayed (QGroupBox checkable + unchecked ->
    // children disabled). A regression that auto-checks the box here would
    // confuse the Login-Flow-v2 user into thinking they took the paste path.
    QVERIFY(!appPasswordGroupBox->isChecked());
    QCOMPARE(loginNameEdit->text(), QStringLiteral("test-login-alice"));
    QCOMPARE(appPasswordEdit->text(), QStringLiteral("canned-app-pw-456"));
    // QGroupBox in checkable+unchecked mode disables its children via
    // _q_setChildrenEnabled -- isEnabled() returns false. This is the
    // user-visible "grayed" property.
    QVERIFY(!loginNameEdit->isEnabled());
    QVERIFY(!appPasswordEdit->isEnabled());

    // ---- Step 8: Apply -> autosave -> sync-on-save -> "completed!" -------
    // Source stays empty so SyncEngine takes the first-sync branch
    // (SyncEngine.cpp:154) and skips merge against m_dbFilePath -- same
    // chain-breaker rationale as the Dropbox workflow.
    MockNextcloudSyncProvider::setDownloadSourcePath(QString());

    QSignalSpy syncCompletedSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);
    QSignalSpy databaseSavedSpy(m_db.data(), &Database::databaseSaved);
    const int refreshBeforeApply = MockNextcloudSyncProvider::refreshAuthCallCount();
    const int downloadBeforeApply = MockNextcloudSyncProvider::downloadCallCount();
    const int uploadBeforeApply = MockNextcloudSyncProvider::uploadCallCount();

    QVERIFY(m_applyButton->isEnabled());
    QVERIFY(m_applyButton->isVisible());
    QTest::mouseClick(m_applyButton, Qt::LeftButton);
    // CRITICAL: Apply re-grays the button synchronously (TestGui:637 pattern).
    QVERIFY(!m_applyButton->isEnabled());

    // ---- Step 9: state assertions BETWEEN Apply and OK -------------------
    // CRITICAL: Apply does not call loadFromConfig and must NOT touch the
    // groupBox check state. The user's contract is "after Apply, the UI looks
    // exactly like it did before Apply, just with the Apply button disabled."
    // A regression that re-runs loadFromConfig in saveSettings (or one that
    // calls setChecked(true) on the groupBox after a successful save) would
    // flip these assertions before we ever close the dialog.
    QVERIFY(!appPasswordGroupBox->isChecked());
    QCOMPARE(loginNameEdit->text(), QStringLiteral("test-login-alice"));
    QCOMPARE(appPasswordEdit->text(), QStringLiteral("canned-app-pw-456"));
    QVERIFY(!loginNameEdit->isEnabled());
    QVERIFY(!appPasswordEdit->isEnabled());

    // Click OK back-to-back with Apply -- same 150ms-timer-collapse rationale
    // as the Dropbox workflow: both markAsModified calls fold into one already-
    // running timer, autosave + sync fire exactly once.
    closeDatabaseSettingsViaOk();

    QVERIFY(syncCompletedSpy.wait(5000));
    // Engage the kill switch IMMEDIATELY (no event-loop spin in between) so
    // any queued onDatabaseSavedTriggerSync from sync 1's own doSave sees
    // isAuthorized()=false and does not start sync 2. Pins the count
    // assertions below to "exactly one sync."
    MockNextcloudSyncProvider::setIsAuthorizedOverride(false);
    // CRITICAL: exactly one sync fired with displayName "Nextcloud". Catches
    // both "Apply doesn't trigger sync-on-save" (count 0) and "Apply emits
    // cloudSyncTriggered AND triggers the autosave chain, double-firing"
    // (count >= 2).
    QCOMPARE(syncCompletedSpy.count(), 1);
    QCOMPARE(syncCompletedSpy.at(0).at(0).toString(), QStringLiteral("Nextcloud"));
    // databaseSaved fires twice in this step (same as Dropbox workflow):
    // autosave-after-Apply, then sync 1's own doSave.
    QCOMPARE(databaseSavedSpy.count(), 2);
    QCOMPARE(MockNextcloudSyncProvider::refreshAuthCallCount() - refreshBeforeApply, 1);
    QCOMPARE(MockNextcloudSyncProvider::downloadCallCount() - downloadBeforeApply, 1);
    QCOMPARE(MockNextcloudSyncProvider::uploadCallCount() - uploadBeforeApply, 1);

    // ---- Step 10: post-OK -> banner + status bar on main view ------------
    // CRITICAL: the "Remote sync 'X' completed!" banner is set by
    // DatabaseWidget::showMessage from inside SyncEngine::syncFinished's
    // lambda -- the user sees it on the main database view, not in the now-
    // closed settings dialog. The "databaseWidgetMessageWidget" objectName
    // disambiguates from the various unnamed Edit-page messageWidgets.
    auto* mainMessage = m_dbWidget->findChild<MessageWidget*>(QStringLiteral("databaseWidgetMessageWidget"));
    QVERIFY(mainMessage);
    QTRY_VERIFY(mainMessage->text().contains(QStringLiteral("Remote sync 'Nextcloud' completed!")));
    QCOMPARE(mainMessage->messageType(), KMessageWidget::Positive);
    // Status-bar caption: MainWindow::updateSyncStatusBar formats
    // "<provider>: Synced h:mm AP". Assert brand prefix + structural shape
    // rather than a literal clock time -- the latter would race the wall clock.
    QTRY_VERIFY(statusBarLabel->text().startsWith(QStringLiteral("Nextcloud: Synced ")));
    QRegularExpression clockRegex(QStringLiteral("^Nextcloud: Synced \\d{1,2}:\\d{2} (AM|PM)$"));
    QVERIFY2(clockRegex.match(statusBarLabel->text()).hasMatch(),
             qPrintable(QString("statusBarLabel text doesn't match 'Nextcloud: Synced h:mm AM/PM' shape: %1")
                            .arg(statusBarLabel->text())));

    // ---- Step 11: reopen Cloud Sync -> Nextcloud + Test Connection green -
    // Re-arm the source so the next Test Connection click reports "file found"
    // (= "Nextcloud connection successful.") instead of the first-sync banner.
    MockNextcloudSyncProvider::setDownloadSourcePath(canonicalKdbxPath);

    openCloudSyncSettings();
    banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    nextcloudPage = m_widget->findChild<NextcloudCloudSyncPage*>(QStringLiteral("nextcloudPage"));
    serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    remotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    authorizeButton = findInNextcloudPage<QPushButton>(m_widget, "authorizeButton");
    testConnectionButton = findInNextcloudPage<QPushButton>(m_widget, "testConnectionButton");
    removeButton = findInNextcloudPage<QPushButton>(m_widget, "removeButton");
    authStatusLabel = findInNextcloudPage<QLabel>(m_widget, "authStatusLabel");
    appPasswordGroupBox = findInNextcloudPage<QGroupBox>(m_widget, "appPasswordGroupBox");
    loginNameEdit = findInNextcloudPage<QLineEdit>(m_widget, "loginNameEdit");
    appPasswordEdit = findInNextcloudPage<QLineEdit>(m_widget, "appPasswordEdit");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(nextcloudPage);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);
    QVERIFY(appPasswordGroupBox);
    QVERIFY(loginNameEdit);
    QVERIFY(appPasswordEdit);

    // CRITICAL: combobox text + index together. initialize()'s
    // active-provider lookup must land on Nextcloud after the previous
    // session persisted nextcloud-default; a desync would either show Dropbox
    // (Apply didn't persist activeProvider) or show Nextcloud combobox while
    // the stacked widget still points at Dropbox.
    QCOMPARE(comboBox->currentText(), QStringLiteral("Nextcloud"));
    QCOMPARE(comboBox->currentIndex(), 1);
    QVERIFY(nextcloudPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));

    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    QTRY_COMPARE(banner->text(), QStringLiteral("Nextcloud connection successful."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));

    // ---- Step 12: EXPECTED FAILURE -- groupBox auto-checked on reopen ----
    // The user-intended contract is "the groupBox reflects whether the user
    // explicitly chose the paste path -- not whether creds happen to be
    // persisted." NextcloudCloudSyncPage::loadFromConfig currently violates
    // this by auto-checking the box whenever both loginName and appPassword
    // are present (see NextcloudCloudSyncPage.cpp around line 156).
    //
    // Locking the intended behavior with these asserts now ensures the
    // forthcoming fix flips this test green WITHOUT needing to add new
    // assertions -- and a regression that re-introduces the auto-check after
    // the fix would fail the same lines. The test is expected to FAIL here
    // until the page-side fix lands; everything below in the same function
    // will be unreached until then.
    QVERIFY(!appPasswordGroupBox->isChecked());
    QVERIFY(!loginNameEdit->isEnabled());
    QVERIFY(!appPasswordEdit->isEnabled());

    // ---- Step 13: persisted JSON contains the Nextcloud config -----------
    QJsonObject nextcloudConfigBeforeClose;
    {
        RemoteSettings rs(m_db, nullptr);
        nextcloudConfigBeforeClose =
            rs.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default"));
        QCOMPARE(nextcloudConfigBeforeClose[QStringLiteral("type")].toString(), QStringLiteral("nextcloud"));
        QCOMPARE(nextcloudConfigBeforeClose[QStringLiteral("loginName")].toString(),
                 QStringLiteral("test-login-alice"));
        QCOMPARE(nextcloudConfigBeforeClose[QStringLiteral("appPassword")].toString(),
                 QStringLiteral("canned-app-pw-456"));
        QCOMPARE(nextcloudConfigBeforeClose[QStringLiteral("serverBaseUrl")].toString(),
                 QStringLiteral("https://cloud.example.com"));
        QCOMPARE(nextcloudConfigBeforeClose[QStringLiteral("remotePath")].toString(),
                 QStringLiteral("/Passwords/Database.kdbx"));
        QCOMPARE(rs.activeProvider(), QStringLiteral("nextcloud"));
    }

    // ---- Step 14: close + reopen db, JSON survives unchanged -------------
    // Close the settings dialog via Cancel (no extra save + sync round) then
    // close the database. Same dance as the Dropbox workflow.
    {
        auto* dialog = m_dbWidget->findChild<DatabaseSettingsDialog*>("databaseSettingsDialog");
        QVERIFY(dialog);
        auto* buttonBox = dialog->findChild<QDialogButtonBox*>();
        QVERIFY(buttonBox);
        auto* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
        QVERIFY(cancelButton);
        QTest::mouseClick(cancelButton, Qt::LeftButton);
        QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
        m_widget = nullptr;
        m_applyButton = nullptr;
    }

    {
        // DO NOT autosave during close -- step 8's CustomData write already
        // persisted, an extra save would trigger another sync we don't want
        // to wait on. Same tradeoff acknowledgement as the Dropbox workflow:
        // markAsClean here masks a "step-8 write wasn't autosaved" regression,
        // but the downstream JSON-equality assertion below catches that case
        // indirectly.
        m_db->markAsClean();
        MessageBox::setNextAnswer(MessageBox::No);
        triggerAction("actionDatabaseClose");
        QApplication::processEvents();
        MessageBox::setNextAnswer(MessageBox::NoButton);
        delete m_dbWidget;
        m_db.reset();
    }

    // Flip the mock back to first-sync mode BEFORE reopen -- the post-unlock
    // sync-on-open path will call download(), and we want it to take
    // SyncEngine's "remote not found" branch (no fs source touch).
    MockNextcloudSyncProvider::setDownloadSourcePath(QString());
    // Re-arm the kill switch: the legitimate sync-on-open should fire when
    // the reopened db unlocks. We engage the kill switch again right after.
    MockNextcloudSyncProvider::setIsAuthorizedOverride(true);

    {
        m_mainWindow->activateWindow();
        QApplication::processEvents();
        fileDialog()->setNextFileName(m_dbFilePath);
        triggerAction("actionDatabaseOpen");
        QApplication::processEvents();

        m_dbWidget = m_tabWidget->currentDatabaseWidget();
        QVERIFY(m_dbWidget);
        // Spy MUST be created BEFORE the password Enter -- the mock-provider
        // sync is fast enough that databaseSyncCompleted can fire between
        // Enter-keypress and a spy created later, making a wait() on the
        // late-bound spy hang forever.
        QSignalSpy reopenSyncSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);

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
        // CRITICAL: poll for sync-on-unlock. If syncOnOpen flips off by
        // default or onDatabaseUnlockedTriggerSync drops its dispatch, this
        // times out -- pinning the contract that opening an authorized
        // database triggers a sync.
        if (reopenSyncSpy.count() == 0) {
            QVERIFY(reopenSyncSpy.wait(5000));
        }
        MockNextcloudSyncProvider::setIsAuthorizedOverride(false);
        QCOMPARE(reopenSyncSpy.count(), 1);
        QCOMPARE(reopenSyncSpy.at(0).at(0).toString(), QStringLiteral("Nextcloud"));
    }

    // CRITICAL: persisted JSON read from the freshly-opened db must match
    // what was on disk before close. A save/load round-trip that drops fields
    // (e.g. appPassword not written, serverBaseUrl missing) would fail here.
    {
        RemoteSettings rs(m_db, nullptr);
        QJsonObject after =
            rs.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default"));
        QCOMPARE(after[QStringLiteral("type")].toString(),
                 nextcloudConfigBeforeClose[QStringLiteral("type")].toString());
        QCOMPARE(after[QStringLiteral("loginName")].toString(),
                 nextcloudConfigBeforeClose[QStringLiteral("loginName")].toString());
        QCOMPARE(after[QStringLiteral("appPassword")].toString(),
                 nextcloudConfigBeforeClose[QStringLiteral("appPassword")].toString());
        QCOMPARE(after[QStringLiteral("serverBaseUrl")].toString(),
                 nextcloudConfigBeforeClose[QStringLiteral("serverBaseUrl")].toString());
        QCOMPARE(after[QStringLiteral("remotePath")].toString(),
                 nextcloudConfigBeforeClose[QStringLiteral("remotePath")].toString());
        QCOMPARE(rs.activeProvider(), QStringLiteral("nextcloud"));
    }

    // ---- Step 15: reopen Cloud Sync after db reopen ----------------------
    MockNextcloudSyncProvider::setDownloadSourcePath(canonicalKdbxPath);
    openCloudSyncSettings();
    banner = m_widget->findChild<MessageWidget*>(QStringLiteral("messageWidget"));
    comboBox = m_widget->findChild<QComboBox*>(QStringLiteral("providerComboBox"));
    nextcloudPage = m_widget->findChild<NextcloudCloudSyncPage*>(QStringLiteral("nextcloudPage"));
    serverBaseUrlEdit = findInNextcloudPage<QLineEdit>(m_widget, "serverBaseUrlEdit");
    remotePathEdit = findInNextcloudPage<QLineEdit>(m_widget, "remotePathEdit");
    authorizeButton = findInNextcloudPage<QPushButton>(m_widget, "authorizeButton");
    testConnectionButton = findInNextcloudPage<QPushButton>(m_widget, "testConnectionButton");
    removeButton = findInNextcloudPage<QPushButton>(m_widget, "removeButton");
    authStatusLabel = findInNextcloudPage<QLabel>(m_widget, "authStatusLabel");
    appPasswordGroupBox = findInNextcloudPage<QGroupBox>(m_widget, "appPasswordGroupBox");
    loginNameEdit = findInNextcloudPage<QLineEdit>(m_widget, "loginNameEdit");
    appPasswordEdit = findInNextcloudPage<QLineEdit>(m_widget, "appPasswordEdit");
    QVERIFY(banner);
    QVERIFY(comboBox);
    QVERIFY(nextcloudPage);
    QVERIFY(serverBaseUrlEdit);
    QVERIFY(remotePathEdit);
    QVERIFY(authorizeButton);
    QVERIFY(testConnectionButton);
    QVERIFY(removeButton);
    QVERIFY(authStatusLabel);
    QVERIFY(appPasswordGroupBox);
    QVERIFY(loginNameEdit);
    QVERIFY(appPasswordEdit);

    QCOMPARE(comboBox->currentText(), QStringLiteral("Nextcloud"));
    QCOMPARE(comboBox->currentIndex(), 1);
    QVERIFY(nextcloudPage->isVisible());
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));

    QTest::mouseClick(testConnectionButton, Qt::LeftButton);
    QTRY_COMPARE(banner->text(), QStringLiteral("Nextcloud connection successful."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Authorized as test-login-alice"));

    // ---- Step 16: Remove -> banner + UI cleared + JSON gone --------------
    QTest::mouseClick(removeButton, Qt::LeftButton);
    // CRITICAL: the post-Remove banner is the user-facing proof. Exact text
    // comes from NextcloudCloudSyncPage::onRemoveClicked -- a regression that
    // drops the emit (or shortens the two-sentence form to one) would leave
    // the user wondering whether Remove worked at all, and whether they need
    // to do anything server-side.
    QTRY_COMPARE(banner->text(),
                 QStringLiteral("Nextcloud configuration removed. "
                                "To revoke the app-password server-side, visit your Nextcloud Security page."));
    QCOMPARE(banner->messageType(), KMessageWidget::Positive);
    // The auth status label must flip back to "Not authorized" -- the reverse
    // of step 5's transition.
    QCOMPARE(authStatusLabel->text(), QStringLiteral("Not authorized"));
    // Fields cleared under QSignalBlockers in onRemoveClicked; the placeholder
    // text re-surfaces in the now-empty line edits. Assert on placeholderText,
    // not text(), because text() is empty after the clear.
    QVERIFY(serverBaseUrlEdit->text().isEmpty());
    QVERIFY(remotePathEdit->text().isEmpty());
    QVERIFY(loginNameEdit->text().isEmpty());
    QVERIFY(appPasswordEdit->text().isEmpty());
    QCOMPARE(loginNameEdit->placeholderText(), QStringLiteral("alice"));
    QCOMPARE(appPasswordEdit->placeholderText(), QStringLiteral("xxxx-xxxx-xxxx-xxxx"));
    // "Use App Password Instead" unchecked after Remove (onRemoveClicked
    // calls setChecked(false) under a QSignalBlocker), children grayed.
    QVERIFY(!appPasswordGroupBox->isChecked());
    QVERIFY(!loginNameEdit->isEnabled());
    QVERIFY(!appPasswordEdit->isEnabled());
    // CRITICAL: Apply must be grayed after Remove. m_modified is reset to
    // false in onRemoveClicked. Apply re-enable here would be especially bad:
    // a subsequent Apply would have nothing to save (saveToConfig returns
    // empty for fresh-no-edit) but would still re-stamp other widgets' state.
    QVERIFY(!m_applyButton->isEnabled());
    // JSON must be gone from CustomData -- onRemoveClicked persisted the
    // removal via m_remoteSettings->removeProviderConfig + saveSettings.
    {
        RemoteSettings rs(m_db, nullptr);
        QVERIFY(rs.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default")).isEmpty());
    }

    // ---- Step 17: OK -> save fires, NO remote sync triggered -------------
    // After Remove, persisted CustomData no longer has a Nextcloud entry; the
    // kill switch is engaged so SyncEngine's isAuthorized check returns false.
    // OK triggers saveAllSettings -> General page re-stamps SettingsChanged
    // -> Database modified -> autosave -> databaseSaved. The queued
    // onDatabaseSavedTriggerSync runs but isCloudSyncAuthorized returns false,
    // so no sync starts.
    QSignalSpy postRemoveSavedSpy(m_db.data(), &Database::databaseSaved);
    QSignalSpy postRemoveSyncSpy(m_dbWidget.data(), &DatabaseWidget::databaseSyncCompleted);
    closeDatabaseSettingsViaOk();
    // CRITICAL: the save itself must run -- confirms the cleared CustomData
    // entry actually lands on disk, not just in memory. A regression that
    // drops the autosave (e.g. an over-eager m_blockAutoSave) would silently
    // leave the nextcloud CustomData entry on disk and the next open would
    // re-resurrect the provider.
    QVERIFY(postRemoveSavedSpy.wait(5000));
    QVERIFY(postRemoveSavedSpy.count() >= 1);
    // CRITICAL: zero syncs after Remove + OK. Drain pending events so any
    // queued slot has a chance to run before we check.
    QTest::qWait(200);
    QCOMPARE(postRemoveSyncSpy.count(), 0);
    // CRITICAL: on-disk JSON has no nextcloud entry. A regression that re-
    // adds the provider during OK's saveAllSettings (e.g. saveToConfig
    // returning a non-empty config for an empty form) would re-resurrect it.
    {
        RemoteSettings rs(m_db, nullptr);
        QVERIFY(rs.getProviderConfig(QStringLiteral("nextcloud"), QStringLiteral("nextcloud-default")).isEmpty());
        QVERIFY(rs.activeProvider().isEmpty());
    }

    // Step-17's JSON-empty + activeProvider-empty checks already prove the
    // Remove + OK round-trip wiped CustomData. We deliberately do NOT reopen
    // the settings dialog to assert "which provider page does it default
    // to?" -- the dialog persists its combobox selection across reopens, and
    // pinning either "stays on Nextcloud" or "resets to Dropbox" would lock
    // in a UX detail the product doesn't currently care to specify.

    // Reset the mock so it doesn't bleed into other tests in this binary.
    MockNextcloudSyncProvider::setDownloadSourcePath(QString());
}

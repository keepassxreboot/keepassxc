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
#include <QGroupBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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

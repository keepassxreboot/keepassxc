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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QCloseEvent>
#include <QShortcut>
#include <QTimer>

#if defined(Q_OS_LINUX) && ! defined(QT_NO_DBUS)
#include <QList>
#include <QtDBus/QtDBus>
#endif

#include "config-keepassx.h"

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/FilePath.h"
#include "core/InactivityTimer.h"
#include "core/Metadata.h"
#include "format/KeePass2Writer.h"
#include "gui/AboutDialog.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseRepairWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/SearchWidget.h"

#ifdef WITH_XC_HTTP
#include "http/Service.h"
#include "http/HttpSettings.h"
#include "http/OptionDialog.h"
#endif

#include "gui/SettingsWidget.h"
#include "gui/PasswordGeneratorWidget.h"

#ifdef WITH_XC_HTTP
class HttpPlugin: public ISettingsPage
{
public:
    HttpPlugin(DatabaseTabWidget * tabWidget)
    {
        m_service = new Service(tabWidget);
    }

    ~HttpPlugin() = default;

    QString name() override
    {
        return QObject::tr("Browser Integration");
    }

    QIcon icon() override
    {
        return FilePath::instance()->icon("apps", "internet-web-browser");
    }

    QWidget * createWidget() override
    {
        OptionDialog * dlg = new OptionDialog();
        QObject::connect(dlg, SIGNAL(removeSharedEncryptionKeys()), m_service, SLOT(removeSharedEncryptionKeys()));
        QObject::connect(dlg, SIGNAL(removeStoredPermissions()), m_service, SLOT(removeStoredPermissions()));
        return dlg;
    }

    void loadSettings(QWidget * widget) override
    {
        qobject_cast<OptionDialog*>(widget)->loadSettings();
    }

    void saveSettings(QWidget * widget) override
    {
        qobject_cast<OptionDialog*>(widget)->saveSettings();
        if (HttpSettings::isEnabled())
            m_service->start();
        else
            m_service->stop();
    }
private:
    Service *m_service;
};
#endif

const QString MainWindow::BaseWindowTitle = "KeePassXC";

MainWindow::MainWindow()
    : m_ui(new Ui::MainWindow())
    , m_trayIcon(nullptr)
    , m_appExitCalled(false)
    , m_appExiting(false)
{
    m_ui->setupUi(this);

    // Setup the search widget in the toolbar
    SearchWidget *search = new SearchWidget();
    search->connectSignals(m_actionMultiplexer);
    m_searchWidgetAction = m_ui->toolBar->addWidget(search);
    m_searchWidgetAction->setEnabled(false);

    m_countDefaultAttributes = m_ui->menuEntryCopyAttribute->actions().size();

    restoreGeometry(config()->get("GUI/MainWindowGeometry").toByteArray());
    #ifdef WITH_XC_HTTP
    m_ui->settingsWidget->addSettingsPage(new HttpPlugin(m_ui->tabWidget));
    #endif

    setWindowIcon(filePath()->applicationIcon());
    m_ui->globalMessageWidget->setHidden(true);
    QAction* toggleViewAction = m_ui->toolBar->toggleViewAction();
    toggleViewAction->setText(tr("Show toolbar"));
    m_ui->menuView->addAction(toggleViewAction);
    bool showToolbar = config()->get("ShowToolbar").toBool();
    m_ui->toolBar->setVisible(showToolbar);
    connect(m_ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(saveToolbarState(bool)));

    m_clearHistoryAction = new QAction(tr("Clear history"), m_ui->menuFile);
    m_lastDatabasesActions = new QActionGroup(m_ui->menuRecentDatabases);
    connect(m_clearHistoryAction, SIGNAL(triggered()), this, SLOT(clearLastDatabases()));
    connect(m_lastDatabasesActions, SIGNAL(triggered(QAction*)), this, SLOT(openRecentDatabase(QAction*)));
    connect(m_ui->menuRecentDatabases, SIGNAL(aboutToShow()), this, SLOT(updateLastDatabasesMenu()));

    m_copyAdditionalAttributeActions = new QActionGroup(m_ui->menuEntryCopyAttribute);
    m_actionMultiplexer.connect(m_copyAdditionalAttributeActions, SIGNAL(triggered(QAction*)),
                                SLOT(copyAttribute(QAction*)));
    connect(m_ui->menuEntryCopyAttribute, SIGNAL(aboutToShow()),
            this, SLOT(updateCopyAttributesMenu()));

    Qt::Key globalAutoTypeKey = static_cast<Qt::Key>(config()->get("GlobalAutoTypeKey").toInt());
    Qt::KeyboardModifiers globalAutoTypeModifiers = static_cast<Qt::KeyboardModifiers>(
                config()->get("GlobalAutoTypeModifiers").toInt());
    if (globalAutoTypeKey > 0 && globalAutoTypeModifiers > 0) {
        autoType()->registerGlobalShortcut(globalAutoTypeKey, globalAutoTypeModifiers);
    }

    m_ui->actionEntryAutoType->setVisible(autoType()->isAvailable());

    m_inactivityTimer = new InactivityTimer(this);
    connect(m_inactivityTimer, SIGNAL(inactivityDetected()),
            this, SLOT(lockDatabasesAfterInactivity()));
    applySettingsChanges();

    m_ui->actionDatabaseNew->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N);
    setShortcut(m_ui->actionDatabaseOpen, QKeySequence::Open, Qt::CTRL + Qt::Key_O);
    setShortcut(m_ui->actionDatabaseSave, QKeySequence::Save, Qt::CTRL + Qt::Key_S);
    setShortcut(m_ui->actionDatabaseSaveAs, QKeySequence::SaveAs);
    setShortcut(m_ui->actionDatabaseClose, QKeySequence::Close, Qt::CTRL + Qt::Key_W);
    m_ui->actionLockDatabases->setShortcut(Qt::CTRL + Qt::Key_L);
    setShortcut(m_ui->actionQuit, QKeySequence::Quit, Qt::CTRL + Qt::Key_Q);
    setShortcut(m_ui->actionEntryNew, QKeySequence::New, Qt::CTRL + Qt::Key_N);
    m_ui->actionEntryEdit->setShortcut(Qt::CTRL + Qt::Key_E);
    m_ui->actionEntryDelete->setShortcut(Qt::CTRL + Qt::Key_D);
    m_ui->actionEntryClone->setShortcut(Qt::CTRL + Qt::Key_K);
    m_ui->actionEntryTotp->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
    m_ui->actionEntryCopyTotp->setShortcut(Qt::CTRL + Qt::Key_T);
    m_ui->actionEntryCopyUsername->setShortcut(Qt::CTRL + Qt::Key_B);
    m_ui->actionEntryCopyPassword->setShortcut(Qt::CTRL + Qt::Key_C);
    setShortcut(m_ui->actionEntryAutoType, QKeySequence::Paste, Qt::CTRL + Qt::Key_V);
    m_ui->actionEntryOpenUrl->setShortcut(Qt::CTRL + Qt::Key_U);
    m_ui->actionEntryCopyURL->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_U);

    new QShortcut(Qt::CTRL + Qt::Key_M, this, SLOT(showMinimized()));

    m_ui->actionDatabaseNew->setIcon(filePath()->icon("actions", "document-new"));
    m_ui->actionDatabaseOpen->setIcon(filePath()->icon("actions", "document-open"));
    m_ui->actionDatabaseSave->setIcon(filePath()->icon("actions", "document-save"));
    m_ui->actionDatabaseSaveAs->setIcon(filePath()->icon("actions", "document-save-as"));
    m_ui->actionDatabaseClose->setIcon(filePath()->icon("actions", "document-close"));
    m_ui->actionChangeDatabaseSettings->setIcon(filePath()->icon("actions", "document-edit"));
    m_ui->actionChangeMasterKey->setIcon(filePath()->icon("actions", "database-change-key", false));
    m_ui->actionLockDatabases->setIcon(filePath()->icon("actions", "document-encrypt", false));
    m_ui->actionQuit->setIcon(filePath()->icon("actions", "application-exit"));
    m_ui->actionQuit->setMenuRole(QAction::QuitRole);

    m_ui->actionEntryNew->setIcon(filePath()->icon("actions", "entry-new", false));
    m_ui->actionEntryClone->setIcon(filePath()->icon("actions", "entry-clone", false));
    m_ui->actionEntryEdit->setIcon(filePath()->icon("actions", "entry-edit", false));
    m_ui->actionEntryDelete->setIcon(filePath()->icon("actions", "entry-delete", false));
    m_ui->actionEntryAutoType->setIcon(filePath()->icon("actions", "auto-type", false));
    m_ui->actionEntryCopyUsername->setIcon(filePath()->icon("actions", "username-copy", false));
    m_ui->actionEntryCopyPassword->setIcon(filePath()->icon("actions", "password-copy", false));

    m_ui->actionGroupNew->setIcon(filePath()->icon("actions", "group-new", false));
    m_ui->actionGroupEdit->setIcon(filePath()->icon("actions", "group-edit", false));
    m_ui->actionGroupDelete->setIcon(filePath()->icon("actions", "group-delete", false));
    m_ui->actionGroupEmptyRecycleBin->setIcon(filePath()->icon("actions", "group-empty-trash", false));

    m_ui->actionSettings->setIcon(filePath()->icon("actions", "configure"));
    m_ui->actionSettings->setMenuRole(QAction::PreferencesRole);
    m_ui->actionPasswordGenerator->setIcon(filePath()->icon("actions", "password-generator", false));

    m_ui->actionAbout->setIcon(filePath()->icon("actions", "help-about"));
    m_ui->actionAbout->setMenuRole(QAction::AboutRole);

    m_actionMultiplexer.connect(SIGNAL(currentModeChanged(DatabaseWidget::Mode)),
                                this, SLOT(setMenuActionState(DatabaseWidget::Mode)));
    m_actionMultiplexer.connect(SIGNAL(groupChanged()),
                                this, SLOT(setMenuActionState()));
    m_actionMultiplexer.connect(SIGNAL(entrySelectionChanged()),
                                this, SLOT(setMenuActionState()));
    m_actionMultiplexer.connect(SIGNAL(groupContextMenuRequested(QPoint)),
                                this, SLOT(showGroupContextMenu(QPoint)));
    m_actionMultiplexer.connect(SIGNAL(entryContextMenuRequested(QPoint)),
                                this, SLOT(showEntryContextMenu(QPoint)));

    // Notify search when the active database changes or gets locked
    connect(m_ui->tabWidget, SIGNAL(activateDatabaseChanged(DatabaseWidget*)),
            search, SLOT(databaseChanged(DatabaseWidget*)));
    connect(m_ui->tabWidget, SIGNAL(databaseLocked(DatabaseWidget*)),
            search, SLOT(databaseChanged()));

    connect(m_ui->tabWidget, SIGNAL(tabNameChanged()),
            SLOT(updateWindowTitle()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(updateWindowTitle()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(databaseTabChanged(int)));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(setMenuActionState()));
    connect(m_ui->tabWidget, SIGNAL(databaseLocked(DatabaseWidget*)),
            SLOT(databaseStatusChanged(DatabaseWidget*)));
    connect(m_ui->tabWidget, SIGNAL(databaseUnlocked(DatabaseWidget*)),
            SLOT(databaseStatusChanged(DatabaseWidget*)));
    connect(m_ui->stackedWidget, SIGNAL(currentChanged(int)), SLOT(setMenuActionState()));
    connect(m_ui->stackedWidget, SIGNAL(currentChanged(int)), SLOT(updateWindowTitle()));
    connect(m_ui->settingsWidget, SIGNAL(accepted()), SLOT(applySettingsChanges()));
    connect(m_ui->settingsWidget, SIGNAL(apply()), SLOT(applySettingsChanges()));
    connect(m_ui->settingsWidget, SIGNAL(accepted()), SLOT(switchToDatabases()));
    connect(m_ui->settingsWidget, SIGNAL(rejected()), SLOT(switchToDatabases()));

    connect(m_ui->actionDatabaseNew, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(newDatabase()));
    connect(m_ui->actionDatabaseOpen, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(openDatabase()));
    connect(m_ui->actionDatabaseSave, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(saveDatabase()));
    connect(m_ui->actionDatabaseSaveAs, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(saveDatabaseAs()));
    connect(m_ui->actionDatabaseClose, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(closeDatabase()));
    connect(m_ui->actionDatabaseMerge, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(mergeDatabase()));
    connect(m_ui->actionChangeMasterKey, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(changeMasterKey()));
    connect(m_ui->actionChangeDatabaseSettings, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(changeDatabaseSettings()));
    connect(m_ui->actionImportCsv, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(importCsv()));
    connect(m_ui->actionImportKeePass1, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(importKeePass1Database()));
    connect(m_ui->actionRepairDatabase, SIGNAL(triggered()), this,
            SLOT(repairDatabase()));
    connect(m_ui->actionExportCsv, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(exportToCsv()));
    connect(m_ui->actionLockDatabases, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(lockDatabases()));
    connect(m_ui->actionQuit, SIGNAL(triggered()), SLOT(appExit()));

    m_actionMultiplexer.connect(m_ui->actionEntryNew, SIGNAL(triggered()),
            SLOT(createEntry()));
    m_actionMultiplexer.connect(m_ui->actionEntryClone, SIGNAL(triggered()),
            SLOT(cloneEntry()));
    m_actionMultiplexer.connect(m_ui->actionEntryEdit, SIGNAL(triggered()),
            SLOT(switchToEntryEdit()));
    m_actionMultiplexer.connect(m_ui->actionEntryDelete, SIGNAL(triggered()),
            SLOT(deleteEntries()));

    m_actionMultiplexer.connect(m_ui->actionEntryTotp, SIGNAL(triggered()),
            SLOT(showTotp()));
    m_actionMultiplexer.connect(m_ui->actionEntrySetupTotp, SIGNAL(triggered()),
            SLOT(setupTotp()));

    m_actionMultiplexer.connect(m_ui->actionEntryCopyTotp, SIGNAL(triggered()),
            SLOT(copyTotp()));
    m_actionMultiplexer.connect(m_ui->actionEntryCopyTitle, SIGNAL(triggered()),
            SLOT(copyTitle()));
    m_actionMultiplexer.connect(m_ui->actionEntryCopyUsername, SIGNAL(triggered()),
            SLOT(copyUsername()));
    m_actionMultiplexer.connect(m_ui->actionEntryCopyPassword, SIGNAL(triggered()),
            SLOT(copyPassword()));
    m_actionMultiplexer.connect(m_ui->actionEntryCopyURL, SIGNAL(triggered()),
            SLOT(copyURL()));
    m_actionMultiplexer.connect(m_ui->actionEntryCopyNotes, SIGNAL(triggered()),
            SLOT(copyNotes()));
    m_actionMultiplexer.connect(m_ui->actionEntryAutoType, SIGNAL(triggered()),
            SLOT(performAutoType()));
    m_actionMultiplexer.connect(m_ui->actionEntryOpenUrl, SIGNAL(triggered()),
            SLOT(openUrl()));

    m_actionMultiplexer.connect(m_ui->actionGroupNew, SIGNAL(triggered()),
            SLOT(createGroup()));
    m_actionMultiplexer.connect(m_ui->actionGroupEdit, SIGNAL(triggered()),
            SLOT(switchToGroupEdit()));
    m_actionMultiplexer.connect(m_ui->actionGroupDelete, SIGNAL(triggered()),
            SLOT(deleteGroup()));
    m_actionMultiplexer.connect(m_ui->actionGroupEmptyRecycleBin, SIGNAL(triggered()),
            SLOT(emptyRecycleBin()));

    connect(m_ui->actionSettings, SIGNAL(triggered()), SLOT(switchToSettings()));
    connect(m_ui->actionPasswordGenerator, SIGNAL(toggled(bool)), SLOT(switchToPasswordGen(bool)));
    connect(m_ui->passwordGeneratorWidget, SIGNAL(dialogTerminated()), SLOT(closePasswordGen()));

    connect(m_ui->welcomeWidget, SIGNAL(newDatabase()), SLOT(switchToNewDatabase()));
    connect(m_ui->welcomeWidget, SIGNAL(openDatabase()), SLOT(switchToOpenDatabase()));
    connect(m_ui->welcomeWidget, SIGNAL(openDatabaseFile(QString)), SLOT(switchToDatabaseFile(QString)));
    connect(m_ui->welcomeWidget, SIGNAL(importKeePass1Database()), SLOT(switchToKeePass1Database()));
    connect(m_ui->welcomeWidget, SIGNAL(importCsv()), SLOT(switchToImportCsv()));

    connect(m_ui->actionAbout, SIGNAL(triggered()), SLOT(showAboutDialog()));

#ifdef Q_OS_MAC
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    connect(m_ui->tabWidget, SIGNAL(messageGlobal(QString,MessageWidget::MessageType)), this, SLOT(displayGlobalMessage(QString, MessageWidget::MessageType)));
    connect(m_ui->tabWidget, SIGNAL(messageDismissGlobal()), this, SLOT(hideGlobalMessage()));
    connect(m_ui->tabWidget, SIGNAL(messageTab(QString,MessageWidget::MessageType)), this, SLOT(displayTabMessage(QString, MessageWidget::MessageType)));
    connect(m_ui->tabWidget, SIGNAL(messageDismissTab()), this, SLOT(hideTabMessage()));

    m_screenLockListener = new ScreenLockListener(this);
    connect(m_screenLockListener, SIGNAL(screenLocked()), SLOT(handleScreenLock()));

    updateTrayIcon();

    if (config()->hasAccessError()) {
        m_ui->globalMessageWidget->showMessage(
            tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
    }

}

MainWindow::~MainWindow()
{
}

void MainWindow::appExit()
{
    m_appExitCalled = true;
    close();
}

void MainWindow::updateLastDatabasesMenu()
{
    m_ui->menuRecentDatabases->clear();

    const QStringList lastDatabases = config()->get("LastDatabases", QVariant()).toStringList();
    for (const QString& database : lastDatabases) {
        QAction* action = m_ui->menuRecentDatabases->addAction(database);
        action->setData(database);
        m_lastDatabasesActions->addAction(action);
    }
    m_ui->menuRecentDatabases->addSeparator();
    m_ui->menuRecentDatabases->addAction(m_clearHistoryAction);
}

void MainWindow::updateCopyAttributesMenu()
{
    DatabaseWidget* dbWidget = m_ui->tabWidget->currentDatabaseWidget();
    if (!dbWidget) {
        return;
    }

    if (dbWidget->numberOfSelectedEntries() != 1) {
        return;
    }

    QList<QAction*> actions = m_ui->menuEntryCopyAttribute->actions();
    for (int i = m_countDefaultAttributes; i < actions.size(); i++) {
        delete actions[i];
    }

    const QStringList customEntryAttributes = dbWidget->customEntryAttributes();
    for (const QString& key : customEntryAttributes) {
        QAction* action = m_ui->menuEntryCopyAttribute->addAction(key);
        m_copyAdditionalAttributeActions->addAction(action);
    }
}

void MainWindow::openRecentDatabase(QAction* action)
{
    openDatabase(action->data().toString());
}

void MainWindow::clearLastDatabases()
{
    config()->set("LastDatabases", QVariant());
    bool inWelcomeWidget = (m_ui->stackedWidget->currentIndex() == 2);

    if (inWelcomeWidget) {
        m_ui->welcomeWidget->refreshLastDatabases();
    }
}

void MainWindow::openDatabase(const QString& fileName, const QString& pw, const QString& keyFile)
{
    m_ui->tabWidget->openDatabase(fileName, pw, keyFile);
}

void MainWindow::setMenuActionState(DatabaseWidget::Mode mode)
{
    int currentIndex = m_ui->stackedWidget->currentIndex();
    bool inDatabaseTabWidget = (currentIndex == DatabaseTabScreen);
    bool inWelcomeWidget = (currentIndex == WelcomeScreen);

    if (inDatabaseTabWidget && m_ui->tabWidget->currentIndex() != -1) {
        DatabaseWidget* dbWidget = m_ui->tabWidget->currentDatabaseWidget();
        Q_ASSERT(dbWidget);

        if (mode == DatabaseWidget::None) {
            mode = dbWidget->currentMode();
        }

        switch (mode) {
        case DatabaseWidget::ViewMode: {
            bool inSearch = dbWidget->isInSearchMode();
            bool singleEntrySelected = dbWidget->numberOfSelectedEntries() == 1;
            bool entriesSelected = dbWidget->numberOfSelectedEntries() > 0;
            bool groupSelected = dbWidget->isGroupSelected();
            bool recycleBinSelected = dbWidget->isRecycleBinSelected();

            m_ui->actionEntryNew->setEnabled(!inSearch);
            m_ui->actionEntryClone->setEnabled(singleEntrySelected);
            m_ui->actionEntryEdit->setEnabled(singleEntrySelected);
            m_ui->actionEntryDelete->setEnabled(entriesSelected);
            m_ui->actionEntryCopyTitle->setEnabled(singleEntrySelected && dbWidget->currentEntryHasTitle());
            m_ui->actionEntryCopyUsername->setEnabled(singleEntrySelected && dbWidget->currentEntryHasUsername());
            m_ui->actionEntryCopyPassword->setEnabled(singleEntrySelected && dbWidget->currentEntryHasPassword());
            m_ui->actionEntryCopyURL->setEnabled(singleEntrySelected && dbWidget->currentEntryHasUrl());
            m_ui->actionEntryCopyNotes->setEnabled(singleEntrySelected && dbWidget->currentEntryHasNotes());
            m_ui->menuEntryCopyAttribute->setEnabled(singleEntrySelected);
            m_ui->menuEntryTotp->setEnabled(true);
            m_ui->actionEntryAutoType->setEnabled(singleEntrySelected);
            m_ui->actionEntryOpenUrl->setEnabled(singleEntrySelected && dbWidget->currentEntryHasUrl());
            m_ui->actionEntryTotp->setEnabled(singleEntrySelected && dbWidget->currentEntryHasTotp());
            m_ui->actionEntryCopyTotp->setEnabled(singleEntrySelected && dbWidget->currentEntryHasTotp());
            m_ui->actionEntrySetupTotp->setEnabled(singleEntrySelected);
            m_ui->actionGroupNew->setEnabled(groupSelected);
            m_ui->actionGroupEdit->setEnabled(groupSelected);
            m_ui->actionGroupDelete->setEnabled(groupSelected && dbWidget->canDeleteCurrentGroup());
            m_ui->actionGroupEmptyRecycleBin->setVisible(recycleBinSelected);
            m_ui->actionGroupEmptyRecycleBin->setEnabled(recycleBinSelected);
            m_ui->actionChangeMasterKey->setEnabled(true);
            m_ui->actionChangeDatabaseSettings->setEnabled(true);
            m_ui->actionDatabaseSave->setEnabled(true);
            m_ui->actionDatabaseSaveAs->setEnabled(true);
            m_ui->actionExportCsv->setEnabled(true);
            m_ui->actionDatabaseMerge->setEnabled(m_ui->tabWidget->currentIndex() != -1);

            m_searchWidgetAction->setEnabled(true);
            break;
        }
        case DatabaseWidget::EditMode:
        case DatabaseWidget::ImportMode:
        case DatabaseWidget::LockedMode: {
            const QList<QAction*> entryActions = m_ui->menuEntries->actions();
            for (QAction* action : entryActions) {
                action->setEnabled(false);
            }

            const QList<QAction*> groupActions = m_ui->menuGroups->actions();
            for (QAction* action : groupActions) {
                action->setEnabled(false);
            }
            m_ui->actionEntryCopyTitle->setEnabled(false);
            m_ui->actionEntryCopyUsername->setEnabled(false);
            m_ui->actionEntryCopyPassword->setEnabled(false);
            m_ui->actionEntryCopyURL->setEnabled(false);
            m_ui->actionEntryCopyNotes->setEnabled(false);
            m_ui->menuEntryCopyAttribute->setEnabled(false);
            m_ui->menuEntryTotp->setEnabled(false);

            m_ui->actionChangeMasterKey->setEnabled(false);
            m_ui->actionChangeDatabaseSettings->setEnabled(false);
            m_ui->actionDatabaseSave->setEnabled(false);
            m_ui->actionDatabaseSaveAs->setEnabled(false);
            m_ui->actionExportCsv->setEnabled(false);
            m_ui->actionDatabaseMerge->setEnabled(false);

            m_searchWidgetAction->setEnabled(false);
            break;
        }
        default:
            Q_ASSERT(false);
        }
        m_ui->actionDatabaseClose->setEnabled(true);
    }
    else {
        const QList<QAction*> entryActions = m_ui->menuEntries->actions();
        for (QAction* action : entryActions) {
            action->setEnabled(false);
        }

        const QList<QAction*> groupActions = m_ui->menuGroups->actions();
        for (QAction* action : groupActions) {
            action->setEnabled(false);
        }
        m_ui->actionEntryCopyTitle->setEnabled(false);
        m_ui->actionEntryCopyUsername->setEnabled(false);
        m_ui->actionEntryCopyPassword->setEnabled(false);
        m_ui->actionEntryCopyURL->setEnabled(false);
        m_ui->actionEntryCopyNotes->setEnabled(false);
        m_ui->menuEntryCopyAttribute->setEnabled(false);
        m_ui->menuEntryTotp->setEnabled(false);

        m_ui->actionChangeMasterKey->setEnabled(false);
        m_ui->actionChangeDatabaseSettings->setEnabled(false);
        m_ui->actionDatabaseSave->setEnabled(false);
        m_ui->actionDatabaseSaveAs->setEnabled(false);
        m_ui->actionDatabaseClose->setEnabled(false);
        m_ui->actionExportCsv->setEnabled(false);
        m_ui->actionDatabaseMerge->setEnabled(false);

        m_searchWidgetAction->setEnabled(false);
    }

    bool inDatabaseTabWidgetOrWelcomeWidget = inDatabaseTabWidget || inWelcomeWidget;
    m_ui->actionDatabaseNew->setEnabled(inDatabaseTabWidgetOrWelcomeWidget);
    m_ui->actionDatabaseOpen->setEnabled(inDatabaseTabWidgetOrWelcomeWidget);
    m_ui->menuRecentDatabases->setEnabled(inDatabaseTabWidgetOrWelcomeWidget);
    m_ui->menuImport->setEnabled(inDatabaseTabWidgetOrWelcomeWidget);
    m_ui->actionDatabaseMerge->setEnabled(inDatabaseTabWidget);
    m_ui->actionRepairDatabase->setEnabled(inDatabaseTabWidgetOrWelcomeWidget);

    m_ui->actionLockDatabases->setEnabled(m_ui->tabWidget->hasLockableDatabases());

    if ((currentIndex == PasswordGeneratorScreen) != m_ui->actionPasswordGenerator->isChecked()) {
        bool blocked = m_ui->actionPasswordGenerator->blockSignals(true);
        m_ui->actionPasswordGenerator->toggle();
        m_ui->actionPasswordGenerator->blockSignals(blocked);
    }
}

void MainWindow::updateWindowTitle()
{
    QString customWindowTitlePart;
    int stackedWidgetIndex = m_ui->stackedWidget->currentIndex();
    int tabWidgetIndex = m_ui->tabWidget->currentIndex();
    bool isModified = m_ui->tabWidget->isModified(tabWidgetIndex);

    if (stackedWidgetIndex == DatabaseTabScreen && tabWidgetIndex != -1) {
        customWindowTitlePart = m_ui->tabWidget->tabText(tabWidgetIndex);
        if (isModified) {
            // remove asterisk '*' from title
            customWindowTitlePart.remove(customWindowTitlePart.size() - 1, 1);
        }
        if (m_ui->tabWidget->readOnly(tabWidgetIndex)) {
            customWindowTitlePart.append(QString(" [%1]").arg(tr("read-only")));
        }
    } else if (stackedWidgetIndex == 1) {
        customWindowTitlePart = tr("Settings");
    }

    QString windowTitle;
    if (customWindowTitlePart.isEmpty()) {
        windowTitle = BaseWindowTitle;
    } else {
        windowTitle = QString("%1[*] - %2").arg(customWindowTitlePart, BaseWindowTitle);
    }

    if (customWindowTitlePart.isEmpty() || stackedWidgetIndex == 1) {
        setWindowFilePath("");
    } else {
        setWindowFilePath(m_ui->tabWidget->databasePath(tabWidgetIndex));
    }

    setWindowModified(isModified);

    setWindowTitle(windowTitle);
}

void MainWindow::showAboutDialog()
{
    AboutDialog* aboutDialog = new AboutDialog(this);
    aboutDialog->open();
}

void MainWindow::switchToDatabases()
{
    if (m_ui->tabWidget->currentIndex() == -1) {
        m_ui->stackedWidget->setCurrentIndex(WelcomeScreen);
    }
    else {
        m_ui->stackedWidget->setCurrentIndex(DatabaseTabScreen);
    }
}

void MainWindow::switchToSettings()
{
    m_ui->settingsWidget->loadSettings();
    m_ui->stackedWidget->setCurrentIndex(SettingsScreen);
}

void MainWindow::switchToPasswordGen(bool enabled)
{
    if (enabled == true) {
      m_ui->passwordGeneratorWidget->loadSettings();
      m_ui->passwordGeneratorWidget->regeneratePassword();
      m_ui->passwordGeneratorWidget->setStandaloneMode(true);
      m_ui->stackedWidget->setCurrentIndex(PasswordGeneratorScreen);
    } else {
      m_ui->passwordGeneratorWidget->saveSettings();
      switchToDatabases();
    }
}

void MainWindow::closePasswordGen()
{
    switchToPasswordGen(false);
}

void MainWindow::switchToNewDatabase()
{
    m_ui->tabWidget->newDatabase();
    switchToDatabases();
}

void MainWindow::switchToOpenDatabase()
{
    m_ui->tabWidget->openDatabase();
    switchToDatabases();
}

void MainWindow::switchToDatabaseFile(QString file)
{
    m_ui->tabWidget->openDatabase(file);
    switchToDatabases();
}

void MainWindow::switchToKeePass1Database()
{
    m_ui->tabWidget->importKeePass1Database();
    switchToDatabases();
}

void MainWindow::switchToImportCsv()
{
    m_ui->tabWidget->importCsv();
    switchToDatabases();
}

void MainWindow::databaseStatusChanged(DatabaseWidget *)
{
    updateTrayIcon();
}

void MainWindow::databaseTabChanged(int tabIndex)
{
    if (tabIndex != -1 && m_ui->stackedWidget->currentIndex() == WelcomeScreen) {
        m_ui->stackedWidget->setCurrentIndex(DatabaseTabScreen);
    }
    else if (tabIndex == -1 && m_ui->stackedWidget->currentIndex() == DatabaseTabScreen) {
        m_ui->stackedWidget->setCurrentIndex(WelcomeScreen);
    }

    m_actionMultiplexer.setCurrentObject(m_ui->tabWidget->currentDatabaseWidget());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // ignore double close events (happens on macOS when closing from the dock)
    if (m_appExiting) {
        event->accept();
        return;
    }

    bool minimizeOnClose = isTrayIconEnabled() &&
                           config()->get("GUI/MinimizeOnClose").toBool();
    if (minimizeOnClose && !m_appExitCalled)
    {
        event->ignore();
        hideWindow();

        if (config()->get("security/lockdatabaseminimize").toBool()) {
            m_ui->tabWidget->lockDatabases();
        }

        return;
    }

    bool accept = saveLastDatabases();

    if (accept) {
        m_appExiting = true;
        saveWindowInformation();

        event->accept();
        QApplication::quit();
    }
    else {
        event->ignore();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::WindowStateChange) && isMinimized()) {
        if (isTrayIconEnabled() && m_trayIcon && m_trayIcon->isVisible()
                && config()->get("GUI/MinimizeToTray").toBool())
        {
            event->ignore();
            QTimer::singleShot(0, this, SLOT(hide()));
        }

        if (config()->get("security/lockdatabaseminimize").toBool()) {
            m_ui->tabWidget->lockDatabases();
        }
    }
    else {
        QMainWindow::changeEvent(event);
    }
}

void MainWindow::saveWindowInformation()
{
    config()->set("GUI/MainWindowGeometry", saveGeometry());
}

bool MainWindow::saveLastDatabases()
{
    bool accept;
    m_openDatabases.clear();
    bool openPreviousDatabasesOnStartup = config()->get("OpenPreviousDatabasesOnStartup").toBool();

    if (openPreviousDatabasesOnStartup) {
        connect(m_ui->tabWidget, SIGNAL(databaseWithFileClosed(QString)),
                this, SLOT(rememberOpenDatabases(QString)));
    }

    if (!m_ui->tabWidget->closeAllDatabases()) {
        accept = false;
    }
    else {
        accept = true;
    }

    if (openPreviousDatabasesOnStartup) {
        disconnect(m_ui->tabWidget, SIGNAL(databaseWithFileClosed(QString)),
                   this, SLOT(rememberOpenDatabases(QString)));
        config()->set("LastOpenedDatabases", m_openDatabases);
    }

    return accept;
}

void MainWindow::updateTrayIcon()
{
    if (isTrayIconEnabled()) {
        if (!m_trayIcon) {
            m_trayIcon = new QSystemTrayIcon(this);

            QMenu* menu = new QMenu(this);

            QAction* actionToggle = new QAction(tr("Toggle window"), menu);
            menu->addAction(actionToggle);

#ifdef Q_OS_MAC
            QAction* actionQuit = new QAction(tr("Quit KeePassXC"), menu);
            menu->addAction(actionQuit);

            connect(actionQuit, SIGNAL(triggered()), SLOT(appExit()));
#else
            menu->addAction(m_ui->actionQuit);

            connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                    SLOT(trayIconTriggered(QSystemTrayIcon::ActivationReason)));
#endif
            connect(actionToggle, SIGNAL(triggered()), SLOT(toggleWindow()));

            m_trayIcon->setContextMenu(menu);
            m_trayIcon->setIcon(filePath()->applicationIcon());
            m_trayIcon->show();
        }
        if (m_ui->tabWidget->hasLockableDatabases()) {
            m_trayIcon->setIcon(filePath()->trayIconUnlocked());
        }
        else {
            m_trayIcon->setIcon(filePath()->trayIconLocked());
        }
    }
    else {
        if (m_trayIcon) {
            m_trayIcon->hide();
            delete m_trayIcon;
            m_trayIcon = nullptr;
        }
    }
}

void MainWindow::showEntryContextMenu(const QPoint& globalPos)
{
    m_ui->menuEntries->popup(globalPos);
}

void MainWindow::showGroupContextMenu(const QPoint& globalPos)
{
    m_ui->menuGroups->popup(globalPos);
}

void MainWindow::saveToolbarState(bool value)
{
    config()->set("ShowToolbar", value);
}

void MainWindow::setShortcut(QAction* action, QKeySequence::StandardKey standard, int fallback)
{
    if (!QKeySequence::keyBindings(standard).isEmpty()) {
        action->setShortcuts(standard);
    }
    else if (fallback != 0) {
        action->setShortcut(QKeySequence(fallback));
    }
}

void MainWindow::rememberOpenDatabases(const QString& filePath)
{
    m_openDatabases.append(filePath);
}

void MainWindow::applySettingsChanges()
{
    int timeout = config()->get("security/lockdatabaseidlesec").toInt() * 1000;
    if (timeout <= 0) {
        timeout = 60;
    }

    m_inactivityTimer->setInactivityTimeout(timeout);
    if (config()->get("security/lockdatabaseidle").toBool()) {
        m_inactivityTimer->activate();
    }
    else {
        m_inactivityTimer->deactivate();
    }

    updateTrayIcon();
}

void MainWindow::trayIconTriggered(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        toggleWindow();
    }
}

void MainWindow::hideWindow()
{
#ifndef Q_OS_MAC
    setWindowState(windowState() | Qt::WindowMinimized);
#endif
    QTimer::singleShot(0, this, SLOT(hide()));

    if (config()->get("security/lockdatabaseminimize").toBool()) {
        m_ui->tabWidget->lockDatabases();
    }
}

void MainWindow::toggleWindow()
{
    if ((QApplication::activeWindow() == this) && isVisible() && !isMinimized()) {
        hideWindow();
    } else {
        ensurePolished();
        setWindowState(windowState() & ~Qt::WindowMinimized);
        show();
        raise();
        activateWindow();

#if defined(Q_OS_LINUX) && ! defined(QT_NO_DBUS)
        // re-register global D-Bus menu (needed on Ubuntu with Unity)
        // see https://github.com/keepassxreboot/keepassxc/issues/271
        // and https://bugreports.qt.io/browse/QTBUG-58723
        // check for !isVisible(), because isNativeMenuBar() does not work with appmenu-qt5
        if (!m_ui->menubar->isVisible()) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                "com.canonical.AppMenu.Registrar",
                "/com/canonical/AppMenu/Registrar",
                "com.canonical.AppMenu.Registrar",
                "RegisterWindow");
            QList<QVariant> args;
            args << QVariant::fromValue(static_cast<uint32_t>(winId()))
                 << QVariant::fromValue(QDBusObjectPath("/MenuBar/1"));
            msg.setArguments(args);
            QDBusConnection::sessionBus().send(msg);
        }
#endif
    }
}

void MainWindow::lockDatabasesAfterInactivity()
{
    // ignore event if a modal dialog is open (such as a message box or file dialog)
    if (QApplication::activeModalWidget()) {
        return;
    }

    m_ui->tabWidget->lockDatabases();
}

void MainWindow::repairDatabase()
{
    QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open database"), QString(),
                                                     filter);
    if (fileName.isEmpty()) {
        return;
    }

    QScopedPointer<QDialog> dialog(new QDialog(this));
    DatabaseRepairWidget* dbRepairWidget = new DatabaseRepairWidget(dialog.data());
    connect(dbRepairWidget, SIGNAL(success()), dialog.data(), SLOT(accept()));
    connect(dbRepairWidget, SIGNAL(error()), dialog.data(), SLOT(reject()));
    dbRepairWidget->load(fileName);
    if (dialog->exec() == QDialog::Accepted && dbRepairWidget->database()) {
        QString saveFileName = fileDialog()->getSaveFileName(this, tr("Save repaired database"), QString(),
                                                             tr("KeePass 2 Database").append(" (*.kdbx)"),
                                                             nullptr, 0, "kdbx");

        if (!saveFileName.isEmpty()) {
            KeePass2Writer writer;
            writer.writeDatabase(saveFileName, dbRepairWidget->database());
            if (writer.hasError()) {
                displayGlobalMessage(
                    tr("Writing the database failed.").append("\n").append(writer.errorString()),
                    MessageWidget::Error);
            }
        }
    }
}

bool MainWindow::isTrayIconEnabled() const
{
    return config()->get("GUI/ShowTrayIcon").toBool()
            && QSystemTrayIcon::isSystemTrayAvailable();
}

void MainWindow::displayGlobalMessage(const QString& text, MessageWidget::MessageType type, bool showClosebutton)
{
    m_ui->globalMessageWidget->setCloseButtonVisible(showClosebutton);
    m_ui->globalMessageWidget->showMessage(text, type);
}

void MainWindow::displayTabMessage(const QString& text, MessageWidget::MessageType type, bool showClosebutton)
{
    m_ui->globalMessageWidget->setCloseButtonVisible(showClosebutton);
    m_ui->tabWidget->currentDatabaseWidget()->showMessage(text, type);
}

void MainWindow::hideGlobalMessage()
{
    m_ui->globalMessageWidget->hideMessage();
}

void MainWindow::hideTabMessage()
{
    if (m_ui->stackedWidget->currentIndex() == DatabaseTabScreen) {
        m_ui->tabWidget->currentDatabaseWidget()->hideMessage();
    }
}

void MainWindow::showYubiKeyPopup()
{
    displayGlobalMessage(tr("Please touch the button on your YubiKey!"), MessageWidget::Information, false);
    setEnabled(false);
}

void MainWindow::hideYubiKeyPopup()
{
    hideGlobalMessage();
    setEnabled(true);
}

void MainWindow::handleScreenLock()
{
    if (config()->get("security/lockdatabasescreenlock").toBool()){
        lockDatabasesAfterInactivity();
    }
}

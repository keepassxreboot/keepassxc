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

#ifndef KEEPASSX_MAINWINDOW_H
#define KEEPASSX_MAINWINDOW_H

#include <QActionGroup>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include "core/ScreenLockListener.h"
#include "core/SignalMultiplexer.h"
#include "gui/Application.h"
#include "gui/DatabaseWidget.h"

namespace Ui
{
    class MainWindow;
}

class InactivityTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(QT_NO_DBUS)
    Q_CLASSINFO("D-Bus Interface", "org.keepassxc.KeePassXC.MainWindow")
#endif

public:
    MainWindow();
    ~MainWindow();

    enum StackedWidgetIndex
    {
        DatabaseTabScreen = 0,
        SettingsScreen = 1,
        WelcomeScreen = 2,
        PasswordGeneratorScreen = 3
    };

public slots:
    void openDatabase(const QString& filePath, const QString& pw = {}, const QString& keyFile = {});
    void appExit();
    void displayGlobalMessage(const QString& text,
                              MessageWidget::MessageType type,
                              bool showClosebutton = true,
                              int autoHideTimeout = MessageWidget::DefaultAutoHideTimeout);
    void displayTabMessage(const QString& text,
                           MessageWidget::MessageType type,
                           bool showClosebutton = true,
                           int autoHideTimeout = MessageWidget::DefaultAutoHideTimeout);
    void hideGlobalMessage();
    void showYubiKeyPopup();
    void hideYubiKeyPopup();
    void hideWindow();
    void toggleWindow();
    void bringToFront();
    void closeAllDatabases();
    void lockAllDatabases();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void setMenuActionState(DatabaseWidget::Mode mode = DatabaseWidget::Mode::None);
    void updateWindowTitle();
    void showAboutDialog();
    void showUpdateCheckStartup();
    void showUpdateCheckDialog();
    void hasUpdateAvailable(bool hasUpdate, const QString& version);
    void openDonateUrl();
    void openBugReportUrl();
    void switchToDatabases();
    void switchToSettings(bool enabled);
    void switchToPasswordGen(bool enabled);
    void switchToNewDatabase();
    void switchToOpenDatabase();
    void switchToDatabaseFile(const QString& file);
    void switchToKeePass1Database();
    void switchToCsvImport();
    void closePasswordGen();
    void databaseStatusChanged(DatabaseWidget* dbWidget);
    void databaseTabChanged(int tabIndex);
    void openRecentDatabase(QAction* action);
    void clearLastDatabases();
    void updateLastDatabasesMenu();
    void updateCopyAttributesMenu();
    void showEntryContextMenu(const QPoint& globalPos);
    void showGroupContextMenu(const QPoint& globalPos);
    void rememberOpenDatabases(const QString& filePath);
    void applySettingsChanges();
    void trayIconTriggered(QSystemTrayIcon::ActivationReason reason);
    void lockDatabasesAfterInactivity();
    void forgetTouchIDAfterInactivity();
    void handleScreenLock();
    void showErrorMessage(const QString& message);
    void selectNextDatabaseTab();
    void selectPreviousDatabaseTab();
    void togglePasswordsHidden();
    void toggleUsernamesHidden();

private:
    static void setShortcut(QAction* action, QKeySequence::StandardKey standard, int fallback = 0);

    static const QString BaseWindowTitle;

    void saveWindowInformation();
    bool saveLastDatabases();
    void updateTrayIcon();
    bool isTrayIconEnabled() const;

    static QStringList kdbxFilesFromUrls(const QList<QUrl>& urls);
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    const QScopedPointer<Ui::MainWindow> m_ui;
    SignalMultiplexer m_actionMultiplexer;
    QAction* m_clearHistoryAction;
    QAction* m_searchWidgetAction;
    QActionGroup* m_lastDatabasesActions;
    QActionGroup* m_copyAdditionalAttributeActions;
    QStringList m_openDatabases;
    InactivityTimer* m_inactivityTimer;
    InactivityTimer* m_touchIDinactivityTimer;
    int m_countDefaultAttributes;
    QSystemTrayIcon* m_trayIcon;
    ScreenLockListener* m_screenLockListener;

    Q_DISABLE_COPY(MainWindow)

    bool m_appExitCalled;
    bool m_appExiting;
};

/**
 * Return instance of MainWindow created on app load
 * non-gui instances will return nullptr
 *
 * @return MainWindow instance or nullptr
 */
MainWindow* getMainWindow();

#endif // KEEPASSX_MAINWINDOW_H

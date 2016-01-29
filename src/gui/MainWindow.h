/*
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

#ifndef KEEPASSX_MAINWINDOW_H
#define KEEPASSX_MAINWINDOW_H

#include <QActionGroup>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include "core/SignalMultiplexer.h"
#include "gui/DatabaseWidget.h"

namespace Ui {
    class MainWindow;
}

class InactivityTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

public Q_SLOTS:
    void openDatabase(const QString& fileName, const QString& pw = QString(),
                      const QString& keyFile = QString());

protected:
     void closeEvent(QCloseEvent* event) override;
     void changeEvent(QEvent* event) override;

private Q_SLOTS:
    void setMenuActionState(DatabaseWidget::Mode mode = DatabaseWidget::None);
    void updateWindowTitle();
    void showAboutDialog();
    void switchToDatabases();
    void switchToSettings();
    void databaseTabChanged(int tabIndex);
    void openRecentDatabase(QAction* action);
    void clearLastDatabases();
    void updateLastDatabasesMenu();
    void updateCopyAttributesMenu();
    void showEntryContextMenu(const QPoint& globalPos);
    void showGroupContextMenu(const QPoint& globalPos);
    void saveToolbarState(bool value);
    void rememberOpenDatabases(const QString& filePath);
    void applySettingsChanges();
    void trayIconTriggered(QSystemTrayIcon::ActivationReason reason);
    void toggleWindow();
    void lockDatabasesAfterInactivity();
    void repairDatabase();

private:
    static void setShortcut(QAction* action, QKeySequence::StandardKey standard, int fallback = 0);

    static const QString BaseWindowTitle;

    void saveWindowInformation();
    bool saveLastDatabases();
    void updateTrayIcon();
    bool isTrayIconEnabled() const;

    const QScopedPointer<Ui::MainWindow> m_ui;
    SignalMultiplexer m_actionMultiplexer;
    QAction* m_clearHistoryAction;
    QActionGroup* m_lastDatabasesActions;
    QActionGroup* m_copyAdditionalAttributeActions;
    QStringList m_openDatabases;
    InactivityTimer* m_inactivityTimer;
    int m_countDefaultAttributes;
    QSystemTrayIcon* m_trayIcon;

    Q_DISABLE_COPY(MainWindow)
};

#endif // KEEPASSX_MAINWINDOW_H

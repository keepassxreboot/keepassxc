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

#ifndef KEEPASSXC_TESTCLOUDSYNCWIDGET_H
#define KEEPASSXC_TESTCLOUDSYNCWIDGET_H

#include "gui/MainWindow.h"
#include "util/TemporaryFile.h"

#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>

class Database;
class DatabaseSettingsWidgetCloudSync;
class DatabaseTabWidget;
class DatabaseWidget;
class QPushButton;

class TestCloudSyncWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void CloudSettingNotImpactedWhileExploringOtherProviders();
    void CloudSettingSwitchProviderRemoveOldOne();

private:
    void triggerAction(const QString& name);
    void openCloudSyncSettings();

    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;
    DatabaseSettingsWidgetCloudSync* m_widget = nullptr; // borrowed -- lives on DatabaseSettingsDialog
    QPushButton* m_applyButton = nullptr;                 // borrowed -- lives on DatabaseSettingsDialog's buttonBox
    TemporaryFile m_dbFile;
    QString m_dbFilePath;
};

#endif // KEEPASSXC_TESTCLOUDSYNCWIDGET_H

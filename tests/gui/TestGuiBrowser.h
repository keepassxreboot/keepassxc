/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTGUIBROWSER_H
#define KEEPASSXC_TESTGUIBROWSER_H

#include "gui/MainWindow.h"
#include "util/TemporaryFile.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class QAbstractItemView;

class TestGuiBrowser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testEntrySettings();
    void testAdditionalURLs();
    void testGetDatabaseGroups();

private:
    void triggerAction(const QString& name);
    void clickIndex(const QModelIndex& index,
                    QAbstractItemView* view,
                    Qt::MouseButton button,
                    Qt::KeyboardModifiers stateKey = 0);

    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;
    QScopedPointer<TemporaryFile> m_dbFile;
};

#endif // KEEPASSXC_TESTGUIBROWSER_H

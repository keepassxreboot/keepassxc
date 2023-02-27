/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSXC_GUI_TEST_BASE_H
#define KEEPASSXC_GUI_TEST_BASE_H

#include "core/Tools.h"
#include "gui/MainWindow.h"
#include "gui/entry/EntryView.h"
#include "util/TemporaryFile.h"

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class QAbstractItemView;
class Entry;
class EntryView;

class GuiTestBase : public QObject
{
    Q_OBJECT

protected slots:
    virtual void initTestCase();
    virtual void init();
    virtual void cleanup();
    virtual void cleanupTestCase();

protected:
    Entry* selectEntryViewRow(int row);
    EntryView* findEntryView();

    void triggerAction(const QString& name);

    static void clickDialogButton(QWidget* dialog, QDialogButtonBox::StandardButton button);
    static void clickIndex(const QModelIndex& index,
                           QAbstractItemView* view,
                           Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey = 0);

    static void wait(int ms = 2000)
    {
        Tools::wait(ms);
    }

    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<QLabel> m_statusBarLabel;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;
    TemporaryFile m_dbFile;
    QString m_dbFileName;
    QString m_dbFilePath;
};

#endif // KEEPASSXC_GUI_TEST_BASE_H

/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#ifndef KEEPASSXC_FIXTUREBASE_H
#define KEEPASSXC_FIXTUREBASE_H

#include "core/Tools.h"
#include "gui/MainWindow.h"

// A base fixture with initialized MainWindow and default config with custom settings for GUI tests.
// Can be used as standalone fixture for scenario where open and unlocked DB is not needed.
// Should be used as a parent class for other fixtures, for example with open DB.
// Contains helpful utility functions to reduce duplicates in tests.
class FixtureBase
{
public:
    FixtureBase();

protected:
    static QToolBar* findToolBar();
    static QAction* findAction(const QString& actionName);
    static void triggerAction(const QString& name);
    static void clickToolbarButton(const QString& actionName);

    static QString findLabelText(const QWidget* pWidget, const QString& labelName);
    static void escape(QWidget* pWidget, int waitMs = 250);
    static void wait(int ms);

protected:
    // Have one single main window for all GUI tests.
    static QPointer<MainWindow> m_mainWindow;
    static QPointer<QLabel> m_statusBarLabel;
};

#endif // KEEPASSXC_FIXTUREBASE_H

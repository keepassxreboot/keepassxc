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
#include "FixtureBase.h"
#include "../../tests/util/TemporaryFile.h"
#include "catch2/catch_all.hpp"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QLabel>
#include <QTest>
#include <QToolBar>

QPointer<MainWindow> FixtureBase::m_mainWindow(nullptr);
QPointer<QLabel> FixtureBase::m_statusBarLabel(nullptr);

FixtureBase::FixtureBase()
{
    if (m_mainWindow.isNull()) {
        m_mainWindow = new MainWindow();
        m_mainWindow->resize(1024, 768);
        m_mainWindow->show();
        m_mainWindow->activateWindow();

        QApplication::processEvents();

        m_statusBarLabel = m_mainWindow->findChild<QLabel*>("statusBarLabel");
    }

    // Every test starts with resetting of config settings
    config()->resetToDefaults();
    // Disable auto-save so we can test the modified file indicator
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    // Enable the tray icon, so we can test hiding/restoring the windowQByteArray
    config()->set(Config::GUI_ShowTrayIcon, true);
    // Disable the update check first time alert
    config()->set(Config::UpdateCheckMessageShown, true);
    // Disable quick unlock
    config()->set(Config::Security_QuickUnlock, false);
    // Disable atomic saves to prevent transient errors on some platforms
    config()->set(Config::UseAtomicSaves, false);
    // Disable showing expired entries on unlock
    config()->set(Config::GUI_ShowExpiredEntriesOnDatabaseUnlock, false);
    // Disabling showing pre-release warning
    config()->set(Config::Messages_HidePreReleaseWarning, true);
}

QToolBar* FixtureBase::findToolBar()
{
    auto* pToolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    REQUIRE(pToolBar);

    return pToolBar;
}

QAction* FixtureBase::findAction(const QString& actionName)
{
    auto* pAction = m_mainWindow->findChild<QAction*>(actionName);
    REQUIRE(pAction);

    return pAction;
}

void FixtureBase::triggerAction(const QString& name)
{
    auto* pAction = findAction(name);
    REQUIRE(pAction->isEnabled());
    pAction->trigger();
    QApplication::processEvents();
}

void FixtureBase::clickToolbarButton(const QString& actionName)
{
    auto* pToolBar = findToolBar();

    auto* pAction = findAction(actionName);
    REQUIRE(pAction->isEnabled());

    auto* pWidget = pToolBar->widgetForAction(pAction);
    REQUIRE(pWidget->isVisible());
    REQUIRE(pWidget->isEnabled());

    QTest::mouseClick(pWidget, Qt::LeftButton);
    QApplication::processEvents();
    wait(100); // let the action to be completed
}

QString FixtureBase::findLabelText(const QWidget* pWidget, const QString& labelName)
{
    auto* pLabel = pWidget->findChild<QLabel*>(labelName);
    REQUIRE(pLabel);

    return pLabel->text();
}

void FixtureBase::escape(QWidget* pWidget, const int waitMs)
{
    QTest::keyClick(pWidget, Qt::Key_Escape);
    wait(waitMs);
}

void FixtureBase::wait(const int ms)
{
    Tools::wait(ms);
}

// Generates a temporary filename. The created file will be removed automatically by ~TemporaryFile().
QString FixtureBase::newTempFileName()
{
    TemporaryFile tmpFile;
    return tmpFile.fileName();
}

void FixtureBase::checkStatusBarText(const QString& textFragment)
{
    QApplication::processEvents();
    REQUIRE(m_statusBarLabel->isVisible());
    REQUIRE(m_statusBarLabel->text().startsWith(textFragment));
    // TODO Catch2 does not have a way to print a custom message in case of fail. Implement own way?
    // qPrintable(QString("'%1' doesn't start with '%2'").arg(m_statusBarLabel->text(), textFragment)));
}

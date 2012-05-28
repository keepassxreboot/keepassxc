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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QtGui/QCloseEvent>

#include "core/Database.h"
#include "core/DataPath.h"
#include "core/Metadata.h"
#include "gui/AboutDialog.h"
#include "gui/DatabaseWidget.h"
#include "gui/entry/EntryView.h"

MainWindow::MainWindow()
    : m_ui(new Ui::MainWindow())
{
    m_ui->setupUi(this);

    setWindowIcon(dataPath()->applicationIcon());
    m_ui->toolBar->toggleViewAction()->setText(tr("Show toolbar"));

    setShortcut(m_ui->actionDatabaseOpen, QKeySequence::Open, Qt::CTRL + Qt::Key_O);
    setShortcut(m_ui->actionDatabaseSave, QKeySequence::Save, Qt::CTRL + Qt::Key_S);
    setShortcut(m_ui->actionDatabaseSaveAs, QKeySequence::SaveAs);
    setShortcut(m_ui->actionDatabaseClose, QKeySequence::Close, Qt::CTRL + Qt::Key_W);
    setShortcut(m_ui->actionQuit, QKeySequence::Quit, Qt::CTRL + Qt::Key_Q);
    setShortcut(m_ui->actionSearch, QKeySequence::Find, Qt::CTRL + Qt::Key_F);
    m_ui->actionEntryCopyUsername->setShortcut(Qt::CTRL + Qt::Key_B);
    m_ui->actionEntryCopyPassword->setShortcut(Qt::CTRL + Qt::Key_C);

    m_ui->actionDatabaseNew->setIcon(dataPath()->icon("actions", "document-new"));
    m_ui->actionDatabaseOpen->setIcon(dataPath()->icon("actions", "document-open"));
    m_ui->actionDatabaseSave->setIcon(dataPath()->icon("actions", "document-save"));
    m_ui->actionDatabaseSaveAs->setIcon(dataPath()->icon("actions", "document-save-as"));
    m_ui->actionDatabaseClose->setIcon(dataPath()->icon("actions", "document-close"));
    m_ui->actionChangeDatabaseSettings->setIcon(dataPath()->icon("actions", "document-edit"));
    m_ui->actionChangeMasterKey->setIcon(dataPath()->icon("actions", "database-change-key", false));
    m_ui->actionQuit->setIcon(dataPath()->icon("actions", "application-exit"));

    m_ui->actionEntryNew->setIcon(dataPath()->icon("actions", "entry-new", false));
    m_ui->actionEntryClone->setIcon(dataPath()->icon("actions", "entry-clone", false));
    m_ui->actionEntryEdit->setIcon(dataPath()->icon("actions", "entry-edit", false));
    m_ui->actionEntryDelete->setIcon(dataPath()->icon("actions", "entry-delete", false));

    m_ui->actionGroupNew->setIcon(dataPath()->icon("actions", "group-new", false));
    m_ui->actionGroupEdit->setIcon(dataPath()->icon("actions", "group-edit", false));
    m_ui->actionGroupDelete->setIcon(dataPath()->icon("actions", "group-delete", false));

    m_ui->actionSettings->setIcon(dataPath()->icon("actions", "configure"));

    m_ui->actionAbout->setIcon(dataPath()->icon("actions", "help-about"));

    m_ui->actionSearch->setIcon(dataPath()->icon("actions", "system-search"));

    connect(m_ui->tabWidget, SIGNAL(entrySelectionChanged(bool)),
            SLOT(setMenuActionState()));
    connect(m_ui->tabWidget, SIGNAL(currentWidgetModeChanged(DatabaseWidget::Mode)),
            SLOT(setMenuActionState(DatabaseWidget::Mode)));
    connect(m_ui->tabWidget, SIGNAL(tabNameChanged()),
            SLOT(updateWindowTitle()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(updateWindowTitle()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(databaseTabChanged(int)));
    connect(m_ui->stackedWidget, SIGNAL(currentChanged(int)), SLOT(setMenuActionState()));
    connect(m_ui->settingsWidget, SIGNAL(editFinished(bool)), SLOT(switchToDatabases()));

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
    connect(m_ui->actionChangeMasterKey, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(changeMasterKey()));
    connect(m_ui->actionChangeDatabaseSettings, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(changeDatabaseSettings()));
    connect(m_ui->actionImportKeePass1, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(importKeePass1Database()));
    connect(m_ui->actionQuit, SIGNAL(triggered()), SLOT(close()));

    connect(m_ui->actionEntryNew, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(createEntry()));
    connect(m_ui->actionEntryClone, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(cloneEntry()));
    connect(m_ui->actionEntryEdit, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(editEntry()));
    connect(m_ui->actionEntryDelete, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(deleteEntry()));
    connect(m_ui->actionEntryCopyUsername, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(copyUsername()));
    connect(m_ui->actionEntryCopyPassword, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(copyPassword()));

    connect(m_ui->actionGroupNew, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(createGroup()));
    connect(m_ui->actionGroupEdit, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(editGroup()));
    connect(m_ui->actionGroupDelete, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(deleteGroup()));

    connect(m_ui->actionSettings, SIGNAL(triggered()), SLOT(switchToSettings()));

    connect(m_ui->actionAbout, SIGNAL(triggered()), SLOT(showAboutDialog()));

    connect(m_ui->actionSearch, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(toggleSearch()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::openDatabase(const QString& fileName, const QString& pw, const QString& keyFile)
{
    m_ui->tabWidget->openDatabase(fileName, pw, keyFile);
}

const QString MainWindow::BaseWindowTitle = "KeePassX";

void MainWindow::setMenuActionState(DatabaseWidget::Mode mode)
{
    bool inDatabaseTabWidget = (m_ui->stackedWidget->currentIndex() == 0);
    bool inWelcomeWidget = (m_ui->stackedWidget->currentIndex() == 2);

    if (inDatabaseTabWidget && m_ui->tabWidget->currentIndex() != -1) {
        DatabaseWidget* dbWidget = m_ui->tabWidget->currentDatabaseWidget();
        Q_ASSERT(dbWidget);

        if (mode == DatabaseWidget::None) {
            mode = dbWidget->currentMode();
        }

        switch (mode) {
        case DatabaseWidget::ViewMode: {
            m_ui->actionEntryNew->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryNew));
            m_ui->actionEntryClone->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryClone));
            m_ui->actionEntryEdit->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryEditView));
            m_ui->actionEntryDelete->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryDelete));
            m_ui->actionEntryCopyUsername->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryCopyUsername));
            m_ui->actionEntryCopyPassword->setEnabled(dbWidget->actionEnabled(DatabaseWidget::EntryCopyPassword));
            m_ui->actionGroupNew->setEnabled(dbWidget->actionEnabled(DatabaseWidget::GroupNew));
            m_ui->actionGroupEdit->setEnabled(dbWidget->actionEnabled(DatabaseWidget::GroupEdit));
            m_ui->actionGroupDelete->setEnabled(dbWidget->actionEnabled(DatabaseWidget::GroupDelete));
            m_ui->actionSearch->setEnabled(true);
            // TODO: get checked state from db widget
            m_ui->actionSearch->setChecked(dbWidget->entryView()->inSearch());
            m_ui->actionChangeMasterKey->setEnabled(true);
            m_ui->actionChangeDatabaseSettings->setEnabled(true);
            m_ui->actionDatabaseSave->setEnabled(true);
            m_ui->actionDatabaseSaveAs->setEnabled(true);
            break;
        }
        case DatabaseWidget::EditMode:
            m_ui->actionEntryNew->setEnabled(false);
            m_ui->actionEntryClone->setEnabled(false);
            m_ui->actionEntryEdit->setEnabled(false);
            m_ui->actionEntryDelete->setEnabled(false);
            m_ui->actionEntryCopyUsername->setEnabled(false);
            m_ui->actionEntryCopyPassword->setEnabled(false);
            m_ui->actionGroupNew->setEnabled(false);
            m_ui->actionGroupEdit->setEnabled(false);
            m_ui->actionGroupDelete->setEnabled(false);
            m_ui->actionSearch->setEnabled(false);
            m_ui->actionSearch->setChecked(false);
            m_ui->actionChangeMasterKey->setEnabled(false);
            m_ui->actionChangeDatabaseSettings->setEnabled(false);
            m_ui->actionDatabaseSave->setEnabled(false);
            m_ui->actionDatabaseSaveAs->setEnabled(false);
            break;
        default:
            Q_ASSERT(false);
        }
        m_ui->actionDatabaseClose->setEnabled(true);
    }
    else {
        m_ui->actionEntryNew->setEnabled(false);
        m_ui->actionEntryClone->setEnabled(false);
        m_ui->actionEntryEdit->setEnabled(false);
        m_ui->actionEntryDelete->setEnabled(false);
        m_ui->actionEntryCopyUsername->setEnabled(false);
        m_ui->actionEntryCopyPassword->setEnabled(false);
        m_ui->actionGroupNew->setEnabled(false);
        m_ui->actionGroupEdit->setEnabled(false);
        m_ui->actionGroupDelete->setEnabled(false);
        m_ui->actionSearch->setEnabled(false);
        m_ui->actionSearch->setChecked(false);
        m_ui->actionChangeMasterKey->setEnabled(false);
        m_ui->actionChangeDatabaseSettings->setEnabled(false);
        m_ui->actionDatabaseSave->setEnabled(false);
        m_ui->actionDatabaseSaveAs->setEnabled(false);

        m_ui->actionDatabaseClose->setEnabled(false);
    }

    m_ui->actionDatabaseNew->setEnabled(inDatabaseTabWidget || inWelcomeWidget);
    m_ui->actionDatabaseOpen->setEnabled(inDatabaseTabWidget || inWelcomeWidget);
    m_ui->actionImportKeePass1->setEnabled(inDatabaseTabWidget || inWelcomeWidget);
}

void MainWindow::updateWindowTitle()
{
    int index = m_ui->tabWidget->currentIndex();
    if (index == -1) {
        setWindowTitle(BaseWindowTitle);
    }
    else {
        setWindowTitle(m_ui->tabWidget->tabText(index).append(" - ").append(BaseWindowTitle));
    }
}

void MainWindow::showAboutDialog()
{
    AboutDialog* aboutDialog = new AboutDialog(this);
    aboutDialog->show();
}

void MainWindow::switchToDatabases()
{
    m_ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::switchToSettings()
{
    m_ui->settingsWidget->loadSettings();
    m_ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::databaseTabChanged(int tabIndex)
{
    if (tabIndex != -1 && m_ui->stackedWidget->currentIndex() == 2) {
        switchToDatabases();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_ui->tabWidget->closeAllDatabases()) {
        event->ignore();
    }
    else {
        event->accept();
    }
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

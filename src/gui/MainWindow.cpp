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
#include "gui/EntryView.h"

MainWindow::MainWindow()
    : m_ui(new Ui::MainWindow())
{
    m_ui->setupUi(this);

    setWindowIcon(dataPath()->applicationIcon());
    m_ui->toolBar->toggleViewAction()->setText(tr("Show toolbar"));

    connect(m_ui->tabWidget, SIGNAL(entrySelectionChanged(bool)),
            SLOT(setMenuActionState()));
    connect(m_ui->tabWidget, SIGNAL(currentWidgetModeChanged(DatabaseWidget::Mode)),
            SLOT(setMenuActionState(DatabaseWidget::Mode)));
    connect(m_ui->tabWidget, SIGNAL(tabNameChanged()),
            SLOT(updateWindowTitle()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)),
            SLOT(updateWindowTitle()));

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
    connect(m_ui->actionAbout, SIGNAL(triggered()), SLOT(showAboutDialog()));

    connect(m_ui->actionEntryNew, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(createEntry()));
    connect(m_ui->actionEntryEdit, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(editEntry()));
    connect(m_ui->actionEntryDelete, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(deleteEntry()));

    connect(m_ui->actionGroupNew, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(createGroup()));
    connect(m_ui->actionGroupEdit, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(editGroup()));
    connect(m_ui->actionGroupDelete, SIGNAL(triggered()), m_ui->tabWidget,
            SLOT(deleteGroup()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::openDatabase(const QString& fileName, const QString& pw, const QString& keyFile)
{
    m_ui->tabWidget->openDatabase(fileName, pw, keyFile);
}

const QString MainWindow::m_baseWindowTitle = "KeePassX";

void MainWindow::setMenuActionState(DatabaseWidget::Mode mode)
{
    if (m_ui->tabWidget->currentIndex() != -1) {
        m_ui->actionDatabaseClose->setEnabled(true);
        DatabaseWidget* dbWidget = m_ui->tabWidget->currentDatabaseWidget();
        Q_ASSERT(dbWidget);

        if (mode == DatabaseWidget::None) {
            mode = dbWidget->currentMode();
        }

        switch (mode) {
        case DatabaseWidget::ViewMode:
            m_ui->actionEntryNew->setEnabled(true);
            m_ui->actionGroupNew->setEnabled(true);
            if (dbWidget->entryView()->currentIndex().isValid()) {
               m_ui->actionEntryEdit->setEnabled(true);
               m_ui->actionEntryDelete->setEnabled(true);
            }
            else {
                m_ui->actionEntryEdit->setEnabled(false);
                m_ui->actionEntryDelete->setEnabled(false);
            }
            m_ui->actionGroupEdit->setEnabled(true);

            if (dbWidget->canDeleteCurrentGoup()) {
                    m_ui->actionGroupDelete->setEnabled(true);
            }
            else {
                m_ui->actionGroupDelete->setEnabled(false);
            }
            m_ui->actionChangeMasterKey->setEnabled(true);
            m_ui->actionChangeDatabaseSettings->setEnabled(true);
            m_ui->actionDatabaseSave->setEnabled(true);
            m_ui->actionDatabaseSaveAs->setEnabled(true);
            break;
        case DatabaseWidget::EditMode:
            m_ui->actionEntryNew->setEnabled(false);
            m_ui->actionGroupNew->setEnabled(false);
            m_ui->actionEntryEdit->setEnabled(false);
            m_ui->actionGroupEdit->setEnabled(false);
            m_ui->actionEntryDelete->setEnabled(false);
            m_ui->actionGroupDelete->setEnabled(false);
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
        m_ui->actionGroupNew->setEnabled(false);
        m_ui->actionEntryEdit->setEnabled(false);
        m_ui->actionGroupEdit->setEnabled(false);
        m_ui->actionEntryDelete->setEnabled(false);
        m_ui->actionGroupDelete->setEnabled(false);
        m_ui->actionChangeMasterKey->setEnabled(false);
        m_ui->actionChangeDatabaseSettings->setEnabled(false);
        m_ui->actionDatabaseSave->setEnabled(false);
        m_ui->actionDatabaseSaveAs->setEnabled(false);

        m_ui->actionDatabaseClose->setEnabled(false);
    }
}

void MainWindow::updateWindowTitle()
{
    int index = m_ui->tabWidget->currentIndex();
    if (index == -1) {
        setWindowTitle(m_baseWindowTitle);
    }
    else {
        setWindowTitle(m_ui->tabWidget->tabText(index).append(" - ").append(m_baseWindowTitle));
    }
}

void MainWindow::showAboutDialog()
{
    AboutDialog* aboutDialog = new AboutDialog(this);
    aboutDialog->show();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_ui->tabWidget->closeAllDatabases()) {
        event->ignore();
    }
    else {
        event->accept();
    }
}

#include <utility>

/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "NewDatabaseWizardPage.h"
#include "ui_NewDatabaseWizardPage.h"

#include "core/Database.h"
#include "gui/dbsettings/DatabaseSettingsWidget.h"

#include <QVBoxLayout>

NewDatabaseWizardPage::NewDatabaseWizardPage(QWidget* parent)
    : QWizardPage(parent)
    , m_ui(new Ui::NewDatabaseWizardPage())
{
    m_ui->setupUi(this);

    connect(m_ui->advancedSettingsButton, SIGNAL(clicked()), SLOT(toggleAdvancedSettings()));
}

NewDatabaseWizardPage::~NewDatabaseWizardPage()
{
}

/**
 * Set the database settings page widget for this wizard page.
 * The wizard page will take ownership of the settings page widget.
 *
 * @param page database settings page widget
 */
void NewDatabaseWizardPage::setPageWidget(DatabaseSettingsWidget* page)
{
    m_pageWidget = page;
    m_ui->pageContent->setWidget(m_pageWidget);
    m_ui->advancedSettingsButton->setVisible(m_pageWidget->hasAdvancedMode());
}

/**
 * @return database settings widget of this page widget.
 */
DatabaseSettingsWidget* NewDatabaseWizardPage::pageWidget()
{
    return m_pageWidget;
}

/**
 * Set the database to be configured by the wizard page.
 * The wizard will NOT take ownership of the database object.
 *
 * @param db database object to be configured
 */
void NewDatabaseWizardPage::setDatabase(QSharedPointer<Database> db)
{
    m_db = std::move(db);
}

void NewDatabaseWizardPage::initializePage()
{
    Q_ASSERT(m_pageWidget && m_db);
    if (!m_pageWidget || !m_db) {
        return;
    }

    m_pageWidget->load(m_db);
}

bool NewDatabaseWizardPage::validatePage()
{
    Q_ASSERT(m_pageWidget && m_db);
    if (!m_pageWidget || !m_db) {
        return false;
    }

    bool valid = m_pageWidget->save();
    m_pageWidget->uninitialize();
    return valid;
}

/**
 * Toggle settings page widget between simple and advanced mode.
 */
void NewDatabaseWizardPage::toggleAdvancedSettings()
{
    if (!m_pageWidget || !m_pageWidget->hasAdvancedMode()) {
        return;
    }

    if (m_pageWidget->advancedMode()) {
        m_pageWidget->setAdvancedMode(false);
        m_ui->advancedSettingsButton->setText(tr("Advanced Settings"));
    } else {
        m_pageWidget->setAdvancedMode(true);
        m_ui->advancedSettingsButton->setText(tr("Simple Settings"));
    }
}

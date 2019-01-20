/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseSettingsDialog.h"
#include "ui_DatabaseSettingsDialog.h"

#include "DatabaseSettingsWidgetEncryption.h"
#include "DatabaseSettingsWidgetGeneral.h"
#include "DatabaseSettingsWidgetMasterKey.h"
#ifdef WITH_XC_BROWSER
#include "DatabaseSettingsWidgetBrowser.h"
#endif
#if defined(WITH_XC_KEESHARE)
#include "keeshare/DatabaseSettingsPageKeeShare.h"
#endif

#include "core/Config.h"
#include "core/Database.h"
#include "core/FilePath.h"
#include "core/Global.h"
#include "touchid/TouchID.h"

class DatabaseSettingsDialog::ExtraPage
{
public:
    ExtraPage(IDatabaseSettingsPage* page, QWidget* widget)
        : settingsPage(page)
        , widget(widget)
    {
    }
    void loadSettings(QSharedPointer<Database> db) const
    {
        settingsPage->loadSettings(widget, db);
    }
    void saveSettings() const
    {
        settingsPage->saveSettings(widget);
    }

private:
    QSharedPointer<IDatabaseSettingsPage> settingsPage;
    QWidget* widget;
};

DatabaseSettingsDialog::DatabaseSettingsDialog(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsDialog())
    , m_generalWidget(new DatabaseSettingsWidgetGeneral(this))
    , m_securityTabWidget(new QTabWidget(this))
    , m_masterKeyWidget(new DatabaseSettingsWidgetMasterKey(this))
    , m_encryptionWidget(new DatabaseSettingsWidgetEncryption(this))
#ifdef WITH_XC_BROWSER
    , m_browserWidget(new DatabaseSettingsWidgetBrowser(this))
#endif
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    m_ui->categoryList->addCategory(tr("General"), FilePath::instance()->icon("categories", "preferences-other"));
    m_ui->categoryList->addCategory(tr("Security"), FilePath::instance()->icon("actions", "document-encrypt"));
    m_ui->stackedWidget->addWidget(m_generalWidget);

    m_ui->stackedWidget->addWidget(m_securityTabWidget);
    m_securityTabWidget->addTab(m_masterKeyWidget, tr("Master Key"));
    m_securityTabWidget->addTab(m_encryptionWidget, tr("Encryption Settings"));

#if defined(WITH_XC_KEESHARE)
    addSettingsPage(new DatabaseSettingsPageKeeShare());
#endif

    m_ui->stackedWidget->setCurrentIndex(0);
    m_securityTabWidget->setCurrentIndex(0);

    connect(m_securityTabWidget, SIGNAL(currentChanged(int)), SLOT(pageChanged()));
    connect(m_ui->categoryList, SIGNAL(categoryChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));
    connect(m_ui->advancedSettingsToggle, SIGNAL(toggled(bool)), SLOT(toggleAdvancedMode(bool)));

#ifdef WITH_XC_BROWSER
    m_ui->categoryList->addCategory(tr("Browser Integration"),
                                    FilePath::instance()->icon("apps", "internet-web-browser"));
    m_ui->stackedWidget->addWidget(m_browserWidget);
#endif

    pageChanged();
}

DatabaseSettingsDialog::~DatabaseSettingsDialog()
{
}

void DatabaseSettingsDialog::load(QSharedPointer<Database> db)
{
    m_ui->categoryList->setCurrentCategory(0);
    m_generalWidget->load(db);
    m_masterKeyWidget->load(db);
    m_encryptionWidget->load(db);
#ifdef WITH_XC_BROWSER
    m_browserWidget->load(db);
#endif
    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.loadSettings(db);
    }
    m_ui->advancedSettingsToggle->setChecked(config()->get("GUI/AdvancedSettings", false).toBool());
    m_db = db;
}

void DatabaseSettingsDialog::addSettingsPage(IDatabaseSettingsPage* page)
{
    const int category = m_ui->categoryList->currentCategory();
    QWidget* widget = page->createWidget();
    widget->setParent(this);
    m_extraPages.append(ExtraPage(page, widget));
    m_ui->stackedWidget->addWidget(widget);
    m_ui->categoryList->addCategory(page->name(), page->icon());
    m_ui->categoryList->setCurrentCategory(category);
}

/**
 * Show page and tab with database master key settings.
 */
void DatabaseSettingsDialog::showMasterKeySettings()
{
    m_ui->categoryList->setCurrentCategory(1);
    m_securityTabWidget->setCurrentIndex(0);
}

void DatabaseSettingsDialog::save()
{
    if (!m_generalWidget->save()) {
        return;
    }

    if (!m_masterKeyWidget->save()) {
        return;
    }

    if (!m_encryptionWidget->save()) {
        return;
    }

    for (const ExtraPage& extraPage : asConst(m_extraPages)) {
        extraPage.saveSettings();
    }

#ifdef WITH_XC_TOUCHID
    TouchID::getInstance().reset(m_db ? m_db->filePath() : "");
#endif

    emit editFinished(true);
}

void DatabaseSettingsDialog::reject()
{
    emit editFinished(false);
}

void DatabaseSettingsDialog::pageChanged()
{
    int pageIndex = m_ui->stackedWidget->currentIndex();

    bool enabled = (pageIndex == Page::General && m_generalWidget->hasAdvancedMode());

    if (Page::Security == pageIndex) {
        int tabIndex = m_securityTabWidget->currentIndex();
        enabled = (tabIndex == 0 && m_masterKeyWidget->hasAdvancedMode());
        enabled |= (tabIndex == 1 && m_encryptionWidget->hasAdvancedMode());
    }

    m_ui->advancedSettingsToggle->setEnabled(enabled);
}

void DatabaseSettingsDialog::toggleAdvancedMode(bool advanced)
{
    if (m_generalWidget->hasAdvancedMode()) {
        m_generalWidget->setAdvancedMode(advanced);
    }

    if (m_masterKeyWidget->hasAdvancedMode()) {
        m_masterKeyWidget->setAdvancedMode(advanced);
    }

    if (m_encryptionWidget->hasAdvancedMode()) {
        m_encryptionWidget->setAdvancedMode(advanced);
    }

    config()->set("GUI/AdvancedSettings", advanced);
}

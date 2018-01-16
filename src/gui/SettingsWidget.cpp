/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "SettingsWidget.h"
#include "ui_SettingsWidgetGeneral.h"
#include "ui_SettingsWidgetSecurity.h"

#include "config-keepassx.h"
#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/Translator.h"
#include "core/FilePath.h"
#include "core/Global.h"

class SettingsWidget::ExtraPage
{
    public:
        ExtraPage(ISettingsPage* page, QWidget* widget): settingsPage(page), widget(widget)
        {}

        void loadSettings() const
        {
            settingsPage->loadSettings(widget);
        }

        void saveSettings() const
        {
            settingsPage->saveSettings(widget);
        }

    private:
        QSharedPointer<ISettingsPage> settingsPage;
        QWidget* widget;
};

SettingsWidget::SettingsWidget(QWidget* parent)
    : EditWidget(parent)
    , m_secWidget(new QWidget())
    , m_generalWidget(new QWidget())
    , m_secUi(new Ui::SettingsWidgetSecurity())
    , m_generalUi(new Ui::SettingsWidgetGeneral())
    , m_globalAutoTypeKey(static_cast<Qt::Key>(0))
    , m_globalAutoTypeModifiers(Qt::NoModifier)
{
    setHeadline(tr("Application Settings"));

    m_secUi->setupUi(m_secWidget);
    m_generalUi->setupUi(m_generalWidget);
    addPage(tr("General"), FilePath::instance()->icon("categories", "preferences-other"), m_generalWidget);
    addPage(tr("Security"), FilePath::instance()->icon("status", "security-high"), m_secWidget);

    if (!autoType()->isAvailable()) {
        m_generalUi->generalSettingsTabWidget->removeTab(1);
    }

    connect(this, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(this, SIGNAL(apply()), SLOT(saveSettings()));
    connect(this, SIGNAL(rejected()), SLOT(reject()));

    connect(m_generalUi->autoSaveAfterEveryChangeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(enableAutoSaveOnExit(bool)));
    connect(m_generalUi->systrayShowCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(enableSystray(bool)));

    connect(m_secUi->clearClipboardCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearClipboardSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->lockDatabaseIdleCheckBox, SIGNAL(toggled(bool)),
            m_secUi->lockDatabaseIdleSpinBox, SLOT(setEnabled(bool)));

#ifndef WITH_XC_NETWORKING
    m_secUi->privacy->setVisible(false);
#endif
}

SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::addSettingsPage(ISettingsPage* page)
{
    QWidget* widget = page->createWidget();
    widget->setParent(this);
    m_extraPages.append(ExtraPage(page, widget));
    addPage(page->name(), page->icon(), widget);
}

void SettingsWidget::loadSettings()
{

    if (config()->hasAccessError()) {
        showMessage(
            tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
    }

#ifdef QT_DEBUG
    m_generalUi->singleInstanceCheckBox->setEnabled(false);
#endif
    m_generalUi->singleInstanceCheckBox->setChecked(config()->get("SingleInstance").toBool());
    m_generalUi->rememberLastDatabasesCheckBox->setChecked(config()->get("RememberLastDatabases").toBool());
    m_generalUi->rememberLastKeyFilesCheckBox->setChecked(config()->get("RememberLastKeyFiles").toBool());
    m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(
        config()->get("OpenPreviousDatabasesOnStartup").toBool());
    m_generalUi->autoSaveAfterEveryChangeCheckBox->setChecked(config()->get("AutoSaveAfterEveryChange").toBool());
    m_generalUi->autoSaveOnExitCheckBox->setChecked(config()->get("AutoSaveOnExit").toBool());
    m_generalUi->autoReloadOnChangeCheckBox->setChecked(config()->get("AutoReloadOnChange").toBool());
    m_generalUi->minimizeOnCopyCheckBox->setChecked(config()->get("MinimizeOnCopy").toBool());
    m_generalUi->useGroupIconOnEntryCreationCheckBox->setChecked(config()->get("UseGroupIconOnEntryCreation").toBool());
    m_generalUi->autoTypeEntryTitleMatchCheckBox->setChecked(config()->get("AutoTypeEntryTitleMatch").toBool());
    m_generalUi->autoTypeEntryURLMatchCheckBox->setChecked(config()->get("AutoTypeEntryURLMatch").toBool());
    m_generalUi->ignoreGroupExpansionCheckBox->setChecked(config()->get("IgnoreGroupExpansion").toBool());

    m_generalUi->languageComboBox->clear();
    QList<QPair<QString, QString> > languages = Translator::availableLanguages();
    for (int i = 0; i < languages.size(); i++) {
        m_generalUi->languageComboBox->addItem(languages[i].second, languages[i].first);
    }
    int defaultIndex = m_generalUi->languageComboBox->findData(config()->get("GUI/Language"));
    if (defaultIndex > 0) {
        m_generalUi->languageComboBox->setCurrentIndex(defaultIndex);
    }

    m_generalUi->detailsHideCheckBox->setChecked(config()->get("GUI/HideDetailsView").toBool());
    m_generalUi->systrayShowCheckBox->setChecked(config()->get("GUI/ShowTrayIcon").toBool());
    m_generalUi->systrayDarkIconCheckBox->setChecked(config()->get("GUI/DarkTrayIcon").toBool());
    m_generalUi->systrayMinimizeToTrayCheckBox->setChecked(config()->get("GUI/MinimizeToTray").toBool());
    m_generalUi->systrayMinimizeOnCloseCheckBox->setChecked(config()->get("GUI/MinimizeOnClose").toBool());
    m_generalUi->systrayMinimizeOnStartup->setChecked(config()->get("GUI/MinimizeOnStartup").toBool());
    m_generalUi->autoTypeAskCheckBox->setChecked(config()->get("security/autotypeask").toBool());

    if (autoType()->isAvailable()) {
        m_globalAutoTypeKey = static_cast<Qt::Key>(config()->get("GlobalAutoTypeKey").toInt());
        m_globalAutoTypeModifiers = static_cast<Qt::KeyboardModifiers>(config()->get("GlobalAutoTypeModifiers").toInt());
        if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
            m_generalUi->autoTypeShortcutWidget->setShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
        }
        m_generalUi->autoTypeDelaySpinBox->setValue(config()->get("AutoTypeDelay").toInt());
    }


    m_secUi->clearClipboardCheckBox->setChecked(config()->get("security/clearclipboard").toBool());
    m_secUi->clearClipboardSpinBox->setValue(config()->get("security/clearclipboardtimeout").toInt());

    m_secUi->lockDatabaseIdleCheckBox->setChecked(config()->get("security/lockdatabaseidle").toBool());
    m_secUi->lockDatabaseIdleSpinBox->setValue(config()->get("security/lockdatabaseidlesec").toInt());
    m_secUi->lockDatabaseMinimizeCheckBox->setChecked(config()->get("security/lockdatabaseminimize").toBool());
    m_secUi->lockDatabaseOnScreenLockCheckBox->setChecked(config()->get("security/lockdatabasescreenlock").toBool());
    m_secUi->fallbackToGoogle->setChecked(config()->get("security/IconDownloadFallbackToGoogle").toBool());

    m_secUi->passwordCleartextCheckBox->setChecked(config()->get("security/passwordscleartext").toBool());
    m_secUi->passwordDetailsCleartextCheckBox->setChecked(config()->get("security/hidepassworddetails").toBool());
    m_secUi->passwordRepeatCheckBox->setChecked(config()->get("security/passwordsrepeat").toBool());
    m_secUi->hideNotesCheckBox->setChecked(config()->get("security/hidenotes").toBool());


    for (const ExtraPage& page: asConst(m_extraPages)) {
        page.loadSettings();
    }

    setCurrentPage(0);
}

void SettingsWidget::saveSettings()
{

    if (config()->hasAccessError()) {
        showMessage(
            tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
        // We prevent closing the settings page if we could not write to
        // the config file.
        return;
    }

    config()->set("SingleInstance", m_generalUi->singleInstanceCheckBox->isChecked());
    config()->set("RememberLastDatabases", m_generalUi->rememberLastDatabasesCheckBox->isChecked());
    config()->set("RememberLastKeyFiles", m_generalUi->rememberLastKeyFilesCheckBox->isChecked());
    config()->set("OpenPreviousDatabasesOnStartup",
                  m_generalUi->openPreviousDatabasesOnStartupCheckBox->isChecked());
    config()->set("AutoSaveAfterEveryChange",
                  m_generalUi->autoSaveAfterEveryChangeCheckBox->isChecked());
    config()->set("AutoSaveOnExit", m_generalUi->autoSaveOnExitCheckBox->isChecked());
    config()->set("AutoReloadOnChange", m_generalUi->autoReloadOnChangeCheckBox->isChecked());
    config()->set("MinimizeOnCopy", m_generalUi->minimizeOnCopyCheckBox->isChecked());
    config()->set("UseGroupIconOnEntryCreation",
                  m_generalUi->useGroupIconOnEntryCreationCheckBox->isChecked());
    config()->set("IgnoreGroupExpansion",
                  m_generalUi->ignoreGroupExpansionCheckBox->isChecked());
    config()->set("AutoTypeEntryTitleMatch",
                  m_generalUi->autoTypeEntryTitleMatchCheckBox->isChecked());
    config()->set("AutoTypeEntryURLMatch",
                  m_generalUi->autoTypeEntryURLMatchCheckBox->isChecked());
    int currentLangIndex = m_generalUi->languageComboBox->currentIndex();

    config()->set("GUI/Language", m_generalUi->languageComboBox->itemData(currentLangIndex).toString());

    config()->set("GUI/HideDetailsView", m_generalUi->detailsHideCheckBox->isChecked());
    config()->set("GUI/ShowTrayIcon", m_generalUi->systrayShowCheckBox->isChecked());
    config()->set("GUI/DarkTrayIcon", m_generalUi->systrayDarkIconCheckBox->isChecked());
    config()->set("GUI/MinimizeToTray", m_generalUi->systrayMinimizeToTrayCheckBox->isChecked());
    config()->set("GUI/MinimizeOnClose", m_generalUi->systrayMinimizeOnCloseCheckBox->isChecked());
    config()->set("GUI/MinimizeOnStartup", m_generalUi->systrayMinimizeOnStartup->isChecked());

    config()->set("security/autotypeask", m_generalUi->autoTypeAskCheckBox->isChecked());

    if (autoType()->isAvailable()) {
        config()->set("GlobalAutoTypeKey", m_generalUi->autoTypeShortcutWidget->key());
        config()->set("GlobalAutoTypeModifiers",
                      static_cast<int>(m_generalUi->autoTypeShortcutWidget->modifiers()));
        config()->set("AutoTypeDelay", m_generalUi->autoTypeDelaySpinBox->value());
    }
    config()->set("security/clearclipboard", m_secUi->clearClipboardCheckBox->isChecked());
    config()->set("security/clearclipboardtimeout", m_secUi->clearClipboardSpinBox->value());

    config()->set("security/lockdatabaseidle", m_secUi->lockDatabaseIdleCheckBox->isChecked());
    config()->set("security/lockdatabaseidlesec", m_secUi->lockDatabaseIdleSpinBox->value());
    config()->set("security/lockdatabaseminimize", m_secUi->lockDatabaseMinimizeCheckBox->isChecked());
    config()->set("security/lockdatabasescreenlock", m_secUi->lockDatabaseOnScreenLockCheckBox->isChecked());
    config()->set("security/IconDownloadFallbackToGoogle", m_secUi->fallbackToGoogle->isChecked());

    config()->set("security/passwordscleartext", m_secUi->passwordCleartextCheckBox->isChecked());
    config()->set("security/hidepassworddetails", m_secUi->passwordDetailsCleartextCheckBox->isChecked());
    config()->set("security/passwordsrepeat", m_secUi->passwordRepeatCheckBox->isChecked());
    config()->set("security/hidenotes", m_secUi->hideNotesCheckBox->isChecked());

    // Security: clear storage if related settings are disabled
    if (!config()->get("RememberLastDatabases").toBool()) {
        config()->set("LastDatabases", QVariant());
    }

    if (!config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastKeyFiles", QVariant());
        config()->set("LastDir", "");
    }

    for (const ExtraPage& page: asConst(m_extraPages)) {
        page.saveSettings();
    }
}

void SettingsWidget::reject()
{
    // register the old key again as it might have changed
    if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
        autoType()->registerGlobalShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
    }

}

void SettingsWidget::enableAutoSaveOnExit(bool checked)
{
    m_generalUi->autoSaveOnExitCheckBox->setEnabled(!checked);
}

void SettingsWidget::enableSystray(bool checked)
{
    m_generalUi->systrayDarkIconCheckBox->setEnabled(checked);
    m_generalUi->systrayMinimizeToTrayCheckBox->setEnabled(checked);
    m_generalUi->systrayMinimizeOnCloseCheckBox->setEnabled(checked);
}

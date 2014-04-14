/*
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

#include "SettingsWidget.h"
#include "ui_SettingsWidgetGeneral.h"
#include "ui_SettingsWidgetSecurity.h"

#include "autotype/AutoType.h"
#include "core/Config.h"

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
    add(tr("General"), m_generalWidget);
    add(tr("Security"), m_secWidget);

    m_generalUi->autoTypeShortcutWidget->setVisible(autoType()->isAvailable());
    m_generalUi->autoTypeShortcutLabel->setVisible(autoType()->isAvailable());

    connect(this, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(this, SIGNAL(rejected()), SLOT(reject()));

    connect(m_generalUi->autoSaveAfterEveryChangeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(enableAutoSaveOnExit(bool)));

    connect(m_secUi->clearClipboardCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearClipboardSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->lockDatabaseIdleCheckBox, SIGNAL(toggled(bool)),
            m_secUi->lockDatabaseIdleSpinBox, SLOT(setEnabled(bool)));
}

SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::loadSettings()
{
    m_generalUi->rememberLastDatabasesCheckBox->setChecked(config()->get("RememberLastDatabases").toBool());
    m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(
        config()->get("OpenPreviousDatabasesOnStartup").toBool());
    m_generalUi->modifiedExpandedChangedCheckBox->setChecked(config()->get("ModifiedOnExpandedStateChanges").toBool());
    m_generalUi->autoSaveAfterEveryChangeCheckBox->setChecked(config()->get("AutoSaveAfterEveryChange").toBool());
    m_generalUi->autoSaveOnExitCheckBox->setChecked(config()->get("AutoSaveOnExit").toBool());
    m_generalUi->minimizeOnCopyCheckBox->setChecked(config()->get("MinimizeOnCopy").toBool());
    m_generalUi->useGroupIconOnEntryCreationCheckBox->setChecked(config()->get("UseGroupIconOnEntryCreation").toBool());

    if (autoType()->isAvailable()) {
        m_globalAutoTypeKey = static_cast<Qt::Key>(config()->get("GlobalAutoTypeKey").toInt());
        m_globalAutoTypeModifiers = static_cast<Qt::KeyboardModifiers>(config()->get("GlobalAutoTypeModifiers").toInt());
        if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
            m_generalUi->autoTypeShortcutWidget->setShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
        }
    }

    m_secUi->clearClipboardCheckBox->setChecked(config()->get("security/clearclipboard").toBool());
    m_secUi->clearClipboardSpinBox->setValue(config()->get("security/clearclipboardtimeout").toInt());

    m_secUi->lockDatabaseIdleCheckBox->setChecked(config()->get("security/lockdatabaseidle").toBool());
    m_secUi->lockDatabaseIdleSpinBox->setValue(config()->get("security/lockdatabaseidlesec").toInt());

    m_secUi->passwordCleartextCheckBox->setChecked(config()->get("security/passwordscleartext").toBool());

    m_secUi->autoTypeAskCheckBox->setChecked(config()->get("security/autotypeask").toBool());

    setCurrentRow(0);
}

void SettingsWidget::saveSettings()
{
    config()->set("RememberLastDatabases", m_generalUi->rememberLastDatabasesCheckBox->isChecked());
    config()->set("OpenPreviousDatabasesOnStartup",
                  m_generalUi->openPreviousDatabasesOnStartupCheckBox->isChecked());
    config()->set("ModifiedOnExpandedStateChanges",
                  m_generalUi->modifiedExpandedChangedCheckBox->isChecked());
    config()->set("AutoSaveAfterEveryChange",
                  m_generalUi->autoSaveAfterEveryChangeCheckBox->isChecked());
    config()->set("AutoSaveOnExit", m_generalUi->autoSaveOnExitCheckBox->isChecked());
    config()->set("MinimizeOnCopy", m_generalUi->minimizeOnCopyCheckBox->isChecked());
    config()->set("UseGroupIconOnEntryCreation",
                  m_generalUi->useGroupIconOnEntryCreationCheckBox->isChecked());
    if (autoType()->isAvailable()) {
        config()->set("GlobalAutoTypeKey", m_generalUi->autoTypeShortcutWidget->key());
        config()->set("GlobalAutoTypeModifiers",
                      static_cast<int>(m_generalUi->autoTypeShortcutWidget->modifiers()));
    }
    config()->set("security/clearclipboard", m_secUi->clearClipboardCheckBox->isChecked());
    config()->set("security/clearclipboardtimeout", m_secUi->clearClipboardSpinBox->value());

    config()->set("security/lockdatabaseidle", m_secUi->lockDatabaseIdleCheckBox->isChecked());
    config()->set("security/lockdatabaseidlesec", m_secUi->lockDatabaseIdleSpinBox->value());

    config()->set("security/passwordscleartext", m_secUi->passwordCleartextCheckBox->isChecked());

    config()->set("security/autotypeask", m_secUi->autoTypeAskCheckBox->isChecked());

    Q_EMIT editFinished(true);
}

void SettingsWidget::reject()
{
    // register the old key again as it might have changed
    if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
        autoType()->registerGlobalShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
    }

    Q_EMIT editFinished(false);
}

void SettingsWidget::enableAutoSaveOnExit(bool checked)
{
    m_generalUi->autoSaveOnExitCheckBox->setEnabled(!checked);
}

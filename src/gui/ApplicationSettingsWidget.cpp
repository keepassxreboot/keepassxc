/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "ApplicationSettingsWidget.h"
#include "ui_ApplicationSettingsWidgetGeneral.h"
#include "ui_ApplicationSettingsWidgetSecurity.h"
#include <QDesktopServices>
#include <QDir>
#include <QToolTip>

#include "config-keepassx.h"

#include "autotype/AutoType.h"
#include "core/Translator.h"
#include "gui/Icons.h"
#include "gui/MainWindow.h"
#include "gui/osutils/OSUtils.h"
#include "quickunlock/QuickUnlockInterface.h"

#include "FileDialog.h"
#include "MessageBox.h"
#ifdef WITH_XC_BROWSER
#include "browser/BrowserSettingsPage.h"
#endif

class ApplicationSettingsWidget::ExtraPage
{
public:
    ExtraPage(ISettingsPage* page, QWidget* widget)
        : settingsPage(page)
        , widget(widget)
    {
    }

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

/**
 * Helper class to ignore mouse wheel events on non-focused widgets
 * NOTE: The widget must NOT have a focus policy of "WHEEL"
 */
class MouseWheelEventFilter : public QObject
{
public:
    explicit MouseWheelEventFilter(QObject* parent)
        : QObject(parent){};

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        const auto* widget = qobject_cast<QWidget*>(obj);
        if (event->type() == QEvent::Wheel && widget && !widget->hasFocus()) {
            event->ignore();
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

ApplicationSettingsWidget::ApplicationSettingsWidget(QWidget* parent)
    : EditWidget(parent)
    , m_secWidget(new QWidget())
    , m_generalWidget(new QWidget())
    , m_secUi(new Ui::ApplicationSettingsWidgetSecurity())
    , m_generalUi(new Ui::ApplicationSettingsWidgetGeneral())
    , m_globalAutoTypeKey(static_cast<Qt::Key>(0))
    , m_globalAutoTypeModifiers(Qt::NoModifier)
{
    setHeadline(tr("Application Settings"));
    showApplyButton(false);

    m_secUi->setupUi(m_secWidget);
    m_generalUi->setupUi(m_generalWidget);
    addPage(tr("General"), icons()->icon("preferences-other"), m_generalWidget);
    addPage(tr("Security"), icons()->icon("security-high"), m_secWidget);

    // Map configuration keys to their respective widgets
    m_configWidgetMap = {
        {Config::SingleInstance, m_generalUi->singleInstanceCheckBox},
        {Config::RememberLastDatabases, m_generalUi->rememberLastDatabasesCheckBox},
        {Config::NumberOfRememberedLastDatabases, m_generalUi->rememberLastDatabasesSpinbox},
        {Config::RememberLastKeyFiles, m_generalUi->rememberLastKeyFilesCheckBox},
        {Config::OpenPreviousDatabasesOnStartup, m_generalUi->openPreviousDatabasesOnStartupCheckBox},
        {Config::AutoSaveAfterEveryChange, m_generalUi->autoSaveAfterEveryChangeCheckBox},
        {Config::AutoSaveOnExit, m_generalUi->autoSaveOnExitCheckBox},
        {Config::AutoSaveNonDataChanges, m_generalUi->autoSaveNonDataChangesCheckBox},
        {Config::BackupBeforeSave, m_generalUi->backupBeforeSaveCheckBox},
        {Config::BackupFilePathPattern, m_generalUi->backupFilePath},
        {Config::UseAtomicSaves, m_generalUi->useAlternativeSaveCheckBox},
        {Config::UseDirectWriteSaves, m_generalUi->alternativeSaveComboBox},
        {Config::AutoReloadOnChange, m_generalUi->autoReloadOnChangeCheckBox},
        {Config::MinimizeAfterUnlock, m_generalUi->minimizeAfterUnlockCheckBox},
        {Config::MinimizeOnOpenUrl, m_generalUi->minimizeOnOpenUrlCheckBox},
        {Config::HideWindowOnCopy, m_generalUi->hideWindowOnCopyCheckBox},
        {Config::MinimizeOnCopy, m_generalUi->minimizeOnCopyRadioButton},
        {Config::DropToBackgroundOnCopy, m_generalUi->dropToBackgroundOnCopyRadioButton},
        {Config::UseGroupIconOnEntryCreation, m_generalUi->useGroupIconOnEntryCreationCheckBox},
        {Config::AutoTypeEntryTitleMatch, m_generalUi->autoTypeEntryTitleMatchCheckBox},
        {Config::AutoTypeEntryURLMatch, m_generalUi->autoTypeEntryURLMatchCheckBox},
        {Config::AutoTypeHideExpiredEntry, m_generalUi->autoTypeHideExpiredEntryCheckBox},
        {Config::FaviconDownloadTimeout, m_generalUi->faviconTimeoutSpinBox},
        {Config::GUI_MovableToolbar, m_generalUi->toolbarMovableCheckBox},
        {Config::GUI_MonospaceNotes, m_generalUi->monospaceNotesCheckBox},
        {Config::GUI_ColorPasswords, m_generalUi->colorPasswordsCheckBox},
        {Config::GUI_ShowTrayIcon, m_generalUi->systrayShowCheckBox},
        {Config::GUI_MinimizeToTray, m_generalUi->systrayMinimizeToTrayCheckBox},
        {Config::GUI_MinimizeOnClose, m_generalUi->minimizeOnCloseCheckBox},
        {Config::GUI_MinimizeOnStartup, m_generalUi->systrayMinimizeOnStartup},
        {Config::GUI_CheckForUpdates, m_generalUi->checkForUpdatesOnStartupCheckBox},
        {Config::GUI_CheckForUpdatesIncludeBetas, m_generalUi->checkForUpdatesIncludeBetasCheckBox},
        {Config::GUI_ShowExpiredEntriesOnDatabaseUnlock, m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox},
        {Config::GUI_ShowExpiredEntriesOnDatabaseUnlockOffsetDays,
         m_generalUi->showExpiredEntriesOnDatabaseUnlockOffsetSpinBox},
        {Config::Security_AutoTypeAsk, m_generalUi->autoTypeAskCheckBox},
        {Config::Security_RelockAutoType, m_generalUi->autoTypeRelockDatabaseCheckBox},
        {Config::Security_ClearClipboard, m_secUi->clearClipboardCheckBox},
        {Config::Security_ClearClipboardTimeout, m_secUi->clearClipboardSpinBox},
        {Config::Security_ClearSearch, m_secUi->clearSearchCheckBox},
        {Config::Security_ClearSearchTimeout, m_secUi->clearSearchSpinBox},
        {Config::Security_LockDatabaseIdle, m_secUi->lockDatabaseIdleCheckBox},
        {Config::Security_LockDatabaseIdleSeconds, m_secUi->lockDatabaseIdleSpinBox},
        {Config::Security_LockDatabaseMinimize, m_secUi->lockDatabaseMinimizeCheckBox},
        {Config::Security_LockDatabaseScreenLock, m_secUi->lockDatabaseOnScreenLockCheckBox},
        {Config::Security_LockDatabaseOnUserSwitch, m_secUi->lockDatabasesOnUserSwitchCheckBox},
        {Config::Security_IconDownloadFallback, m_secUi->fallbackToSearch},
        {Config::Security_PasswordsHidden, m_secUi->passwordsHiddenCheckBox},
        {Config::Security_PasswordEmptyPlaceholder, m_secUi->passwordShowDotsCheckBox},
        {Config::Security_HidePasswordPreviewPanel, m_secUi->passwordPreviewCleartextCheckBox},
        {Config::Security_HideTotpPreviewPanel, m_secUi->hideTotpCheckBox},
        {Config::Security_HideNotes, m_secUi->hideNotesCheckBox},
        {Config::Security_NoConfirmMoveEntryToRecycleBin, m_secUi->NoConfirmMoveEntryToRecycleBinCheckBox},
        {Config::Security_EnableCopyOnDoubleClick, m_secUi->EnableCopyOnDoubleClickCheckBox},
        {Config::Security_QuickUnlock, m_secUi->quickUnlockCheckBox},

        // Custom logic
        {Config::GUI_ToolButtonStyle, m_generalUi->toolButtonStyleComboBox},
        {Config::GUI_Language, m_generalUi->languageComboBox},
        {Config::GUI_TrayIconAppearance, m_generalUi->trayIconAppearance}};

#ifdef WITH_XC_BROWSER
    addSettingsPage(new BrowserSettingsPage());
#endif

    if (!autoType()->isAvailable()) {
        m_generalUi->generalSettingsTabWidget->removeTab(1);
    }

    connect(this, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(this, SIGNAL(rejected()), SLOT(reject()));

    // clang-format off
    connect(m_generalUi->autoSaveAfterEveryChangeCheckBox, SIGNAL(toggled(bool)), SLOT(autoSaveToggled(bool)));
    connect(m_generalUi->hideWindowOnCopyCheckBox, SIGNAL(toggled(bool)), SLOT(hideWindowOnCopyCheckBoxToggled(bool)));
    connect(m_generalUi->systrayShowCheckBox, SIGNAL(toggled(bool)), SLOT(systrayToggled(bool)));
    connect(m_generalUi->rememberLastDatabasesCheckBox, SIGNAL(toggled(bool)), SLOT(rememberDatabasesToggled(bool)));
    connect(m_generalUi->resetSettingsButton, SIGNAL(clicked()), SLOT(resetSettings()));
    connect(m_generalUi->useAlternativeSaveCheckBox, SIGNAL(toggled(bool)),
            m_generalUi->alternativeSaveComboBox, SLOT(setEnabled(bool)));

    connect(m_generalUi->backupBeforeSaveCheckBox, SIGNAL(toggled(bool)),
            m_generalUi->backupFilePath, SLOT(setEnabled(bool)));
    connect(m_generalUi->backupBeforeSaveCheckBox, SIGNAL(toggled(bool)),
            m_generalUi->backupFilePathPicker, SLOT(setEnabled(bool)));
    connect(m_generalUi->backupFilePathPicker, SIGNAL(pressed()), SLOT(selectBackupDirectory()));
    connect(m_generalUi->showExpiredEntriesOnDatabaseUnlockCheckBox, SIGNAL(toggled(bool)),
            SLOT(showExpiredEntriesOnDatabaseUnlockToggled(bool)));

    connect(m_secUi->clearClipboardCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearClipboardSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->clearSearchCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearSearchSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->lockDatabaseIdleCheckBox, SIGNAL(toggled(bool)),
            m_secUi->lockDatabaseIdleSpinBox, SLOT(setEnabled(bool)));
    // clang-format on

    connect(m_generalUi->minimizeAfterUnlockCheckBox, &QCheckBox::toggled, this, [this](bool state) {
        if (state) {
            m_secUi->lockDatabaseMinimizeCheckBox->setChecked(false);
        }
        m_secUi->lockDatabaseMinimizeCheckBox->setToolTip(
            state ? tr("This setting cannot be enabled when minimize on unlock is enabled.") : "");
        m_secUi->lockDatabaseMinimizeCheckBox->setEnabled(!state);
    });

    // Set Auto-Type shortcut when changed
    connect(
        m_generalUi->autoTypeShortcutWidget, &ShortcutWidget::shortcutChanged, this, [this](auto key, auto modifiers) {
            QString error;
            if (autoType()->registerGlobalShortcut(key, modifiers, &error)) {
                m_generalUi->autoTypeShortcutWidget->setStyleSheet("");
            } else {
                QToolTip::showText(mapToGlobal(rect().bottomLeft()), error);
                m_generalUi->autoTypeShortcutWidget->setStyleSheet("background-color: #FF9696;");
            }
        });
    connect(m_generalUi->autoTypeShortcutWidget, &ShortcutWidget::shortcutReset, this, [this] {
        autoType()->unregisterGlobalShortcut();
        m_generalUi->autoTypeShortcutWidget->setStyleSheet("");
    });

    // Disable mouse wheel grab when scrolling
    // This prevents combo box and spinner values from changing without explicit focus
    auto mouseWheelFilter = new MouseWheelEventFilter(this);
    m_generalUi->faviconTimeoutSpinBox->installEventFilter(mouseWheelFilter);
    m_generalUi->toolButtonStyleComboBox->installEventFilter(mouseWheelFilter);
    m_generalUi->languageComboBox->installEventFilter(mouseWheelFilter);
    m_generalUi->trayIconAppearance->installEventFilter(mouseWheelFilter);

#ifdef WITH_XC_UPDATECHECK
    connect(m_generalUi->checkForUpdatesOnStartupCheckBox, SIGNAL(toggled(bool)), SLOT(checkUpdatesToggled(bool)));
#else
    m_generalUi->checkForUpdatesOnStartupCheckBox->setVisible(false);
    m_generalUi->checkForUpdatesIncludeBetasCheckBox->setVisible(false);
    m_generalUi->checkUpdatesSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
#endif

#ifndef WITH_XC_NETWORKING
    m_secUi->privacy->setVisible(false);
    m_generalUi->faviconTimeoutLabel->setVisible(false);
    m_generalUi->faviconTimeoutSpinBox->setVisible(false);
#endif
}

ApplicationSettingsWidget::~ApplicationSettingsWidget() = default;

void ApplicationSettingsWidget::addSettingsPage(ISettingsPage* page)
{
    QWidget* widget = page->createWidget();
    widget->setParent(this);
    m_extraPages.append(ExtraPage(page, widget));
    addPage(page->name(), page->icon(), widget);
}

void ApplicationSettingsWidget::loadSettings()
{
    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
    }

#ifdef QT_DEBUG
    m_generalUi->singleInstanceCheckBox->setEnabled(false);
    m_generalUi->launchAtStartup->setEnabled(false);
#endif

    // Set the values for each config
    for (auto it = m_configWidgetMap.cbegin(); it != m_configWidgetMap.cend(); ++it) {
        Config::ConfigKey configKey = it.key();
        QWidget* widget = it.value();
        QVariant value = config()->get(configKey);

        // Check if the config requires a custom logic for drawing the related widgets
        switch (configKey) {
        case Config::GUI_ToolButtonStyle: {
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
            comboBox->clear();
            comboBox->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
            comboBox->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
            comboBox->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
            comboBox->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
            comboBox->addItem(tr("Follow style"), Qt::ToolButtonFollowStyle);
            int toolButtonStyleIndex = comboBox->findData(value);
            if (toolButtonStyleIndex > 0) {
                comboBox->setCurrentIndex(toolButtonStyleIndex);
            }
            break;
        }
        case Config::GUI_Language: {
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
            comboBox->clear();
            QList<QPair<QString, QString>> languages = Translator::availableLanguages();
            for (const auto& language : languages) {
                comboBox->addItem(language.second, language.first);
            }
            int defaultIndex = comboBox->findData(value);
            if (defaultIndex > 0) {
                comboBox->setCurrentIndex(defaultIndex);
            }
            break;
        }
        case Config::GUI_TrayIconAppearance: {
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
            comboBox->clear();
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
            comboBox->addItem(tr("Monochrome"), "monochrome");
#else
            comboBox->addItem(tr("Monochrome (light)"), "monochrome-light");
            comboBox->addItem(tr("Monochrome (dark)"), "monochrome-dark");
#endif
            comboBox->addItem(tr("Colorful"), "colorful");
            int trayIconIndex = comboBox->findData(icons()->trayIconAppearance());
            if (trayIconIndex > 0) {
                comboBox->setCurrentIndex(trayIconIndex);
            }
            break;
        }
        default: {
            if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
                checkBox->setChecked(value.toBool());
            } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
                spinBox->setValue(value.toInt());
            } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
                comboBox->setCurrentIndex(value.toInt());
            } else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
                lineEdit->setText(value.toString());
            }
        }
        }

        if (config()->isManaged(configKey)) {
            widget->setEnabled(false);
            widget->setToolTip("This setting is managed by your organization.");
        }
    }

    // Custom handling for auto-type related settings
    if (autoType()->isAvailable()) {
        m_globalAutoTypeKey = static_cast<Qt::Key>(config()->get(Config::GlobalAutoTypeKey).toInt());
        m_globalAutoTypeModifiers =
            static_cast<Qt::KeyboardModifiers>(config()->get(Config::GlobalAutoTypeModifiers).toInt());
        if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
            m_generalUi->autoTypeShortcutWidget->setShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
        }
        m_generalUi->autoTypeRetypeTimeSpinBox->setValue(config()->get(Config::GlobalAutoTypeRetypeTime).toInt());
        m_generalUi->autoTypeShortcutWidget->setAttribute(Qt::WA_MacShowFocusRect, true);
        m_generalUi->autoTypeDelaySpinBox->setValue(config()->get(Config::AutoTypeDelay).toInt());
        m_generalUi->autoTypeStartDelaySpinBox->setValue(config()->get(Config::AutoTypeStartDelay).toInt());
    }

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.loadSettings();
    }

    setCurrentPage(0);
}

void ApplicationSettingsWidget::saveSettings()
{
    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
        return; // Prevent closing the settings page if we could not write to the config file.
    }

#ifndef QT_DEBUG
    osUtils->setLaunchAtStartup(m_generalUi->launchAtStartup->isChecked());
#endif

    // Iterate over the config-to-widget map
    for (auto it = m_configWidgetMap.cbegin(); it != m_configWidgetMap.cend(); ++it) {
        Config::ConfigKey configKey = it.key();
        QWidget* widget = it.value();

        // Custom logic for specific configurations
        switch (configKey) {
        case Config::GUI_Language: {
            auto language = qobject_cast<QComboBox*>(widget)->currentData().toString();
            if (config()->get(Config::GUI_Language) != language) {
                QTimer::singleShot(200, [] {
                    getMainWindow()->restartApp(
                        tr("You must restart the application to set the new language. Would you like to restart now?"));
                });
            }
            config()->set(configKey, language);
            break;
        }
        case Config::GUI_ToolButtonStyle:
        case Config::GUI_TrayIconAppearance: {
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
            config()->set(configKey, comboBox->currentData().toString());
            break;
        }
        default: {
            if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
                config()->set(configKey, checkBox->isChecked());
            } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
                config()->set(configKey, spinBox->value());
            } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
                config()->set(configKey, comboBox->currentIndex());
            } else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
                config()->set(configKey, lineEdit->text());
            }
            break;
        }
        }
    }

    // Handle auto-type settings
    if (autoType()->isAvailable()) {
        config()->set(Config::GlobalAutoTypeKey, m_generalUi->autoTypeShortcutWidget->key());
        config()->set(Config::GlobalAutoTypeModifiers,
                      static_cast<int>(m_generalUi->autoTypeShortcutWidget->modifiers()));
        config()->set(Config::GlobalAutoTypeRetypeTime, m_generalUi->autoTypeRetypeTimeSpinBox->value());
        config()->set(Config::AutoTypeDelay, m_generalUi->autoTypeDelaySpinBox->value());
        config()->set(Config::AutoTypeStartDelay, m_generalUi->autoTypeStartDelaySpinBox->value());
    }

    // Security: clear storage if related settings are disabled
    if (!config()->get(Config::RememberLastDatabases).toBool()) {
        config()->remove(Config::LastDir);
        config()->remove(Config::LastDatabases);
        config()->remove(Config::LastActiveDatabase);
    }

    if (!config()->get(Config::RememberLastKeyFiles).toBool()) {
        config()->remove(Config::LastDir);
        config()->remove(Config::LastKeyFiles);
        config()->remove(Config::LastChallengeResponse);
    }

    // Save settings for additional pages
    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.saveSettings();
    }
}

void ApplicationSettingsWidget::resetSettings()
{
    // Confirm reset
    auto ans = MessageBox::question(this,
                                    tr("Reset Settings?"),
                                    tr("Are you sure you want to reset all general and security settings to default?"),
                                    MessageBox::Reset | MessageBox::Cancel,
                                    MessageBox::Cancel);
    if (ans == MessageBox::Cancel) {
        return;
    }

    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
        // We prevent closing the settings page if we could not write to
        // the config file.
        return;
    }

    // Reset general and security settings to default
    config()->resetToDefaults();

    // Clear recently used data
    config()->remove(Config::LastDatabases);
    config()->remove(Config::LastActiveDatabase);
    config()->remove(Config::LastKeyFiles);
    config()->remove(Config::LastDir);

    // Save the Extra Pages (these are NOT reset)
    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.saveSettings();
    }

    config()->sync();

    // Refresh the settings widget and notify listeners
    loadSettings();
    emit settingsReset();
}

void ApplicationSettingsWidget::reject()
{
    // register the old key again as it might have changed
    if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
        autoType()->registerGlobalShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
    }
}

void ApplicationSettingsWidget::autoSaveToggled(bool checked)
{
    // Explicitly enable other auto-save options
    if (checked) {
        m_generalUi->autoSaveOnExitCheckBox->setChecked(true);
        m_generalUi->autoSaveNonDataChangesCheckBox->setChecked(true);
    }
    m_generalUi->autoSaveOnExitCheckBox->setEnabled(!checked);
    m_generalUi->autoSaveNonDataChangesCheckBox->setEnabled(!checked);
}

void ApplicationSettingsWidget::hideWindowOnCopyCheckBoxToggled(bool checked)
{
    m_generalUi->minimizeOnCopyRadioButton->setEnabled(checked);
    m_generalUi->dropToBackgroundOnCopyRadioButton->setEnabled(checked);
}

void ApplicationSettingsWidget::systrayToggled(bool checked)
{
    m_generalUi->trayIconAppearance->setEnabled(checked);
    m_generalUi->trayIconAppearanceLabel->setEnabled(checked);
    m_generalUi->systrayMinimizeToTrayCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::rememberDatabasesToggled(bool checked)
{
    if (!checked) {
        m_generalUi->rememberLastKeyFilesCheckBox->setChecked(false);
        m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(false);
    }

    m_generalUi->rememberLastDatabasesSpinbox->setEnabled(checked);
    m_generalUi->rememberLastKeyFilesCheckBox->setEnabled(checked);
    m_generalUi->openPreviousDatabasesOnStartupCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::checkUpdatesToggled(bool checked)
{
    m_generalUi->checkForUpdatesIncludeBetasCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::showExpiredEntriesOnDatabaseUnlockToggled(bool checked)
{
    m_generalUi->showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setEnabled(checked);
}

void ApplicationSettingsWidget::selectBackupDirectory()
{
    auto backupDirectory =
        FileDialog::instance()->getExistingDirectory(this, tr("Select backup storage directory"), QDir::homePath());
    if (!backupDirectory.isEmpty()) {
        m_generalUi->backupFilePath->setText(
            QDir(backupDirectory).filePath(config()->getDefault(Config::BackupFilePathPattern).toString()));
    }
}
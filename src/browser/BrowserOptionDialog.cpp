/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BrowserOptionDialog.h"
#include "ui_BrowserOptionDialog.h"

#include "BrowserSettings.h"
#include "config-keepassx.h"
#include "core/FilePath.h"

#include <QFileDialog>

BrowserOptionDialog::BrowserOptionDialog(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::BrowserOptionDialog())
{
    m_ui->setupUi(this);

    // clang-format off
    QString snapInstructions;
#if defined(KEEPASSXC_DIST_SNAP)
    snapInstructions = "<br /><br />" +
            tr("Due to Snap sandboxing, you must run a script to enable browser integration."
               "<br />"
               "You can obtain this script from %1")
               .arg("<a href=\"https://keepassxc.org/download#linux\">https://keepassxc.org</a>");
#endif

    m_ui->extensionLabel->setOpenExternalLinks(true);
    m_ui->extensionLabel->setText(
        tr("KeePassXC-Browser is needed for the browser integration to work. <br />Download it for %1 and %2. %3")
            .arg("<a href=\"https://addons.mozilla.org/en-US/firefox/addon/keepassxc-browser/\">Firefox</a>",
                 "<a href=\"https://chrome.google.com/webstore/detail/keepassxc-browser/oboonakemofpalcgghocfoadofidjkkk\">"
                 "Google Chrome / Chromium / Vivaldi / Brave</a>",
                 snapInstructions));
    // clang-format on

    m_ui->scriptWarningWidget->setVisible(false);
    m_ui->scriptWarningWidget->setAutoHideTimeout(-1);
    m_ui->scriptWarningWidget->showMessage(
        tr("<b>Warning</b>, the keepassxc-proxy application was not found!"
           "<br />Please check the KeePassXC installation directory or confirm the custom path in advanced options."
           "<br />Browser integration WILL NOT WORK without the proxy application."
           "<br />Expected Path: "),
        MessageWidget::Warning);

    m_ui->warningWidget->showMessage(tr("<b>Warning:</b> The following options can be dangerous!"),
                                     MessageWidget::Warning);
    m_ui->warningWidget->setCloseButtonVisible(false);
    m_ui->warningWidget->setAutoHideTimeout(-1);

    m_ui->tabWidget->setEnabled(m_ui->enableBrowserSupport->isChecked());
    connect(m_ui->enableBrowserSupport, SIGNAL(toggled(bool)), m_ui->tabWidget, SLOT(setEnabled(bool)));

    m_ui->customProxyLocation->setEnabled(m_ui->useCustomProxy->isChecked());
    m_ui->customProxyLocationBrowseButton->setEnabled(m_ui->useCustomProxy->isChecked());
    connect(m_ui->useCustomProxy, SIGNAL(toggled(bool)), m_ui->customProxyLocation, SLOT(setEnabled(bool)));
    connect(m_ui->useCustomProxy, SIGNAL(toggled(bool)), m_ui->customProxyLocationBrowseButton, SLOT(setEnabled(bool)));
    connect(m_ui->customProxyLocationBrowseButton, SIGNAL(clicked()), this, SLOT(showProxyLocationFileDialog()));

#ifdef Q_OS_WIN
    // Brave uses Chrome's registry settings
    m_ui->braveSupport->setHidden(true);
    // Vivaldi uses Chrome's registry settings
    m_ui->vivaldiSupport->setHidden(true);
    m_ui->chromeSupport->setText("Chrome, Vivaldi, and Brave");
    // Tor Browser uses Firefox's registry settings
    m_ui->torBrowserSupport->setHidden(true);
    m_ui->firefoxSupport->setText("Firefox and Tor Browser");
#endif
    m_ui->browserGlobalWarningWidget->setVisible(false);
}

BrowserOptionDialog::~BrowserOptionDialog()
{
}

void BrowserOptionDialog::loadSettings()
{
    auto settings = browserSettings();
    m_ui->enableBrowserSupport->setChecked(settings->isEnabled());

    m_ui->showNotification->setChecked(settings->showNotification());
    m_ui->bestMatchOnly->setChecked(settings->bestMatchOnly());
    m_ui->unlockDatabase->setChecked(settings->unlockDatabase());
    m_ui->matchUrlScheme->setChecked(settings->matchUrlScheme());

    // hide unimplemented options
    // TODO: fix this
    m_ui->showNotification->hide();

    if (settings->sortByUsername()) {
        m_ui->sortByUsername->setChecked(true);
    } else {
        m_ui->sortByTitle->setChecked(true);
    }

    m_ui->alwaysAllowAccess->setChecked(settings->alwaysAllowAccess());
    m_ui->alwaysAllowUpdate->setChecked(settings->alwaysAllowUpdate());
    m_ui->httpAuthPermission->setChecked(settings->httpAuthPermission());
    m_ui->searchInAllDatabases->setChecked(settings->searchInAllDatabases());
    m_ui->supportKphFields->setChecked(settings->supportKphFields());
    m_ui->noMigrationPrompt->setChecked(settings->noMigrationPrompt());
    m_ui->supportBrowserProxy->setChecked(settings->supportBrowserProxy());
    m_ui->useCustomProxy->setChecked(settings->useCustomProxy());
    m_ui->customProxyLocation->setText(settings->customProxyLocation());
    m_ui->updateBinaryPath->setChecked(settings->updateBinaryPath());
    m_ui->allowExpiredCredentials->setChecked(settings->allowExpiredCredentials());
    m_ui->chromeSupport->setChecked(settings->chromeSupport());
    m_ui->chromiumSupport->setChecked(settings->chromiumSupport());
    m_ui->firefoxSupport->setChecked(settings->firefoxSupport());
#ifndef Q_OS_WIN
    m_ui->braveSupport->setChecked(settings->braveSupport());
    m_ui->vivaldiSupport->setChecked(settings->vivaldiSupport());
    m_ui->torBrowserSupport->setChecked(settings->torBrowserSupport());
#endif

#if defined(KEEPASSXC_DIST_APPIMAGE)
    m_ui->supportBrowserProxy->setChecked(true);
    m_ui->supportBrowserProxy->setEnabled(false);
#elif defined(KEEPASSXC_DIST_SNAP)
    // Disable settings that will not work
    m_ui->supportBrowserProxy->setChecked(true);
    m_ui->supportBrowserProxy->setEnabled(false);
    m_ui->useCustomProxy->setChecked(false);
    m_ui->useCustomProxy->setEnabled(false);
    m_ui->browsersGroupBox->setVisible(false);
    m_ui->browsersGroupBox->setEnabled(false);
    m_ui->updateBinaryPath->setChecked(false);
    m_ui->updateBinaryPath->setEnabled(false);
    // Show notice to user
    m_ui->browserGlobalWarningWidget->showMessage(tr("Please see special instructions for browser extension use below"),
                                                  MessageWidget::Warning);
    m_ui->browserGlobalWarningWidget->setCloseButtonVisible(false);
    m_ui->browserGlobalWarningWidget->setAutoHideTimeout(-1);
#endif

    // Check for native messaging host location errors
    QString path;
    if (!settings->checkIfProxyExists(path)) {
        QString text = m_ui->scriptWarningWidget->text();
        text.append(path);
        m_ui->scriptWarningWidget->setText(text);
        m_ui->scriptWarningWidget->setVisible(true);
    } else {
        m_ui->scriptWarningWidget->setVisible(false);
    }
}

void BrowserOptionDialog::saveSettings()
{
    auto settings = browserSettings();
    settings->setEnabled(m_ui->enableBrowserSupport->isChecked());
    settings->setShowNotification(m_ui->showNotification->isChecked());
    settings->setBestMatchOnly(m_ui->bestMatchOnly->isChecked());
    settings->setUnlockDatabase(m_ui->unlockDatabase->isChecked());
    settings->setMatchUrlScheme(m_ui->matchUrlScheme->isChecked());
    settings->setSortByUsername(m_ui->sortByUsername->isChecked());

    settings->setSupportBrowserProxy(m_ui->supportBrowserProxy->isChecked());
    settings->setUseCustomProxy(m_ui->useCustomProxy->isChecked());
    settings->setCustomProxyLocation(m_ui->customProxyLocation->text());

    settings->setUpdateBinaryPath(m_ui->updateBinaryPath->isChecked());
    settings->setAllowExpiredCredentials(m_ui->allowExpiredCredentials->isChecked());
    settings->setAlwaysAllowAccess(m_ui->alwaysAllowAccess->isChecked());
    settings->setAlwaysAllowUpdate(m_ui->alwaysAllowUpdate->isChecked());
    settings->setHttpAuthPermission(m_ui->httpAuthPermission->isChecked());
    settings->setSearchInAllDatabases(m_ui->searchInAllDatabases->isChecked());
    settings->setSupportKphFields(m_ui->supportKphFields->isChecked());
    settings->setNoMigrationPrompt(m_ui->noMigrationPrompt->isChecked());

    settings->setChromeSupport(m_ui->chromeSupport->isChecked());
    settings->setChromiumSupport(m_ui->chromiumSupport->isChecked());
    settings->setFirefoxSupport(m_ui->firefoxSupport->isChecked());
#ifndef Q_OS_WIN
    settings->setBraveSupport(m_ui->braveSupport->isChecked());
    settings->setVivaldiSupport(m_ui->vivaldiSupport->isChecked());
    settings->setTorBrowserSupport(m_ui->torBrowserSupport->isChecked());
#endif
}

void BrowserOptionDialog::showProxyLocationFileDialog()
{
#ifdef Q_OS_WIN
    QString fileTypeFilter(QString("%1 (*.exe);;%2 (*.*)").arg(tr("Executable Files"), tr("All Files")));
#else
    QString fileTypeFilter(QString("%1 (*)").arg(tr("Executable Files")));
#endif
    auto proxyLocation = QFileDialog::getOpenFileName(this,
                                                      tr("Select custom proxy location"),
                                                      QFileInfo(QCoreApplication::applicationDirPath()).filePath(),
                                                      fileTypeFilter);
    m_ui->customProxyLocation->setText(proxyLocation);
}

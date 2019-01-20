/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2018 Sami Vänttinen <sami.vanttinen@protonmail.com>
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

#include "DatabaseSettingsWidgetBrowser.h"
#include "ui_DatabaseSettingsWidgetBrowser.h"

#include <QProgressDialog>

#include "browser/BrowserSettings.h"
#include "core/Clock.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/MessageBox.h"

DatabaseSettingsWidgetBrowser::DatabaseSettingsWidgetBrowser(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetBrowser())
    , m_customData(new CustomData(this))
    , m_customDataModel(new QStandardItemModel(this))
    , m_browserService(nullptr)
{
    m_ui->setupUi(this);
    m_ui->removeCustomDataButton->setEnabled(false);
    m_ui->customDataTable->setModel(m_customDataModel);

    settingsWarning();

    // clang-format off
    connect(m_ui->customDataTable->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(toggleRemoveButton(QItemSelection)));
    // clang-format on

    connect(m_ui->removeCustomDataButton, SIGNAL(clicked()), SLOT(removeSelectedKey()));
    connect(m_ui->convertToCustomData, SIGNAL(clicked()), this, SLOT(convertAttributesToCustomData()));
    connect(m_ui->convertToCustomData, SIGNAL(clicked()), this, SLOT(updateSharedKeyList()));
    connect(m_ui->removeSharedEncryptionKeys, SIGNAL(clicked()), this, SLOT(removeSharedEncryptionKeys()));
    connect(m_ui->removeSharedEncryptionKeys, SIGNAL(clicked()), this, SLOT(updateSharedKeyList()));
    connect(m_ui->removeStoredPermissions, SIGNAL(clicked()), this, SLOT(removeStoredPermissions()));
}

DatabaseSettingsWidgetBrowser::~DatabaseSettingsWidgetBrowser()
{
}

CustomData* DatabaseSettingsWidgetBrowser::customData() const
{
    // Returns the current database customData from metadata. Otherwise return an empty customData member.
    if (m_db) {
        return m_db->metadata()->customData();
    }
    return m_customData;
}

void DatabaseSettingsWidgetBrowser::initialize()
{
    updateModel();
    settingsWarning();
}

void DatabaseSettingsWidgetBrowser::uninitialize()
{
}

void DatabaseSettingsWidgetBrowser::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
}

bool DatabaseSettingsWidgetBrowser::save()
{
    return true;
}

void DatabaseSettingsWidgetBrowser::removeSelectedKey()
{
    if (MessageBox::Yes
        != MessageBox::question(this,
                                tr("Delete the selected key?"),
                                tr("Do you really want to delete the selected key?\n"
                                   "This may prevent connection to the browser plugin."),
                                MessageBox::Yes | MessageBox::Cancel,
                                MessageBox::Cancel)) {
        return;
    }

    const QItemSelectionModel* itemSelectionModel = m_ui->customDataTable->selectionModel();
    if (itemSelectionModel) {
        for (const QModelIndex& index : itemSelectionModel->selectedRows(0)) {
            QString key = index.data().toString();
            key.insert(0, BrowserService::ASSOCIATE_KEY_PREFIX);
            customData()->remove(key);
        }
        updateModel();
    }
}

void DatabaseSettingsWidgetBrowser::toggleRemoveButton(const QItemSelection& selected)
{
    m_ui->removeCustomDataButton->setEnabled(!selected.isEmpty());
}

void DatabaseSettingsWidgetBrowser::updateModel()
{
    m_customDataModel->clear();
    m_customDataModel->setHorizontalHeaderLabels({tr("Key"), tr("Value")});

    for (const QString& key : customData()->keys()) {
        if (key.startsWith(BrowserService::ASSOCIATE_KEY_PREFIX)) {
            QString strippedKey = key;
            strippedKey.remove(BrowserService::ASSOCIATE_KEY_PREFIX);
            m_customDataModel->appendRow(QList<QStandardItem*>() << new QStandardItem(strippedKey)
                                                                 << new QStandardItem(customData()->value(key)));
        }
    }

    m_ui->removeCustomDataButton->setEnabled(false);
}

void DatabaseSettingsWidgetBrowser::settingsWarning()
{
    if (!browserSettings()->isEnabled()) {
        m_ui->convertToCustomData->setEnabled(false);
        m_ui->removeSharedEncryptionKeys->setEnabled(false);
        m_ui->removeStoredPermissions->setEnabled(false);
        m_ui->customDataTable->setEnabled(false);
        m_ui->warningWidget->showMessage(tr("Enable Browser Integration to access these settings."),
                                         MessageWidget::Warning);
        m_ui->warningWidget->setCloseButtonVisible(false);
        m_ui->warningWidget->setAutoHideTimeout(-1);
    } else {
        m_ui->convertToCustomData->setEnabled(true);
        m_ui->removeSharedEncryptionKeys->setEnabled(true);
        m_ui->removeStoredPermissions->setEnabled(true);
        m_ui->customDataTable->setEnabled(true);
        m_ui->warningWidget->hideMessage();
    }
}

void DatabaseSettingsWidgetBrowser::removeSharedEncryptionKeys()
{
    if (MessageBox::Yes
        != MessageBox::question(this,
                                tr("Disconnect all browsers"),
                                tr("Do you really want to disconnect all browsers?\n"
                                   "This may prevent connection to the browser plugin."),
                                MessageBox::Yes | MessageBox::Cancel,
                                MessageBox::Cancel)) {
        return;
    }

    QStringList keysToRemove;
    for (const QString& key : m_db->metadata()->customData()->keys()) {
        if (key.startsWith(BrowserService::ASSOCIATE_KEY_PREFIX)) {
            keysToRemove << key;
        }
    }

    if (keysToRemove.isEmpty()) {
        MessageBox::information(this,
                                tr("KeePassXC: No keys found"),
                                tr("No shared encryption keys found in KeePassXC settings."),
                                MessageBox::Ok);
        return;
    }

    for (const QString& key : keysToRemove) {
        m_db->metadata()->customData()->remove(key);
    }

    const int count = keysToRemove.count();
    MessageBox::information(this,
                            tr("KeePassXC: Removed keys from database"),
                            tr("Successfully removed %n encryption key(s) from KeePassXC settings.", "", count),
                            MessageBox::Ok);
}

void DatabaseSettingsWidgetBrowser::removeStoredPermissions()
{
    if (MessageBox::Yes
        != MessageBox::question(this,
                                tr("Forget all site-specific settings on entries"),
                                tr("Do you really want forget all site-specific settings on every entry?\n"
                                   "Permissions to access entries will be revoked."),
                                MessageBox::Yes | MessageBox::Cancel,
                                MessageBox::Cancel)) {
        return;
    }

    QList<Entry*> entries = m_db->rootGroup()->entriesRecursive();

    QProgressDialog progress(tr("Removing stored permissions…"), tr("Abort"), 0, entries.count());
    progress.setWindowModality(Qt::WindowModal);

    uint counter = 0;
    for (Entry* entry : entries) {
        if (progress.wasCanceled()) {
            return;
        }

        if (entry->customData()->contains(BrowserService::KEEPASSXCBROWSER_NAME)) {
            entry->beginUpdate();
            entry->customData()->remove(BrowserService::KEEPASSXCBROWSER_NAME);
            entry->endUpdate();
            ++counter;
        }
        progress.setValue(progress.value() + 1);
    }
    progress.reset();

    if (counter > 0) {
        MessageBox::information(this,
                                tr("KeePassXC: Removed permissions"),
                                tr("Successfully removed permissions from %n entry(s).", "", counter),
                                MessageBox::Ok);
    } else {
        MessageBox::information(this,
                                tr("KeePassXC: No entry with permissions found!"),
                                tr("The active database does not contain an entry with permissions."),
                                MessageBox::Ok);
    }
}

void DatabaseSettingsWidgetBrowser::convertAttributesToCustomData()
{
    if (MessageBox::Yes
        != MessageBox::question(
               this,
               tr("Move KeePassHTTP attributes to custom data"),
               tr("Do you really want to move all legacy browser integration data to the latest standard?\n"
                  "This is necessary to maintain compatibility with the browser plugin."),
               MessageBox::Yes | MessageBox::Cancel,
               MessageBox::Cancel)) {
        return;
    }

    m_browserService.convertAttributesToCustomData(m_db);
}

// Updates the shared key list after the list is cleared
void DatabaseSettingsWidgetBrowser::updateSharedKeyList()
{
    updateModel();
}

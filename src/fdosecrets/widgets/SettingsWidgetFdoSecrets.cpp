/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "SettingsWidgetFdoSecrets.h"
#include "ui_SettingsWidgetFdoSecrets.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Session.h"

#include "core/DatabaseIcons.h"
#include "core/FilePath.h"
#include "gui/DatabaseWidget.h"

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QToolBar>
#include <QVariant>

using FdoSecrets::Collection;
using FdoSecrets::Service;
using FdoSecrets::Session;

SettingsWidgetFdoSecrets::SettingsWidgetFdoSecrets(FdoSecretsPlugin* plugin, QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidgetFdoSecrets())
    , m_plugin(plugin)
{
    m_ui->setupUi(this);

    auto sessHeader = m_ui->tableSessions->horizontalHeader();
    sessHeader->setSelectionMode(QAbstractItemView::NoSelection);
    sessHeader->setSectionsClickable(false);
    sessHeader->setSectionResizeMode(0, QHeaderView::Stretch); // application
    sessHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents); // disconnect button

    auto dbHeader = m_ui->tableDatabases->horizontalHeader();
    dbHeader->setSelectionMode(QAbstractItemView::NoSelection);
    dbHeader->setSectionsClickable(false);
    dbHeader->setSectionResizeMode(0, QHeaderView::Stretch); // file name
    dbHeader->setSectionResizeMode(1, QHeaderView::Stretch); // group
    dbHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents); // manage button

    m_ui->tabWidget->setEnabled(m_ui->enableFdoSecretService->isChecked());
    connect(m_ui->enableFdoSecretService, &QCheckBox::toggled, m_ui->tabWidget, &QTabWidget::setEnabled);
}

SettingsWidgetFdoSecrets::~SettingsWidgetFdoSecrets() = default;

void SettingsWidgetFdoSecrets::populateSessions(bool enabled)
{
    m_ui->tableSessions->setRowCount(0);

    auto service = m_plugin->serviceInstance();
    if (!service || !enabled) {
        return;
    }

    for (const auto& sess : service->sessions()) {
        addSessionRow(sess);
    }
}

void SettingsWidgetFdoSecrets::addSessionRow(Session* sess)
{
    auto row = m_ui->tableSessions->rowCount();
    m_ui->tableSessions->insertRow(row);

    // column 0: application name
    auto item = new QTableWidgetItem(sess->peer());
    item->setData(Qt::UserRole, QVariant::fromValue(sess));
    m_ui->tableSessions->setItem(row, 0, item);

    // column 1: disconnect button
    auto btn = new QPushButton(tr("Disconnect"));
    connect(btn, &QPushButton::clicked, sess, &Session::close);
    m_ui->tableSessions->setCellWidget(row, 1, btn);

    // column 2: hidden uuid
    m_ui->tableSessions->setItem(row, 2, new QTableWidgetItem(sess->id()));
}

void SettingsWidgetFdoSecrets::removeSessionRow(Session* sess)
{
    int row = 0;
    while (row != m_ui->tableSessions->rowCount()) {
        auto item = m_ui->tableSessions->item(row, 0);
        const auto itemSess = item->data(Qt::UserRole).value<Session*>();
        if (itemSess == sess) {
            break;
        }
        ++row;
    }
    if (row == m_ui->tableSessions->rowCount()) {
        qWarning() << "Unknown Fdo Secret Service session" << sess->id() << "while removing collection from table";
        return;
    }

    m_ui->tableSessions->removeRow(row);
}

void SettingsWidgetFdoSecrets::populateDatabases(bool enabled)
{
    m_ui->tableDatabases->setRowCount(0);

    auto service = m_plugin->serviceInstance();
    if (!service || !enabled) {
        return;
    }

    auto ret = service->collections();
    if (ret.isError()) {
        return;
    }
    for (const auto& coll : ret.value()) {
        addDatabaseRow(coll);
    }
}

void SettingsWidgetFdoSecrets::addDatabaseRow(Collection* coll)
{
    auto row = m_ui->tableDatabases->rowCount();
    m_ui->tableDatabases->insertRow(row);

    // column 0: File name
    QFileInfo fi(coll->backend()->database()->filePath());
    auto item = new QTableWidgetItem(fi.fileName());
    item->setData(Qt::UserRole, QVariant::fromValue(coll));
    m_ui->tableDatabases->setItem(row, 0, item);

    // column 2: manage button: hboxlayout: unlock/lock settings
    // create this first so we have a widget to bind connection to,
    // which can then be auto deleted when the row is deleted.
    auto widget = createManageButtons(coll);
    m_ui->tableDatabases->setCellWidget(row, 2, widget);

    // column 1: Group name
    auto itemGroupName = new QTableWidgetItem();
    updateExposedGroupItem(itemGroupName, coll);

    connect(coll, &Collection::collectionLockChanged, widget, [this, itemGroupName, coll](bool) {
        updateExposedGroupItem(itemGroupName, coll);
    });

    m_ui->tableDatabases->setItem(row, 1, itemGroupName);
}

QWidget* SettingsWidgetFdoSecrets::createManageButtons(Collection* coll)
{
    auto toolbar = new QToolBar;
    toolbar->setFloatable(false);
    toolbar->setMovable(false);

    // db settings
    auto dbSettingsAct = new QAction(tr("Database settings"), toolbar);
    dbSettingsAct->setIcon(filePath()->icon(QStringLiteral("actions"), QStringLiteral("document-edit")));
    dbSettingsAct->setToolTip(tr("Edit database settings"));
    dbSettingsAct->setEnabled(!coll->locked().value());
    connect(dbSettingsAct, &QAction::triggered, this, [this, coll]() {
        auto db = coll->backend();
        m_plugin->serviceInstance()->doSwitchToChangeDatabaseSettings(db);
    });
    toolbar->addAction(dbSettingsAct);

    // unlock/lock
    auto lockAct = new QAction(tr("Unlock database"), toolbar);
    lockAct->setIcon(filePath()->icon(QStringLiteral("actions"), QStringLiteral("object-locked"), true));
    lockAct->setToolTip(tr("Unlock database to show more information"));
    connect(coll, &Collection::collectionLockChanged, lockAct, [lockAct, dbSettingsAct](bool locked) {
        if (locked) {
            lockAct->setIcon(filePath()->icon(QStringLiteral("actions"), QStringLiteral("object-locked"), true));
            lockAct->setToolTip(tr("Unlock database to show more information"));
        } else {
            lockAct->setIcon(filePath()->icon(QStringLiteral("actions"), QStringLiteral("object-unlocked"), true));
            lockAct->setToolTip(tr("Lock database"));
        }
        dbSettingsAct->setEnabled(!locked);
    });
    connect(lockAct, &QAction::triggered, this, [coll]() {
        if (coll->locked().value()) {
            coll->doUnlock();
        } else {
            coll->doLock();
        }
    });
    toolbar->addAction(lockAct);

    return toolbar;
}

void SettingsWidgetFdoSecrets::updateExposedGroupItem(QTableWidgetItem* item, Collection* coll)
{
    if (coll->locked().value()) {
        item->setText(tr("Unlock to show"));
        item->setIcon(filePath()->icon(QStringLiteral("apps"), QStringLiteral("object-locked"), true));
        QFont font;
        font.setItalic(true);
        item->setFont(font);
        return;
    }

    auto db = coll->backend()->database();
    auto group = db->rootGroup()->findGroupByUuid(FdoSecrets::settings()->exposedGroup(db));
    if (group) {
        item->setText(group->name());
        item->setIcon(group->isExpired() ? databaseIcons()->iconPixmap(DatabaseIcons::ExpiredIconIndex)
                                         : group->iconScaledPixmap());
        if (group->isExpired()) {
            QFont font;
            font.setStrikeOut(true);
            item->setFont(font);
        }
    } else {
        item->setText(tr("None"));
        item->setIcon(filePath()->icon(QStringLiteral("apps"), QStringLiteral("paint-none"), true));
    }
}

void SettingsWidgetFdoSecrets::removeDatabaseRow(Collection* coll)
{
    int row = 0;
    while (row != m_ui->tableDatabases->rowCount()) {
        auto item = m_ui->tableDatabases->item(row, 0);
        const auto itemColl = item->data(Qt::UserRole).value<Collection*>();
        if (itemColl == coll) {
            break;
        }
        ++row;
    }
    if (row == m_ui->tableDatabases->rowCount()) {
        qWarning() << "Unknown Fdo Secret Service collection" << coll->name() << "while removing collection from table";
        return;
    }

    m_ui->tableDatabases->removeRow(row);
}

void SettingsWidgetFdoSecrets::loadSettings()
{
    m_ui->enableFdoSecretService->setChecked(FdoSecrets::settings()->isEnabled());
    m_ui->showNotification->setChecked(FdoSecrets::settings()->showNotification());
    m_ui->noConfirmDeleteItem->setChecked(FdoSecrets::settings()->noConfirmDeleteItem());
}

void SettingsWidgetFdoSecrets::saveSettings()
{
    FdoSecrets::settings()->setEnabled(m_ui->enableFdoSecretService->isChecked());
    FdoSecrets::settings()->setShowNotification(m_ui->showNotification->isChecked());
    FdoSecrets::settings()->setNoConfirmDeleteItem(m_ui->noConfirmDeleteItem->isChecked());
}

void SettingsWidgetFdoSecrets::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    QMetaObject::invokeMethod(this, "updateTables", Qt::QueuedConnection, Q_ARG(bool, true));
}

void SettingsWidgetFdoSecrets::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);

    QMetaObject::invokeMethod(this, "updateTables", Qt::QueuedConnection, Q_ARG(bool, false));
}

void SettingsWidgetFdoSecrets::updateTables(bool enabled)
{
    if (enabled) {
        // update the table
        populateDatabases(m_ui->enableFdoSecretService->isChecked());
        populateSessions(m_ui->enableFdoSecretService->isChecked());

        // re-layout the widget to adjust the table cell size
        adjustSize();

        connect(m_ui->enableFdoSecretService, &QCheckBox::toggled, this, &SettingsWidgetFdoSecrets::populateSessions);
        connect(m_ui->enableFdoSecretService, &QCheckBox::toggled, this, &SettingsWidgetFdoSecrets::populateDatabases);

        auto service = m_plugin->serviceInstance();
        if (service) {
            connect(service, &Service::sessionOpened, this, &SettingsWidgetFdoSecrets::addSessionRow);
            connect(service, &Service::sessionClosed, this, &SettingsWidgetFdoSecrets::removeSessionRow);
            connect(service, &Service::collectionCreated, this, &SettingsWidgetFdoSecrets::addDatabaseRow);
            connect(service, &Service::collectionDeleted, this, &SettingsWidgetFdoSecrets::removeDatabaseRow);
        }
    } else {
        disconnect(
            m_ui->enableFdoSecretService, &QCheckBox::toggled, this, &SettingsWidgetFdoSecrets::populateSessions);
        disconnect(
            m_ui->enableFdoSecretService, &QCheckBox::toggled, this, &SettingsWidgetFdoSecrets::populateDatabases);

        auto service = m_plugin->serviceInstance();
        if (service) {
            disconnect(service, &Service::sessionOpened, this, &SettingsWidgetFdoSecrets::addSessionRow);
            disconnect(service, &Service::sessionClosed, this, &SettingsWidgetFdoSecrets::removeSessionRow);
            disconnect(service, &Service::collectionCreated, this, &SettingsWidgetFdoSecrets::addDatabaseRow);
            disconnect(service, &Service::collectionDeleted, this, &SettingsWidgetFdoSecrets::removeDatabaseRow);
        }
    }
}

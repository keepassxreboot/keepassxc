/**
 ***************************************************************************
 * @file OptionDialog.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "OptionDialog.h"
#include "ui_OptionDialog.h"
#include "HttpSettings.h"

#include "core/FilePath.h"

#include <QMessageBox>

OptionDialog::OptionDialog(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::OptionDialog())
{
    m_ui->setupUi(this);
    connect(m_ui->removeSharedEncryptionKeys, SIGNAL(clicked()), this, SIGNAL(removeSharedEncryptionKeys()));
    connect(m_ui->removeStoredPermissions, SIGNAL(clicked()), this, SIGNAL(removeStoredPermissions()));

    m_ui->warningWidget->showMessage(tr("The following options can be dangerous!\nChange them only if you know what you are doing."), MessageWidget::Warning);
    m_ui->warningWidget->setIcon(FilePath::instance()->icon("status", "dialog-warning"));
    m_ui->warningWidget->setCloseButtonVisible(false);
}

OptionDialog::~OptionDialog()
{
}

void OptionDialog::loadSettings()
{
    HttpSettings settings;
    m_ui->enableHttpServer->setChecked(settings.isEnabled());

    m_ui->showNotification->setChecked(settings.showNotification());
    m_ui->bestMatchOnly->setChecked(settings.bestMatchOnly());
    m_ui->unlockDatabase->setChecked(settings.unlockDatabase());
    m_ui->matchUrlScheme->setChecked(settings.matchUrlScheme());
    if (settings.sortByUsername())
        m_ui->sortByUsername->setChecked(true);
    else
        m_ui->sortByTitle->setChecked(true);
    m_ui->httpPort->setText(QString::number(settings.httpPort()));

    m_ui->alwaysAllowAccess->setChecked(settings.alwaysAllowAccess());
    m_ui->alwaysAllowUpdate->setChecked(settings.alwaysAllowUpdate());
    m_ui->searchInAllDatabases->setChecked(settings.searchInAllDatabases());
    m_ui->supportKphFields->setChecked(settings.supportKphFields());

    m_ui->passwordGenerator->loadSettings();
}

void OptionDialog::saveSettings()
{
    HttpSettings settings;
    settings.setEnabled(m_ui->enableHttpServer->isChecked());

    settings.setShowNotification(m_ui->showNotification->isChecked());
    settings.setBestMatchOnly(m_ui->bestMatchOnly->isChecked());
    settings.setUnlockDatabase(m_ui->unlockDatabase->isChecked());
    settings.setMatchUrlScheme(m_ui->matchUrlScheme->isChecked());
    settings.setSortByUsername(m_ui->sortByUsername->isChecked());
    
    int port = m_ui->httpPort->text().toInt();
    if (port < 1024) {
        QMessageBox::warning(this, tr("Cannot bind to privileged ports"),
            tr("Cannot bind to privileged ports below 1024!\nUsing default port 19455."));
        port = 19455;
    }
    settings.setHttpPort(port);
    settings.setAlwaysAllowAccess(m_ui->alwaysAllowAccess->isChecked());
    settings.setAlwaysAllowUpdate(m_ui->alwaysAllowUpdate->isChecked());
    settings.setSearchInAllDatabases(m_ui->searchInAllDatabases->isChecked());
    settings.setSupportKphFields(m_ui->supportKphFields->isChecked());

    m_ui->passwordGenerator->saveSettings();
}

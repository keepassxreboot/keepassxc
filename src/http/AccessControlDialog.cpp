/**
 ***************************************************************************
 * @file AccessControlDialog.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "AccessControlDialog.h"
#include "ui_AccessControlDialog.h"
#include "core/Entry.h"

AccessControlDialog::AccessControlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccessControlDialog)
{
    ui->setupUi(this);
    connect(ui->allowButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->denyButton, SIGNAL(clicked()), this, SLOT(reject()));
}

AccessControlDialog::~AccessControlDialog()
{
    delete ui;
}

void AccessControlDialog::setUrl(const QString &url)
{
    ui->label->setText(QString(tr("%1 has requested access to passwords for the following item(s).\n"
                                  "Please select whether you want to allow access.")).arg(QUrl(url).host()));
}

void AccessControlDialog::setItems(const QList<Entry *> &items)
{
    Q_FOREACH (Entry * entry, items)
        ui->itemsList->addItem(entry->title() + " - " + entry->username());
}

bool AccessControlDialog::remember() const
{
    return ui->rememberDecisionCheckBox->isChecked();
}

void AccessControlDialog::setRemember(bool r)
{
    ui->rememberDecisionCheckBox->setChecked(r);
}

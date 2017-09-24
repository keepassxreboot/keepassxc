/*
*  Copyright (C) 2013 Francois Ferrand
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

#include "KeyAcceptDialog.h"
#include "ui_KeyAcceptDialog.h"
#include "core/Entry.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <Qt>
#include <QDialogButtonBox>
#include <QPushButton>

KeyAcceptDialog::KeyAcceptDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyAcceptDialog())
{
    this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(ui->keyNameLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(keyEditChanged(const QString)));

    QStandardItemModel* listViewModel = new QStandardItemModel(ui->databasesListView);
    ui->databasesListView->setModel(listViewModel);
    connect(listViewModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(modelItemChanged(QStandardItem*)));
}

KeyAcceptDialog::~KeyAcceptDialog()
{
}

void KeyAcceptDialog::setItems(const QList<QString> &items)
{
    QStandardItemModel* listViewModel = qobject_cast<QStandardItemModel*>(ui->databasesListView->model());

    listViewModel->clear();

    for (QString item: items) {
        QStandardItem* listItem = new QStandardItem(item);
        listItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        listItem->setData(Qt::Unchecked, Qt::CheckStateRole);

        listViewModel->appendRow(listItem);
    }
}

void KeyAcceptDialog::setItemEnabled(int itemIndex, bool enabled)
{
    QStandardItemModel* listViewModel = qobject_cast<QStandardItemModel*>(ui->databasesListView->model());

    QStandardItem* item = listViewModel->item(itemIndex);

    if (item) {
        item->setEnabled(enabled);
    }
}

void KeyAcceptDialog::setItemChecked(int itemIndex, bool checked)
{
    QStandardItemModel* listViewModel = qobject_cast<QStandardItemModel*>(ui->databasesListView->model());

    QStandardItem* item = listViewModel->item(itemIndex);

    if (item) {
        if (checked) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

QList<int> KeyAcceptDialog::getCheckedItems()
{
    QStandardItemModel* listViewModel = qobject_cast<QStandardItemModel*>(ui->databasesListView->model());

    int numRows = listViewModel->rowCount();

    //XXX Memory Leak
    QList<int> resultList;

    for (int row = 0; row < numRows; row++) {
        QStandardItem* item = listViewModel->item(row);
        if (item->checkState() == Qt::Checked) {
            resultList << row;
        }
    }

    return resultList;
}

QString KeyAcceptDialog::getKeyName()
{
    return ui->keyNameLineEdit->text();
}

void KeyAcceptDialog::checkAcceptable()
{
    QStandardItemModel* listViewModel = qobject_cast<QStandardItemModel*>(ui->databasesListView->model());
    int numRows = listViewModel->rowCount();
    bool hasChecked = false;
    for (int row = 0; row < numRows; row++) {
        QStandardItem* item = listViewModel->item(row);
        if (item->checkState() == Qt::Checked) {
            hasChecked = true;
            break;
        }
    }

    //TODO: Make translateable
    if (getKeyName().isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setToolTip("Make sure the key name is not empty!");
    }
    else if (!hasChecked) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setToolTip("Check at least one database");
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setToolTip("");
    }
}

void KeyAcceptDialog::keyEditChanged(const QString &text)
{
    (void)text;

    checkAcceptable();
}

void KeyAcceptDialog::modelItemChanged(QStandardItem *item)
{
    (void)item;

    checkAcceptable();
}

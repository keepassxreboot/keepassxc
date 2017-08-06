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

KeyAcceptDialog::KeyAcceptDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyAcceptDialog())
{
    this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QStandardItemModel* listViewModel = new QStandardItemModel(ui->databasesListView);
    ui->databasesListView->setModel(listViewModel);
}

KeyAcceptDialog::~KeyAcceptDialog()
{
}

void KeyAcceptDialog::setItems(const QList<QString> &items)
{
    QStandardItemModel* listViewModel = (QStandardItemModel*) ui->databasesListView->model();

    for (QString item: items) {
        QStandardItem* listItem = new QStandardItem(item);
        listItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        listItem->setData(Qt::Unchecked, Qt::CheckStateRole);

        listViewModel->appendRow(listItem);
    }
}

void KeyAcceptDialog::setItemChecked(int itemIndex, bool checked)
{
    QStandardItemModel* listViewModel = (QStandardItemModel*) ui->databasesListView->model();

    QStandardItem* item = listViewModel->item(itemIndex);

    if (item) {
        if (checked) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

QList<int>* KeyAcceptDialog::getCheckedItems()
{
    QStandardItemModel* listViewModel = (QStandardItemModel*) ui->databasesListView->model();

    int numRows = listViewModel->rowCount();

    //XXX Memory Leak
    QList<int>* resultList = new QList<int>;

    for (int row = 0; row < numRows; row++) {
        QStandardItem* item = listViewModel->item(row);
        if (item->checkState() == Qt::Checked) {
            resultList->append(row);
        }
    }

    return resultList;
}

QString KeyAcceptDialog::getKeyName()
{
    return ui->keyNameLineEdit->text();
}

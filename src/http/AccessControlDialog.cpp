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

#include "AccessControlDialog.h"
#include "ui_AccessControlDialog.h"
#include "core/Entry.h"

AccessControlDialog::AccessControlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccessControlDialog())
{
    this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);
    connect(ui->allowButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->denyButton, SIGNAL(clicked()), this, SLOT(reject()));
}

AccessControlDialog::~AccessControlDialog()
{
}

void AccessControlDialog::setUrl(const QString &url)
{
    ui->label->setText(QString(tr("%1 has requested access to passwords for the following item(s).\n"
                                  "Please select whether you want to allow access.")).arg(QUrl(url).host()));
}

void AccessControlDialog::setItems(const QList<Entry*> &items)
{
    for (Entry* entry: items) {
        ui->itemsList->addItem(entry->title() + " - " + entry->username());
    }
}

bool AccessControlDialog::remember() const
{
    return ui->rememberDecisionCheckBox->isChecked();
}

void AccessControlDialog::setRemember(bool r)
{
    ui->rememberDecisionCheckBox->setChecked(r);
}

/*
 *  Copyright (C) 2013 Francois Ferrand
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

#include "BrowserAccessControlDialog.h"
#include "ui_BrowserAccessControlDialog.h"

#include "core/Entry.h"

BrowserAccessControlDialog::BrowserAccessControlDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::BrowserAccessControlDialog())
{
    this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);
    connect(m_ui->allowButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_ui->denyButton, SIGNAL(clicked()), this, SLOT(reject()));
}

BrowserAccessControlDialog::~BrowserAccessControlDialog()
{
}

void BrowserAccessControlDialog::setUrl(const QString& url)
{
    m_ui->label->setText(QString(tr("%1 has requested access to passwords for the following item(s).\n"
                                    "Please select whether you want to allow access."))
                             .arg(QUrl(url).host()));
}

void BrowserAccessControlDialog::setItems(const QList<Entry*>& items)
{
    for (Entry* entry : items) {
        m_ui->itemsList->addItem(entry->title() + " - " + entry->username());
    }
}

bool BrowserAccessControlDialog::remember() const
{
    return m_ui->rememberDecisionCheckBox->isChecked();
}

void BrowserAccessControlDialog::setRemember(bool r)
{
    m_ui->rememberDecisionCheckBox->setChecked(r);
}

void BrowserAccessControlDialog::setHTTPAuth(bool httpAuth)
{
    m_ui->rememberDecisionCheckBox->setVisible(!httpAuth);
}

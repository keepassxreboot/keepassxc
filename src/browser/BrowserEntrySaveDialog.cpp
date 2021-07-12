/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "BrowserEntrySaveDialog.h"
#include "ui_BrowserEntrySaveDialog.h"

#include "gui/DatabaseWidget.h"

BrowserEntrySaveDialog::BrowserEntrySaveDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::BrowserEntrySaveDialog())
{
    this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);
    connect(m_ui->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    m_ui->itemsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->label->setText(QString(tr("You have multiple databases open.\n"
                                    "Please select the correct database for saving credentials.")));
}

BrowserEntrySaveDialog::~BrowserEntrySaveDialog()
{
}

int BrowserEntrySaveDialog::setItems(QList<DatabaseWidget*>& databaseWidgets, DatabaseWidget* currentWidget) const
{
    uint counter = 0;
    int activeIndex = -1;
    for (const auto dbWidget : databaseWidgets) {
        QString databaseName = dbWidget->database()->metadata()->name();
        QString databaseFileName = dbWidget->database()->filePath();

        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, counter);

        // Show database name (and filename if the name has been set in metadata)
        if (databaseName == databaseFileName) {
            item->setText(databaseFileName);
        } else {
            item->setText(QString("%1 (%2)").arg(databaseName, databaseFileName));
        }

        if (currentWidget == dbWidget) {
            activeIndex = counter;
        }

        m_ui->itemsList->addItem(item);
        ++counter;
    }

    // This must be done after the whole list is filled
    if (activeIndex >= 0) {
        m_ui->itemsList->item(activeIndex)->setSelected(true);
    }

    m_ui->itemsList->selectAll();
    return databaseWidgets.length();
}

QList<QListWidgetItem*> BrowserEntrySaveDialog::getSelected() const
{
    return m_ui->itemsList->selectedItems();
}

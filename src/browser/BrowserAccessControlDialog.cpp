/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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
#include <QUrl>

#include "core/Entry.h"
#include <QCloseEvent>

BrowserAccessControlDialog::BrowserAccessControlDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::BrowserAccessControlDialog())
    , m_entriesAccepted(false)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);

    connect(m_ui->allowButton, SIGNAL(clicked()), SLOT(acceptSelections()));
    connect(m_ui->denyButton, SIGNAL(clicked()), SLOT(rejectSelections()));
    connect(this, SIGNAL(rejected()), this, SIGNAL(closed()));
}

BrowserAccessControlDialog::~BrowserAccessControlDialog()
{
}

void BrowserAccessControlDialog::closeEvent(QCloseEvent* event)
{
    // Emits closed signal when clicking X from title bar
    emit closed();
    event->accept();
}

void BrowserAccessControlDialog::setItems(const QList<Entry*>& entriesToConfirm,
                                          const QList<Entry*>& allowedEntries,
                                          const QString& urlString,
                                          bool httpAuth)
{
    m_entriesToConfirm = entriesToConfirm;
    m_allowedEntries = allowedEntries;

    QUrl url(urlString);
    m_ui->siteLabel->setText(m_ui->siteLabel->text().arg(
        url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment)));

    m_ui->rememberDecisionCheckBox->setVisible(!httpAuth);
    m_ui->rememberDecisionCheckBox->setChecked(false);

    m_ui->itemsTable->setRowCount(entriesToConfirm.count());
    m_ui->itemsTable->setColumnCount(2);

    int row = 0;
    for (const auto& entry : entriesToConfirm) {
        auto item = new QTableWidgetItem();
        item->setText(entry->title() + " - " + entry->username());
        item->setData(Qt::UserRole, row);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        m_ui->itemsTable->setItem(row, 0, item);

        auto disableButton = new QPushButton(tr("Disable for this site"));
        disableButton->setAutoDefault(false);

        connect(disableButton, &QAbstractButton::pressed, [&, item] {
            emit disableAccess(item);
            m_ui->itemsTable->removeRow(item->row());

            if (m_ui->itemsTable->rowCount() == 0) {
                emit closed();
            }
        });
        m_ui->itemsTable->setCellWidget(row, 1, disableButton);

        ++row;
    }

    m_ui->itemsTable->resizeColumnsToContents();
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_ui->allowButton->setFocus();
}

bool BrowserAccessControlDialog::remember() const
{
    return m_ui->rememberDecisionCheckBox->isChecked();
}

bool BrowserAccessControlDialog::entriesAccepted() const
{
    return m_entriesAccepted;
}

QList<QTableWidgetItem*> BrowserAccessControlDialog::getSelectedEntries() const
{
    QList<QTableWidgetItem*> selected;
    for (int i = 0; i < m_ui->itemsTable->rowCount(); ++i) {
        auto item = m_ui->itemsTable->item(i, 0);
        if (item->checkState() == Qt::Checked) {
            selected.append(item);
        }
    }
    return selected;
}

QList<QTableWidgetItem*> BrowserAccessControlDialog::getNonSelectedEntries() const
{
    QList<QTableWidgetItem*> notSelected;
    for (int i = 0; i < m_ui->itemsTable->rowCount(); ++i) {
        auto item = m_ui->itemsTable->item(i, 0);
        if (item->checkState() != Qt::Checked) {
            notSelected.append(item);
        }
    }
    return notSelected;
}

void BrowserAccessControlDialog::acceptSelections()
{
    auto selectedEntries = getSelectedEntries();

    m_entriesAccepted = true;
    emit acceptEntries(selectedEntries, m_entriesToConfirm, m_allowedEntries);
    emit closed();
}

void BrowserAccessControlDialog::rejectSelections()
{
    auto rejectedEntries = getNonSelectedEntries();
    emit rejectEntries(rejectedEntries, m_entriesToConfirm);
    emit closed();
}

/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2013 Francois Ferrand
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
#include "gui/Icons.h"

BrowserAccessControlDialog::BrowserAccessControlDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::BrowserAccessControlDialog())
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);

    connect(m_ui->allowButton, SIGNAL(clicked()), SLOT(accept()));
    connect(m_ui->denyButton, SIGNAL(clicked()), SLOT(reject()));
    connect(m_ui->itemsTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(accept()));
    connect(m_ui->itemsTable->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this,
            SLOT(selectionChanged()));
    connect(m_ui->itemsTable, SIGNAL(acceptSelections()), SLOT(accept()));
    connect(m_ui->itemsTable, SIGNAL(focusInWithoutSelections()), this, SLOT(selectionChanged()));
}

BrowserAccessControlDialog::~BrowserAccessControlDialog()
{
}

void BrowserAccessControlDialog::setEntries(const QList<Entry*>& entriesToConfirm,
                                            const QString& urlString,
                                            bool httpAuth)
{
    QUrl url(urlString);
    m_ui->siteLabel->setText(m_ui->siteLabel->text().arg(
        url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment)));
    m_ui->siteLabel->setToolTip(urlString);

    m_ui->rememberDecisionCheckBox->setVisible(!httpAuth);
    m_ui->rememberDecisionCheckBox->setChecked(false);

    m_ui->itemsTable->setRowCount(entriesToConfirm.count());
    m_ui->itemsTable->setColumnCount(3);

    int row = 0;
    for (const auto& entry : entriesToConfirm) {
        addEntryToList(entry, row);
        ++row;
    }

    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_ui->itemsTable->selectAll();
    m_ui->allowButton->setFocus();
}

void BrowserAccessControlDialog::addEntryToList(const Entry* entry, int row)
{
    const auto titleItem = new QTableWidgetItem();
    const auto entryTitle = entry->resolveMultiplePlaceholders(entry->title());
    const auto entryUrl = entry->resolveMultiplePlaceholders(entry->url());
    titleItem->setText(entryTitle);
    titleItem->setToolTip(entryUrl);
    titleItem->setData(Qt::UserRole, row);
    titleItem->setFlags(titleItem->flags() | Qt::ItemIsSelectable);
    m_ui->itemsTable->setItem(row, 0, titleItem);

    const auto usernameItem = new QTableWidgetItem();
    const auto entryUsername = entry->resolveMultiplePlaceholders(entry->username());
    usernameItem->setText(entryUsername);
    usernameItem->setData(Qt::UserRole, row);
    m_ui->itemsTable->setItem(row, 1, usernameItem);

    auto disableButton = new QPushButton();
    disableButton->setIcon(icons()->icon("entry-delete"));
    disableButton->setToolTip(tr("Disable for this site"));

    connect(disableButton, &QAbstractButton::pressed, [&, titleItem, usernameItem, disableButton] {
        auto font = titleItem->font();
        if (titleItem->flags() == Qt::NoItemFlags) {
            titleItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            usernameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            titleItem->setSelected(true);
            usernameItem->setSelected(true);

            font.setStrikeOut(false);
            titleItem->setFont(font);
            usernameItem->setFont(font);

            disableButton->setIcon(icons()->icon("entry-delete"));
            disableButton->setToolTip(tr("Disable for this site"));
            m_ui->rememberDecisionCheckBox->setEnabled(true);
        } else {
            titleItem->setFlags(Qt::NoItemFlags);
            usernameItem->setFlags(Qt::NoItemFlags);
            titleItem->setSelected(false);
            usernameItem->setSelected(false);

            font.setStrikeOut(true);
            titleItem->setFont(font);
            usernameItem->setFont(font);

            disableButton->setIcon(icons()->icon("entry-restore"));
            disableButton->setToolTip(tr("Undo"));

            // Disable Remember checkbox if all items are disabled
            auto areAllDisabled = BrowserAccessControlDialog::areAllDisabled();
            m_ui->rememberDecisionCheckBox->setEnabled(!areAllDisabled);
        }
    });

    m_ui->itemsTable->setCellWidget(row, 2, disableButton);
}

bool BrowserAccessControlDialog::remember() const
{
    return m_ui->rememberDecisionCheckBox->isChecked();
}

QList<QTableWidgetItem*> BrowserAccessControlDialog::getEntries(SelectionType selectionType) const
{
    QList<QTableWidgetItem*> selected;
    for (auto& item : getAllItems()) {
        // Add to list depending on selection type and item status
        if ((selectionType == SelectionType::Selected && item->isSelected())
            || (selectionType == SelectionType::NonSelected && !item->isSelected())
            || (selectionType == SelectionType::Disabled && item->flags() == Qt::NoItemFlags)) {
            selected.append(item);
        }
    }
    return selected;
}

void BrowserAccessControlDialog::selectionChanged()
{
    auto selectedRows = m_ui->itemsTable->selectionModel()->selectedRows();

    m_ui->allowButton->setEnabled(!selectedRows.isEmpty());
    m_ui->allowButton->setDefault(!selectedRows.isEmpty());
    m_ui->allowButton->setAutoDefault(!selectedRows.isEmpty());

    if (selectedRows.isEmpty()) {
        m_ui->allowButton->clearFocus();
        m_ui->denyButton->setFocus();
    }
}

bool BrowserAccessControlDialog::areAllDisabled() const
{
    for (const auto& item : getAllItems()) {
        if (item->flags() != Qt::NoItemFlags) {
            return false;
        }
    }

    return true;
}

QList<QTableWidgetItem*> BrowserAccessControlDialog::getAllItems() const
{
    QList<QTableWidgetItem*> items;
    for (int i = 0; i < m_ui->itemsTable->rowCount(); ++i) {
        auto item = m_ui->itemsTable->item(i, 0);
        items.append(item);
    }
    return items;
}

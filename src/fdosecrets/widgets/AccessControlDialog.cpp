/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "AccessControlDialog.h"
#include "ui_AccessControlDialog.h"

#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/widgets/RowButtonHelper.h"

#include "core/Entry.h"
#include "gui/Icons.h"

#include <QWindow>

#include <functional>

AccessControlDialog::AccessControlDialog(QWindow* parent,
                                         const QList<Entry*>& entries,
                                         const QString& app,
                                         const FdoSecrets::PeerInfo& info,
                                         AuthOptions authOptions)
    : m_ui(new Ui::AccessControlDialog())
    , m_rememberCheck()
    , m_model(new EntryModel(entries))
    , m_decisions()
{
    if (parent) {
        // Force the creation of the QWindow, without this windowHandle() will return nullptr
        winId();
        auto window = windowHandle();
        Q_ASSERT(window);
        window->setTransientParent(parent);
    }
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    connect(this, &QDialog::finished, this, &AccessControlDialog::dialogFinished);

    m_ui->setupUi(this);
    m_ui->appLabel->setText(m_ui->appLabel->text().arg(app));

    // items table
    connect(m_ui->itemsTable, &QTableView::clicked, m_model.data(), &EntryModel::toggleCheckState);
    m_ui->itemsTable->setModel(m_model.data());
    installWidgetItemDelegate<DenyButton>(m_ui->itemsTable, 2, [this](QWidget* p, const QModelIndex& idx) {
        auto btn = new DenyButton(p, idx);
        connect(btn, &DenyButton::clicked, this, &AccessControlDialog::denyEntryClicked);
        return btn;
    });
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_ui->itemsTable->resizeColumnsToContents();

    // the info widget
    m_ui->exePathWarn->setMessageType(MessageWidget::Warning);
    m_ui->exePathWarn->hide();

    setupDetails(info);

    // the button box
    QString detailsButtonText = tr("Details");
    auto detailsButton =
        m_ui->buttonBox->addButton(detailsButtonText + QStringLiteral(" >>"), QDialogButtonBox::HelpRole);
    detailsButton->setCheckable(true);

    QString tooltip = QStringLiteral("<p align='justify'>%1</p>")
                          .arg(tr("Your decision will be remembered for the duration while both the requesting client "
                                  "AND KeePassXC are running."));

    m_rememberCheck = new QCheckBox(tr("Remember"), this);
    m_rememberCheck->setObjectName("rememberCheck"); // for testing
    m_rememberCheck->setChecked(true);
    m_ui->buttonBox->addButton(m_rememberCheck, QDialogButtonBox::ActionRole);
    m_rememberCheck->setToolTip(tooltip);

    auto allowButton = m_ui->buttonBox->addButton(tr("Allow Selected"), QDialogButtonBox::AcceptRole);
    allowButton->setDefault(true);

    auto cancelButton = m_ui->buttonBox->addButton(tr("Deny All && Future"), QDialogButtonBox::RejectRole);
    cancelButton->setToolTip(tooltip);

    auto allowAllButton = m_ui->buttonBox->addButton(tr("Allow All && &Future"), QDialogButtonBox::AcceptRole);
    allowAllButton->setToolTip(tooltip);

    connect(cancelButton, &QPushButton::clicked, this, [this]() { done(DenyAll); });
    connect(allowButton, &QPushButton::clicked, this, [this]() { done(AllowSelected); });
    connect(allowAllButton, &QPushButton::clicked, this, [this]() { done(AllowAll); });
    connect(detailsButton, &QPushButton::clicked, this, [=](bool checked) {
        m_ui->detailsContainer->setVisible(checked);
        if (checked) {
            detailsButton->setText(detailsButtonText + QStringLiteral(" <<"));
        } else {
            detailsButton->setText(detailsButtonText + QStringLiteral(" >>"));
        }
        adjustSize();
    });

    // tune the UI according to options
    if (!authOptions.testFlag(AuthOption::Remember)) {
        m_rememberCheck->hide();
        m_rememberCheck->setChecked(false);
    }
    if (!authOptions.testFlag(AuthOption::PerEntryDeny)) {
        m_ui->itemsTable->horizontalHeader()->setSectionHidden(2, true);
    }

    // show warning and details if not valid
    if (!info.valid) {
        m_ui->exePathWarn->show();
        detailsButton->click();
    }

    // adjust size after we initialize the button box
    adjustSize();

    allowButton->setFocus();
}

AccessControlDialog::~AccessControlDialog() = default;

void AccessControlDialog::setupDetails(const FdoSecrets::PeerInfo& info)
{
    QTreeWidgetItem* item = nullptr;
    for (auto it = info.hierarchy.crbegin(); it != info.hierarchy.crend(); ++it) {
        QStringList columns = {
            it->name,
            QString::number(it->pid),
            it->exePath,
            it->command,
        };
        if (item) {
            item = new QTreeWidgetItem(item, columns);
        } else {
            item = new QTreeWidgetItem(m_ui->procTree, columns);
        }
    }
    m_ui->procTree->expandAll();
    m_ui->procTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->procTree->scrollToBottom();
    m_ui->detailsContainer->hide();
}

void AccessControlDialog::denyEntryClicked(Entry* entry, const QModelIndex& index)
{
    m_decisions.insert(entry, AuthDecision::Denied);
    m_model->removeRow(index.row());
    if (m_model->rowCount({}) == 0) {
        reject();
    }
}

void AccessControlDialog::dialogFinished(int result)
{
    auto decision = AuthDecision::Undecided;
    auto futureDecision = AuthDecision::Undecided;
    switch (result) {
    case AllowSelected:
        decision = m_rememberCheck->isChecked() ? AuthDecision::Allowed : AuthDecision::AllowedOnce;
        break;
    case AllowAll:
        decision = AuthDecision::Allowed;
        futureDecision = AuthDecision::Allowed;
        break;
    case DenyAll:
        decision = AuthDecision::Denied;
        futureDecision = AuthDecision::Denied;
        break;
    case Rejected:
    default:
        break;
    }

    for (int row = 0; row != m_model->rowCount({}); ++row) {
        auto entry = m_model->data(m_model->index(row, 2), Qt::EditRole).value<Entry*>();
        auto selected = m_model->data(m_model->index(row, 0), Qt::CheckStateRole).value<Qt::CheckState>();
        Q_ASSERT(entry);

        auto undecided = result == AllowSelected && !selected;
        m_decisions.insert(entry, undecided ? AuthDecision::Undecided : decision);
    }

    emit finished(m_decisions, futureDecision);
}

QHash<Entry*, AuthDecision> AccessControlDialog::decisions() const
{
    return m_decisions;
}

AccessControlDialog::EntryModel::EntryModel(QList<Entry*> entries, QObject* parent)
    : QAbstractTableModel(parent)
    , m_entries(std::move(entries))
    , m_selected(QSet<Entry*>::fromList(m_entries))
{
}

int AccessControlDialog::EntryModel::rowCount(const QModelIndex& parent) const
{
    return isValid(parent) ? 0 : m_entries.count();
}

int AccessControlDialog::EntryModel::columnCount(const QModelIndex& parent) const
{
    return isValid(parent) ? 0 : 3;
}

bool AccessControlDialog::EntryModel::isValid(const QModelIndex& index) const
{
    return index.isValid() && index.row() < rowCount({}) && index.column() < columnCount({});
}

void AccessControlDialog::EntryModel::toggleCheckState(const QModelIndex& index)
{
    if (!isValid(index)) {
        return;
    }
    auto entry = m_entries.at(index.row());
    // click anywhere in the row to check/uncheck the item
    auto it = m_selected.find(entry);
    if (it == m_selected.end()) {
        m_selected.insert(entry);
    } else {
        m_selected.erase(it);
    }
    auto rowIdx = index.sibling(index.row(), 0);
    emit dataChanged(rowIdx, rowIdx, {Qt::CheckStateRole});
}

QVariant AccessControlDialog::EntryModel::data(const QModelIndex& index, int role) const
{
    if (!isValid(index)) {
        return {};
    }
    auto entry = m_entries.at(index.row());

    switch (index.column()) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return entry->title();
        case Qt::DecorationRole:
            return Icons::entryIconPixmap(entry);
        case Qt::CheckStateRole:
            return QVariant::fromValue(m_selected.contains(entry) ? Qt::Checked : Qt::Unchecked);
        default:
            return {};
        }
    case 1:
        switch (role) {
        case Qt::DisplayRole:
            return entry->username();
        default:
            return {};
        }
    case 2:
        switch (role) {
        case Qt::EditRole:
            return QVariant::fromValue(entry);
        default:
            return {};
        }
    default:
        return {};
    }
}

bool AccessControlDialog::EntryModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    while (count--) {
        m_entries.removeAt(row);
    }
    endRemoveRows();
    return true;
}

AccessControlDialog::DenyButton::DenyButton(QWidget* p, const QModelIndex& idx)
    : QPushButton(p)
    , m_index(idx)
    , m_entry()
{
    setText(tr("Deny for this program"));
    connect(this, &QPushButton::clicked, [this]() { emit clicked(entry(), m_index); });
}

void AccessControlDialog::DenyButton::setEntry(Entry* e)
{
    m_entry = e;
}

Entry* AccessControlDialog::DenyButton::entry() const
{
    return m_entry;
}

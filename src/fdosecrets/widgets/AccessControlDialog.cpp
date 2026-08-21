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

#include "fdosecrets/ClientAuth.h"
#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/widgets/RowButtonHelper.h"

#include "core/Entry.h"
#include "core/Global.h"
#include "gui/Icons.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QTreeWidgetItem>
#include <QWindow>

#include <functional>

AccessControlDialog::AccessControlDialog(QWindow* parent,
                                         const QList<Entry*>& entries,
                                         const QString& app,
                                         const FdoSecrets::PeerInfo& info,
                                         AuthOptions authOptions,
                                         const FingerprintChangeInfo& fingerprintChange)
    : m_ui(new Ui::AccessControlDialog())
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
    // show the requested entries without scrolling, within reason
    const auto itemRows = qMin(m_model->rowCount({}), 5);
    if (itemRows > 0) {
        m_ui->itemsTable->setMinimumHeight(m_ui->itemsTable->verticalHeader()->defaultSectionSize() * itemRows
                                           + 2 * m_ui->itemsTable->frameWidth());
    }

    // anything that should make the user look twice goes into one warning widget
    m_ui->warningWidget->setMessageType(MessageWidget::Warning);
    // without wrapping, a long warning decides how wide the dialog is
    m_ui->warningWidget->setWordWrap(true);
    m_ui->warningWidget->hide();

    setupDetails(info, fingerprintChange);

    QStringList warnings;
    if (!info.valid) {
        warnings << tr("Non-existing/inaccessible executable path. Please double-check the client is legit.");
    }
    if (fingerprintChange.isChanged()) {
        warnings << tr("The executable of this client has changed since access was authorized on %1. "
                       "The changed processes are marked in the details below.")
                        .arg(fingerprintChange.authorizedOn.toLocalTime().toString(Qt::ISODate));
    }
    if (FdoSecrets::isGenericClient(info.exePath())) {
        warnings << tr("%1 acts for whatever invokes it, so a stored decision would authorize every use of it. "
                       "Storing one is strongly discouraged.")
                        .arg(QFileInfo(info.exePath()).fileName());
    }
    if (warnings.size() == 1) {
        m_ui->warningWidget->setText(warnings.first());
        m_ui->warningWidget->show();
    } else if (!warnings.isEmpty()) {
        QStringList escaped;
        for (const auto& warning : asConst(warnings)) {
            escaped << warning.toHtmlEscaped();
        }
        m_ui->warningWidget->setText(
            QStringLiteral("<ul><li>%1</li></ul>").arg(escaped.join(QStringLiteral("</li><li>"))));
        m_ui->warningWidget->show();
    }

    // the button row: details on the left, the scope in the middle, the
    // verdict on the right. The details button is placed by hand, since a
    // QDialogButtonBox would put it next to the scope options.
    QString detailsButtonText = tr("Details");
    auto detailsButton = new QPushButton(detailsButtonText + QStringLiteral(" >>"), this);
    detailsButton->setCheckable(true);
    detailsButton->setAutoDefault(false);
    m_ui->horizontalLayout->insertWidget(0, detailsButton);

    // the verdict says what happens to the entries above, the memory scope says
    // how long it lasts; keeping them apart leaves no combination without a
    // meaning, and a client wide decision is by nature one that is remembered
    auto scopeWidget = new QWidget(this);
    scopeWidget->setObjectName("scopeWidget");
    auto scopeLayout = new QHBoxLayout(scopeWidget);
    scopeLayout->setContentsMargins(0, 0, 0, 0);

    const auto addScopeOption =
        [this, scopeWidget, scopeLayout](const QString& text, const char* name, const QString& tooltip) {
            auto button = new QRadioButton(text, scopeWidget);
            button->setObjectName(QString::fromLatin1(name)); // for testing
            button->setToolTip(QStringLiteral("<p align='justify'>%1</p>").arg(tooltip));
            connect(button, &QRadioButton::toggled, this, [this]() { setMatchColumnEnabled(scope() != Scope::Once); });
            scopeLayout->addWidget(button);
            return button;
        };
    m_scopeOnce = addScopeOption(tr("&Once"), "scopeOnce", tr("The decision applies to this request only."));
    m_scopeSelected = addScopeOption(
        tr("&Selected"),
        "scopeSelected",
        tr("The decision is stored for the entries listed above and applies to this client until removed, even "
           "across restarts. Stored decisions can be managed in the database's Secret Service settings."));
    m_scopeFuture = addScopeOption(tr("Selected + &future"),
                                   "scopeFuture",
                                   tr("As above, and the decision additionally covers every other entry this client "
                                      "asks for, including entries created later."));
    // a QDialogButtonBox only takes buttons, so the options sit next to it.
    // Extra width goes between the details button and the scope, keeping the
    // scope with the verdict it qualifies.
    m_ui->horizontalLayout->insertStretch(1);
    m_ui->horizontalLayout->insertWidget(2, scopeWidget);

    // storing a rule for a suspicious client needs an explicit opt-in
    if (warnings.isEmpty()) {
        m_scopeSelected->setChecked(true);
    } else {
        m_scopeOnce->setChecked(true);
    }
    setMatchColumnEnabled(scope() != Scope::Once);

    auto allowButton = m_ui->buttonBox->addButton(tr("&Allow"), QDialogButtonBox::AcceptRole);
    allowButton->setDefault(true);
    auto denyButton = m_ui->buttonBox->addButton(tr("&Deny"), QDialogButtonBox::RejectRole);

    connect(allowButton, &QPushButton::clicked, this, [this]() { done(Allow); });
    connect(denyButton, &QPushButton::clicked, this, [this]() { done(Deny); });
    connect(detailsButton, &QPushButton::clicked, this, [this, detailsButton, detailsButtonText](bool checked) {
        m_ui->detailsContainer->setVisible(checked);
        if (checked) {
            detailsButton->setText(detailsButtonText + QStringLiteral(" <<"));
        } else {
            detailsButton->setText(detailsButtonText + QStringLiteral(" >>"));
        }
        adjustSize();
    });

    // tune the UI according to options
    if (!authOptions.testFlag(AuthOption::Persist)) {
        scopeWidget->hide();
        m_scopeOnce->setChecked(true);
    }
    if (!authOptions.testFlag(AuthOption::PerEntryDeny)) {
        m_ui->itemsTable->horizontalHeader()->setSectionHidden(2, true);
    }

    // every warning refers to the process details, so start with them visible
    if (!warnings.isEmpty()) {
        detailsButton->click();
    }

    // adjust size after we initialize the button box
    adjustSize();

    allowButton->setFocus();
}

AccessControlDialog::~AccessControlDialog() = default;

void AccessControlDialog::setupDetails(const FdoSecrets::PeerInfo& info, const FingerprintChangeInfo& fingerprintChange)
{
    QTreeWidgetItem* item = nullptr;
    for (auto depth = static_cast<int>(info.hierarchy.size()) - 1; depth >= 0; --depth) {
        const auto& proc = info.hierarchy.at(depth);
        QStringList columns(ColumnCommand + 1);
        columns[ColumnName] = proc.name;
        columns[ColumnPid] = QString::number(proc.pid);
        columns[ColumnExe] = proc.exePath;
        columns[ColumnCommand] = proc.command;
        if (item) {
            item = new QTreeWidgetItem(item, columns);
        } else {
            item = new QTreeWidgetItem(m_ui->procTree, columns);
        }
        m_procItems.insert(depth, item);

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        // only the calling process is proposed: picking ancestors is a
        // judgement about what they identify, which nothing here can make
        item->setCheckState(ColumnMatch, depth == 0 ? Qt::Checked : Qt::Unchecked);
        if (proc.exePath.isEmpty()) {
            // the executable content may still be hashable via /proc/PID/exe
            item->setToolTip(ColumnMatch,
                             tr("A remembered rule will match this process by its executable content; "
                                "the executable path is not known."));
        } else {
            item->setToolTip(ColumnMatch,
                             tr("A remembered rule will match this process by executable path and content."));
        }

        if (fingerprintChange.mismatchedDepths.contains(depth)) {
            item->setIcon(ColumnExe, icons()->icon(QStringLiteral("dialog-warning")));
            item->setToolTip(ColumnExe,
                             tr("The content of this executable no longer matches the hash recorded when access "
                                "was authorized."));
        }
    }
    m_ui->procTree->expandAll();
    m_ui->procTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->procTree->scrollToBottom();
    // show the whole hierarchy without scrolling for the usual process chains,
    // in both directions: a horizontal scrollbar costs a row of height and
    // pushes the tree into scrolling vertically as well
    if (item) {
        const auto rows = qMin(static_cast<int>(info.hierarchy.size()), 8);
        m_ui->procTree->setMinimumHeight(m_ui->procTree->sizeHintForRow(0) * rows
                                         + m_ui->procTree->header()->sizeHint().height()
                                         + 2 * m_ui->procTree->frameWidth());
        int width = 2 * m_ui->procTree->frameWidth();
        for (int column = 0; column != m_ui->procTree->columnCount(); ++column) {
            width += m_ui->procTree->columnWidth(column);
        }
        // a pathological command line must not decide how wide the dialog is
        m_ui->procTree->setMinimumWidth(qMin(width, 900));
    }
    m_ui->detailsContainer->hide();
}

void AccessControlDialog::setMatchColumnEnabled(bool enabled)
{
    for (auto item : asConst(m_procItems)) {
        if (enabled) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        } else {
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        }
    }
}

QSet<int> AccessControlDialog::selectedMatchDepths() const
{
    QSet<int> depths;
    for (auto it = m_procItems.constBegin(); it != m_procItems.constEnd(); ++it) {
        if (it.value()->checkState(ColumnMatch) == Qt::Checked) {
            depths.insert(it.key());
        }
    }
    return depths;
}

void AccessControlDialog::denyEntryClicked(const QUuid& uuid, const QModelIndex& index)
{
    // only the intent is recorded here; how long the denial lasts is read off
    // the memory scope when the dialog concludes, like everything else, so the
    // outcome does not depend on the order of clicks
    m_deniedEntries.insert(uuid);
    m_model->removeRow(index.row());
    if (m_model->rowCount({}) == 0) {
        // every row was explicitly denied: conclude as an allow with nothing
        // left to allow, so the denials keep the memory scope the user chose
        // instead of being downgraded by a plain reject
        done(Allow);
    }
}

AccessControlDialog::Scope AccessControlDialog::scope() const
{
    if (m_scopeFuture && m_scopeFuture->isChecked()) {
        return Scope::Future;
    }
    if (m_scopeSelected && m_scopeSelected->isChecked()) {
        return Scope::Selected;
    }
    return Scope::Once;
}

void AccessControlDialog::dialogFinished(int result)
{
    // escaping means nothing gets remembered; the request just fails
    const auto chosen = result == Rejected ? Scope::Once : scope();
    const auto persist = chosen != Scope::Once;
    auto decision = AuthDecision::Undecided;
    auto futureDecision = AuthDecision::Undecided;
    switch (result) {
    case Allow:
        decision = persist ? AuthDecision::Allowed : AuthDecision::AllowedOnce;
        futureDecision = chosen == Scope::Future ? AuthDecision::Allowed : AuthDecision::Undecided;
        break;
    case Deny:
        decision = persist ? AuthDecision::Denied : AuthDecision::DeniedOnce;
        futureDecision = chosen == Scope::Future ? AuthDecision::Denied : AuthDecision::Undecided;
        break;
    case Rejected:
    default:
        break;
    }

    // entries denied row by row keep that denial: stored alongside a future
    // scope they outrank the catch-all, which is what makes "allow everything
    // except this one" hold for entries the client asks for again later
    for (const auto& uuid : asConst(m_deniedEntries)) {
        m_decisions.insert(uuid, persist ? AuthDecision::Denied : AuthDecision::DeniedOnce);
    }
    for (int row = 0; row != m_model->rowCount({}); ++row) {
        auto uuid = m_model->data(m_model->index(row, 2), Qt::EditRole).value<QUuid>();
        auto selected = m_model->data(m_model->index(row, 0), Qt::CheckStateRole).value<Qt::CheckState>();
        Q_ASSERT(!uuid.isNull());

        // unchecked rows are left undecided by an allow, and denied by a deny
        auto undecided = result == Allow && !selected;
        m_decisions.insert(uuid, undecided ? AuthDecision::Undecided : decision);
    }

    emit finished(m_decisions, futureDecision, persist, selectedMatchDepths());
}

QHash<QUuid, AuthDecision> AccessControlDialog::decisions() const
{
    return m_decisions;
}

AccessControlDialog::EntryModel::EntryModel(const QList<Entry*>& entries, QObject* parent)
    : QAbstractTableModel(parent)
{
    m_entries.reserve(entries.size());
    for (const auto& entry : entries) {
        m_entries.append({entry->uuid(), entry->title(), entry->username(), Icons::entryIconPixmap(entry)});
        m_selected.insert(entry->uuid());
    }
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
    const auto& entry = m_entries.at(index.row());
    // click anywhere in the row to check/uncheck the item
    auto it = m_selected.find(entry.uuid);
    if (it == m_selected.end()) {
        m_selected.insert(entry.uuid);
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
    const auto& entry = m_entries.at(index.row());

    switch (index.column()) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return entry.title;
        case Qt::DecorationRole:
            return entry.icon;
        case Qt::CheckStateRole:
            return QVariant::fromValue(m_selected.contains(entry.uuid) ? Qt::Checked : Qt::Unchecked);
        default:
            return {};
        }
    case 1:
        switch (role) {
        case Qt::DisplayRole:
            return entry.username;
        default:
            return {};
        }
    case 2:
        switch (role) {
        case Qt::EditRole:
            return QVariant::fromValue(entry.uuid);
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
    , m_uuid()
{
    setText(tr("Deny for this program"));
    connect(this, &QPushButton::clicked, [this]() { emit clicked(uuid(), m_index); });
}

void AccessControlDialog::DenyButton::setUuid(const QUuid& uuid)
{
    m_uuid = uuid;
}

QUuid AccessControlDialog::DenyButton::uuid() const
{
    return m_uuid;
}

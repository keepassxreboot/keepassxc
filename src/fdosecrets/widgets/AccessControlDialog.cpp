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

#include "core/Clock.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "gui/Icons.h"
#include "gui/styles/StateColorPalette.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QStyle>
#include <QTreeWidgetItem>
#include <QWindow>

AccessControlDialog::AccessControlDialog(QWindow* parent,
                                         const QList<Entry*>& entries,
                                         const QString& databaseName,
                                         const QString& clientName,
                                         const FdoSecrets::PeerInfo& info,
                                         AuthOptions authOptions,
                                         const FingerprintChangeInfo& fingerprintChange,
                                         const KnownClientInfo& known)
    : m_ui(new Ui::AccessControlDialog())
    , m_model(new EntryModel(entries))
    , m_decisions()
    , m_known(known)
    , m_fingerprintChanged(fingerprintChange.isChanged())
    , m_canPersist(authOptions.testFlag(AuthOption::Persist))
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
    m_ui->appLabel->setText(tr("Requested entries in “%1”:").arg(databaseName));

    const auto hasWarning = setupClient(clientName, info);
    setupEntries();
    setupDetails(info, fingerprintChange, known);
    setupRemember(info, hasWarning);

    auto allowButton = m_ui->buttonBox->addButton(tr("&Allow"), QDialogButtonBox::AcceptRole);
    allowButton->setDefault(true);
    auto denyButton = m_ui->buttonBox->addButton(tr("&Deny"), QDialogButtonBox::RejectRole);
    connect(allowButton, &QPushButton::clicked, this, [this]() { done(Allow); });
    connect(denyButton, &QPushButton::clicked, this, [this]() { done(Deny); });

    const auto detailsButtonText = tr("Details");
    m_ui->detailsButton->setText(detailsButtonText + QStringLiteral(" >>"));
    connect(m_ui->detailsButton, &QPushButton::toggled, this, [this, detailsButtonText](bool checked) {
        m_ui->procTree->setVisible(checked);
        m_ui->detailsButton->setText(detailsButtonText + (checked ? QStringLiteral(" <<") : QStringLiteral(" >>")));
        fitToContents();
    });
    updateRecognition();

    // every warning refers to the process details, so start with them visible
    if (hasWarning) {
        m_ui->detailsButton->setChecked(true);
    }

    allowButton->setFocus();
}

AccessControlDialog::~AccessControlDialog() = default;

void AccessControlDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // Qt sizes a window to its layout just before it is first shown. The item
    // views only know the size of their contents once they have been laid
    // out, which that comes too early for, so fit again now.
    fitToContents();
}

void AccessControlDialog::fitToContents()
{
    // nothing to fit before the dialog is shown, see showEvent()
    if (!isVisible()) {
        return;
    }
    // Showing or hiding a child only posts the layout request that drops the
    // cached size hints. Activating the layout now recomputes them, nested
    // layouts included; adjustSize() then fits the window, within the screen.
    layout()->invalidate();
    layout()->activate();
    adjustSize();
}

bool AccessControlDialog::setupClient(const QString& clientName, const FdoSecrets::PeerInfo& info)
{
    // The block states who is asking and everything about that client as it
    // is now: a name, where it runs from, and one row per further fact.
    const auto exePath = info.exePath();
    const auto procName = info.hierarchy.isEmpty() ? QString() : info.hierarchy.front().name;
    m_ui->clientLabel->setRawText(procName.isEmpty() ? clientName : procName);
    m_ui->pathLabel->setRawText(exePath.isEmpty() ? tr("PID %1").arg(info.pid)
                                                  : tr("%1 · PID %2").arg(exePath).arg(info.pid));

    // palette roles cannot be chosen in the .ui file: the block is a panel on
    // the view background like the lists below it, with the path toned down
    m_ui->clientFrame->setBackgroundRole(QPalette::Base);
    m_ui->pathLabel->setForegroundRole(QPalette::PlaceholderText);

    const auto warningColor = StateColorPalette().color(StateColorPalette::Warning);
    const auto since = Clock::toString(m_known.since.toLocalTime());
    // records are named after the executable by default, which the block
    // already shows; only a name given by the user says something more
    const auto recordName =
        m_known.name == exePath || m_known.name == clientName
            ? QString()
            : fontMetrics().elidedText(m_known.name, Qt::ElideMiddle, 30 * fontMetrics().averageCharWidth());

    // the usual case, a client nothing is stored for, needs no mark
    m_ui->stateLabel->setVisible(m_known.isKnown());
    m_ui->knownFact->setVisible(m_known.isKnown() && !m_fingerprintChanged);
    m_ui->changedFact->setVisible(m_fingerprintChanged);
    if (m_fingerprintChanged) {
        m_ui->stateLabel->setText(tr("Changed", "state of a client whose executable changed"));
        m_ui->stateLabel->setFill(warningColor);
        m_ui->changedFact->setText(recordName.isEmpty()
                                       ? tr("Changed executable: differs from the one remembered on %1.").arg(since)
                                       : tr("Changed executable: differs from the one remembered as “%1” on %2.")
                                             .arg(recordName.toHtmlEscaped(), since));
    } else if (m_known.isKnown()) {
        m_ui->stateLabel->setText(tr("Known", "state of a client that has stored decisions"));
        m_ui->knownFact->setText(
            recordName.isEmpty()
                ? tr("Known client: remembered since %1.").arg(since)
                : tr("Known client: remembered as “%1” since %2.").arg(recordName.toHtmlEscaped(), since));
    }
    m_ui->invalidPathFact->setVisible(!info.valid);
    m_ui->genericFact->setVisible(FdoSecrets::isGenericClient(exePath));

    // a block with something to look at twice is marked along its edge, in
    // the warning color of the theme
    const auto hasWarning = m_fingerprintChanged || !info.valid || FdoSecrets::isGenericClient(exePath);
    auto accentPalette = m_ui->clientAccent->palette();
    accentPalette.setColor(QPalette::Window, warningColor);
    m_ui->clientAccent->setPalette(accentPalette);
    m_ui->clientAccent->setBackgroundRole(QPalette::Window);
    m_ui->clientAccent->setVisible(hasWarning);

    return hasWarning;
}

void AccessControlDialog::setupEntries()
{
    connect(m_ui->itemsTable, &QTableView::clicked, m_model.data(), &EntryModel::toggleCheckState);
    m_ui->itemsTable->setModel(m_model.data());
    // the titles get the width they need and the usernames the rest
    m_ui->itemsTable->resizeColumnsToContents();

    // the future entries row is the table's footer: it stays in view below
    // the entries however many there are
    m_ui->futureFooter->setBackgroundRole(QPalette::Base);
    connect(m_ui->futureCheck, &QCheckBox::toggled, this, &AccessControlDialog::coversFutureChanged);
    // a decision about future entries can only be a stored one
    if (m_canPersist) {
        m_ui->itemsTable->setFooter(m_ui->futureFooter);
    } else {
        m_ui->futureFooter->hide();
    }
}

void AccessControlDialog::setupRemember(const FdoSecrets::PeerInfo& info, bool hasWarning)
{
    // storing a decision for a suspicious client needs an explicit opt-in
    m_ui->rememberCheck->setChecked(m_canPersist && !hasWarning);
    m_ui->rememberCheck->setVisible(m_canPersist);
    connect(m_ui->rememberCheck, &QCheckBox::toggled, this, &AccessControlDialog::updateRecognition);
    connect(m_ui->procTree, &QTreeWidget::itemChanged, this, &AccessControlDialog::updateRecognition);

    // how a new client will be recognized qualifies Remember itself, on the
    // same line; its link opens the details, where the match is edited
    m_ui->recognitionNote->setForegroundRole(QPalette::PlaceholderText);
    connect(m_ui->recognitionNote, &QLabel::linkActivated, this, [this]() { m_ui->detailsButton->setChecked(true); });

    // what else remembering would do is listed below it while it is checked,
    // in line with its label, which is as far in as the style puts it
    m_ui->consequencesLayout->setContentsMargins(
        style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, this)
            + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this),
        0,
        0,
        0);
    const auto exePath = info.exePath();
    m_ui->genericNote->setText(
        tr("General purpose tool: every use of %1 by any program gets authorized. Strongly discouraged.")
            .arg(QStringLiteral("<b>%1</b>").arg(QFileInfo(exePath).fileName().toHtmlEscaped())));
    m_genericClient = FdoSecrets::isGenericClient(exePath);
}

void AccessControlDialog::setupDetails(const FdoSecrets::PeerInfo& info,
                                       const FingerprintChangeInfo& fingerprintChange,
                                       const KnownClientInfo& known)
{
    // what the stored rule of a known client compares, per hierarchy depth
    QHash<int, QStringList> storedKinds;
    for (const auto& cond : known.rule.conditions) {
        switch (cond.kind) {
        case FdoSecrets::RuleCondition::Kind::Path:
            storedKinds[cond.depth] << tr("path", "what a rule matches a process on");
            break;
        case FdoSecrets::RuleCondition::Kind::Name:
            storedKinds[cond.depth] << tr("name", "what a rule matches a process on");
            break;
        case FdoSecrets::RuleCondition::Kind::Hash:
            storedKinds[cond.depth] << tr("content", "what a rule matches a process on");
            break;
        }
    }

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
        item->setToolTip(ColumnExe, proc.exePath);
        item->setToolTip(ColumnCommand, proc.command);

        if (known.isKnown()) {
            // the match belongs to the stored record and is only shown here
            item->setText(ColumnMatch, storedKinds.value(depth).join(QStringLiteral(" + ")));
            item->setToolTip(ColumnMatch,
                             tr("What this client's stored rule compares for this process. Rules are edited in "
                                "the database's Secret Service settings."));
        } else {
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
        }

        if (fingerprintChange.mismatchedDepths.contains(depth)) {
            item->setIcon(ColumnExe, icons()->icon(QStringLiteral("dialog-warning")));
            item->setToolTip(ColumnExe,
                             tr("The content of this executable no longer matches the hash recorded when access "
                                "was authorized."));
        }
    }
    m_ui->procTree->expandAll();
    // Every column as wide as its contents, which is also what the tree then
    // asks for as its own size. A view sized by its contents needs contents
    // of bounded size: command lines run to thousands of characters, so a
    // column has a limit, with longer cells elided and shown in full in
    // their tool tip.
    m_ui->procTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->procTree->header()->setMaximumSectionSize(60 * m_ui->procTree->fontMetrics().averageCharWidth());
    m_ui->procTree->hide();
}

bool AccessControlDialog::remembering() const
{
    return m_canPersist && m_ui->rememberCheck->isChecked();
}

void AccessControlDialog::coversFutureChanged(bool coversFuture)
{
    m_model->setCoversFuture(coversFuture);
    // a decision about entries not asked for yet only exists stored
    if (coversFuture) {
        m_rememberBeforeFuture = m_ui->rememberCheck->isChecked();
        m_ui->rememberCheck->setChecked(true);
    } else {
        m_ui->rememberCheck->setChecked(m_rememberBeforeFuture);
    }
    m_ui->rememberCheck->setEnabled(!coversFuture);
}

void AccessControlDialog::updateRecognition()
{
    updateRememberNotes();
    fitToContents();
}

void AccessControlDialog::updateRememberNotes()
{
    const auto remember = remembering();
    m_ui->acceptChangeNote->setVisible(remember && m_fingerprintChanged);
    m_ui->genericNote->setVisible(remember && m_genericClient);
    if (m_known.isKnown()) {
        m_ui->recognitionNote->hide();
        return;
    }

    // without remembering nothing is stored, so nothing is matched on
    m_ui->procTree->setColumnHidden(ColumnMatch, !remember);
    m_ui->recognitionNote->setVisible(remember);

    const auto depths = selectedMatchDepths();
    const auto parents = static_cast<int>(depths.size()) - (depths.contains(0) ? 1 : 0);
    QString how;
    if (depths.isEmpty()) {
        how = tr("cannot be stored, no process is selected to match");
    } else if (parents == 0) {
        how = tr("recognized by its executable");
    } else if (depths.contains(0)) {
        how = tr("recognized by its executable and %n parent process(es)", nullptr, parents);
    } else {
        how = tr("recognized by %n parent process(es)", nullptr, parents);
    }
    m_ui->recognitionNote->setText(QStringLiteral("%1 · <a href=\"#details\">%2</a>").arg(how, tr("Change…")));
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

void AccessControlDialog::dialogFinished(int result)
{
    // escaping means nothing gets remembered; the request just fails
    const auto persist = result != Rejected && remembering();
    auto decision = AuthDecision::Undecided;
    switch (result) {
    case Allow:
        decision = persist ? AuthDecision::Allowed : AuthDecision::AllowedOnce;
        break;
    case Deny:
        decision = persist ? AuthDecision::Denied : AuthDecision::DeniedOnce;
        break;
    case Rejected:
    default:
        break;
    }
    const auto futureDecision = persist && m_ui->futureCheck->isChecked() ? decision : AuthDecision::Undecided;

    // the verdict applies to the checked entries. An unchecked one is left
    // undecided whatever the verdict: it is not unlocked now, and the client
    // is asked about it again next time.
    const auto selection = m_model->selection();
    for (auto it = selection.constBegin(); it != selection.constEnd(); ++it) {
        m_decisions.insert(it.key(), it.value() ? decision : AuthDecision::Undecided);
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
    m_entryDataList.reserve(entries.size());
    for (const auto& entry : entries) {
        m_entryDataList.append({entry->uuid(), entry->title(), entry->username(), Icons::entryIconPixmap(entry)});
        m_selected.insert(entry->uuid());
    }
}

int AccessControlDialog::EntryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entryDataList.count();
}

int AccessControlDialog::EntryModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 2;
}

bool AccessControlDialog::EntryModel::isValid(const QModelIndex& index) const
{
    return index.isValid() && index.row() < rowCount({}) && index.column() < columnCount({});
}

void AccessControlDialog::EntryModel::setCoversFuture(bool coversFuture)
{
    if (m_coversFuture == coversFuture || m_entryDataList.isEmpty()) {
        m_coversFuture = coversFuture;
        return;
    }
    m_coversFuture = coversFuture;
    // every row changes how it is drawn
    emit dataChanged(index(0, 0), index(rowCount({}) - 1, columnCount({}) - 1));
}

QHash<QUuid, bool> AccessControlDialog::EntryModel::selection() const
{
    QHash<QUuid, bool> res;
    for (const auto& entry : m_entryDataList) {
        res.insert(entry.uuid, m_coversFuture || m_selected.contains(entry.uuid));
    }
    return res;
}

void AccessControlDialog::EntryModel::toggleCheckState(const QModelIndex& index)
{
    if (!isValid(index)) {
        return;
    }
    // click anywhere in the row to check/uncheck it, unless the decision
    // covers every entry anyway
    if (m_coversFuture) {
        return;
    }
    const auto& entry = m_entryDataList.at(index.row());
    auto it = m_selected.find(entry.uuid);
    if (it == m_selected.end()) {
        m_selected.insert(entry.uuid);
    } else {
        m_selected.erase(it);
    }
    auto rowIdx = index.sibling(index.row(), 0);
    emit dataChanged(rowIdx, rowIdx, {Qt::CheckStateRole});
}

Qt::ItemFlags AccessControlDialog::EntryModel::flags(const QModelIndex& index) const
{
    auto flags = QAbstractTableModel::flags(index);
    if (isValid(index) && m_coversFuture) {
        flags &= ~Qt::ItemIsEnabled;
    }
    return flags;
}

QVariant AccessControlDialog::EntryModel::data(const QModelIndex& index, int role) const
{
    if (!isValid(index)) {
        return {};
    }

    const auto& entry = m_entryDataList.at(index.row());
    if (role == Qt::ToolTipRole && m_coversFuture) {
        return tr("Included: the decision covers all entries, now and in the future.");
    }
    if (role == Qt::UserRole) {
        return QVariant::fromValue(entry.uuid);
    }

    switch (index.column()) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return entry.title;
        case Qt::DecorationRole:
            return entry.icon;
        case Qt::CheckStateRole:
            return QVariant::fromValue(m_coversFuture || m_selected.contains(entry.uuid) ? Qt::Checked : Qt::Unchecked);
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
    default:
        return {};
    }
}

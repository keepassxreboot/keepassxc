/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "ClientAuthModels.h"

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"

#include <QComboBox>
#include <QFileInfo>
#include <QFont>
#include <QLocale>

#include <algorithm>

namespace FdoSecrets
{
    // static constexpr still requires definition before c++17
    constexpr const char* ClientRecordsModel::ColumnNames[];

    ClientRecordsModel::ClientRecordsModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    void ClientRecordsModel::setRecords(QList<ClientRecord> records, QHash<DBusClientId, QPair<int, int>> counts)
    {
        beginResetModel();
        m_records = std::move(records);
        m_decisionCounts = std::move(counts);
        std::sort(m_records.begin(), m_records.end(), [](const ClientRecord& a, const ClientRecord& b) {
            return std::tie(a.created, a.id) < std::tie(b.created, b.id);
        });
        endResetModel();
    }

    ClientRecord ClientRecordsModel::recordAt(int row) const
    {
        if (row < 0 || row >= m_records.size()) {
            return {};
        }
        return m_records.at(row);
    }

    int ClientRecordsModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_records.size();
    }

    int ClientRecordsModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return sizeof(ColumnNames) / sizeof(ColumnNames[0]);
    }

    QVariant ClientRecordsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= columnCount({})) {
            return {};
        }
        return tr(ColumnNames[section]);
    }

    QVariant ClientRecordsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.model() != this || index.row() >= m_records.size()) {
            return {};
        }
        const auto& record = m_records.at(index.row());
        switch (index.column()) {
        case ColumnName:
            if (role == Qt::DisplayRole) {
                return record.name;
            }
            if (role == Qt::ToolTipRole) {
                return tr("Created: %1").arg(QLocale().toString(record.created.toLocalTime(), QLocale::ShortFormat));
            }
            break;
        case ColumnRules:
            if (role == Qt::DisplayRole) {
                return rulesSummary(record);
            }
            if (role == Qt::ToolTipRole) {
                return rulesToolTip(record);
            }
            break;
        case ColumnDecisions:
            if (role == Qt::DisplayRole) {
                return decisionsSummary(record);
            }
            break;
        default:
            break;
        }
        return {};
    }

    QString ClientRecordsModel::rulesSummary(const ClientRecord& record)
    {
        if (record.rules.isEmpty()) {
            return tr("never matches");
        }
        const auto& rule = record.rules.first();

        // per depth: which kinds anchor it and the most readable value
        QMap<int, QSet<RuleCondition::Kind>> kindsByDepth;
        QMap<int, QString> valueByDepth;
        for (const auto& cond : rule.conditions) {
            kindsByDepth[cond.depth].insert(cond.kind);
            auto& value = valueByDepth[cond.depth];
            if (value.isEmpty() || cond.kind != RuleCondition::Kind::Hash) {
                value = cond.kind == RuleCondition::Kind::Hash
                            ? QStringLiteral("%1:%2…").arg(cond.algo, cond.value.left(8))
                            : cond.value;
            }
        }

        const auto& innermost = kindsByDepth.first();
        QStringList kindNames;
        if (innermost.contains(RuleCondition::Kind::Path)) {
            kindNames << QStringLiteral("path");
        }
        if (innermost.contains(RuleCondition::Kind::Name)) {
            kindNames << QStringLiteral("name");
        }
        if (innermost.contains(RuleCondition::Kind::Hash)) {
            kindNames << QStringLiteral("hash");
        }
        auto summary = QStringLiteral("%1: %2").arg(kindNames.join(QLatin1Char('+')),
                                                    QStringList(valueByDepth.values()).join(QStringLiteral(" ← ")));
        if (record.rules.size() > 1) {
            summary += tr(" (+%n more rule(s))", nullptr, record.rules.size() - 1);
        }
        return summary;
    }

    QString ClientRecordsModel::rulesToolTip(const ClientRecord& record)
    {
        QStringList lines;
        for (int i = 0; i < record.rules.size(); ++i) {
            lines << tr("Rule %1:").arg(i + 1);
            for (const auto& cond : record.rules.at(i).conditions) {
                lines << QStringLiteral("    %1 %2 = %3")
                             .arg(processLabel(cond.depth), conditionKindLabel(cond.kind, cond.algo), cond.value);
            }
        }
        return lines.join(QLatin1Char('\n'));
    }

    QString ClientRecordsModel::processLabel(int depth)
    {
        switch (depth) {
        case 0:
            return tr("Caller");
        case 1:
            return tr("Parent");
        default:
            return tr("Ancestor ×%1").arg(depth);
        }
    }

    QString ClientRecordsModel::conditionKindLabel(RuleCondition::Kind kind, const QString& algo)
    {
        switch (kind) {
        case RuleCondition::Kind::Path:
            return tr("Path");
        case RuleCondition::Kind::Name:
            return tr("Name");
        case RuleCondition::Kind::Hash:
            if (algo == QStringLiteral("sha256")) {
                return QStringLiteral("SHA-256");
            }
            return algo.toUpper();
        }
        return {};
    }

    QString ClientRecordsModel::decisionsSummary(const ClientRecord& record) const
    {
        const auto counts = m_decisionCounts.value(record.id);
        QString others;
        switch (record.allEntries) {
        case AuthDecision::Allowed:
            others = tr("allow");
            break;
        case AuthDecision::Denied:
            others = tr("deny");
            break;
        default:
            others = tr("ask");
            break;
        }
        return tr("Allowed: %1, Denied: %2, Others: %3").arg(counts.first).arg(counts.second).arg(others);
    }

    QWidget* AuthDecisionDelegate::createEditor(QWidget* parent,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        auto editor = new QComboBox(parent);
        editor->addItem(tr("Allow"), static_cast<int>(AuthDecision::Allowed));
        editor->addItem(tr("Deny"), static_cast<int>(AuthDecision::Denied));
        // the editors are kept open, so nothing ever closes them to commit:
        // write the choice through as soon as it changes. Setting the initial
        // value commits the value that is already there, which the model
        // ignores.
        connect(editor, &QComboBox::currentIndexChanged, this, [this, editor]() {
            emit const_cast<AuthDecisionDelegate*>(this)->commitData(editor);
        });
        return editor;
    }

    void AuthDecisionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        auto combo = static_cast<QComboBox*>(editor);
        combo->setCurrentIndex(combo->findData(index.data(Qt::EditRole).toInt()));
    }

    void AuthDecisionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        model->setData(index, static_cast<QComboBox*>(editor)->currentData(), Qt::EditRole);
    }

    EntryClientDecisionsModel::EntryClientDecisionsModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    void EntryClientDecisionsModel::load(CustomData* customData, const Database* db)
    {
        m_customData = customData;
        m_db = db;
        beginResetModel();
        m_records = db ? loadClientRecords(db) : QList<ClientRecord>{};
        std::sort(m_records.begin(), m_records.end(), [](const ClientRecord& a, const ClientRecord& b) {
            return std::tie(a.created, a.id) < std::tie(b.created, b.id);
        });
        reload();
        endResetModel();
    }

    void EntryClientDecisionsModel::setReadOnly(bool readOnly)
    {
        m_readOnly = readOnly;
    }

    Qt::ItemFlags EntryClientDecisionsModel::flags(const QModelIndex& index) const
    {
        auto flags = QAbstractTableModel::flags(index);
        if (!m_readOnly && index.column() == ColumnAccess) {
            flags |= Qt::ItemIsEditable;
        }
        return flags;
    }

    bool EntryClientDecisionsModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || m_readOnly || index.column() != ColumnAccess || index.row() >= m_rows.size()) {
            return false;
        }
        const auto decision = static_cast<AuthDecision>(value.toInt());
        if (decision != AuthDecision::Allowed && decision != AuthDecision::Denied) {
            return false;
        }
        if (decision == m_rows.at(index.row()).second) {
            return true;
        }
        // in place: the row order does not depend on the decision, and a reset
        // would pull the ground out from under the delegate's editor
        setEntryClientDecision(m_customData, m_rows.at(index.row()).first, decision);
        m_rows[index.row()].second = decision;
        emit dataChanged(index, index);
        return true;
    }

    void EntryClientDecisionsModel::reload()
    {
        m_rows.clear();
        if (!m_customData) {
            return;
        }
        auto decisions = entryClientDecisions(m_customData);
        // known records first, in the order the settings page lists them
        for (const auto& record : asConst(m_records)) {
            const auto it = decisions.constFind(record.id);
            if (it != decisions.constEnd()) {
                m_rows.append({record.id, *it});
                decisions.erase(it);
            }
        }
        // whatever is left references a record that no longer exists
        for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
            m_rows.append({it.key(), it.value()});
        }
    }

    void EntryClientDecisionsModel::setDecision(const DBusClientId& id, AuthDecision decision)
    {
        if (!m_customData || id.isNull()) {
            return;
        }
        beginResetModel();
        setEntryClientDecision(m_customData, id, decision);
        reload();
        endResetModel();
    }

    void EntryClientDecisionsModel::removeRow(int row)
    {
        if (row < 0 || row >= m_rows.size()) {
            return;
        }
        setDecision(m_rows.at(row).first, AuthDecision::Undecided);
    }

    DBusClientId EntryClientDecisionsModel::idAt(int row) const
    {
        if (row < 0 || row >= m_rows.size()) {
            return {};
        }
        return m_rows.at(row).first;
    }

    QList<ClientRecord> EntryClientDecisionsModel::assignableRecords() const
    {
        QSet<DBusClientId> decided;
        for (const auto& row : m_rows) {
            decided.insert(row.first);
        }
        QList<ClientRecord> available;
        for (const auto& record : m_records) {
            if (!decided.contains(record.id)) {
                available.append(record);
            }
        }
        return available;
    }

    QString EntryClientDecisionsModel::recordLabel(const ClientRecord& record)
    {
        return QStringLiteral("%1 — %2").arg(record.name, ClientRecordsModel::rulesSummary(record));
    }

    int EntryClientDecisionsModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    int EntryClientDecisionsModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return 2;
    }

    QVariant EntryClientDecisionsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (section) {
        case ColumnClient:
            return tr("Client");
        case ColumnAccess:
            return tr("Access");
        default:
            return {};
        }
    }

    QVariant EntryClientDecisionsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_rows.size()) {
            return {};
        }
        const auto& row = m_rows.at(index.row());
        ClientRecord record;
        for (const auto& candidate : m_records) {
            if (candidate.id == row.first) {
                record = candidate;
                break;
            }
        }

        switch (index.column()) {
        case ColumnClient:
            if (role == Qt::DisplayRole) {
                // a record removed in the database settings leaves the decision
                // behind; name it as such rather than showing a bare uuid
                return record.isValid() ? recordLabel(record) : tr("Unknown client (removed from database)");
            }
            if (role == Qt::ToolTipRole) {
                return record.isValid() ? ClientRecordsModel::rulesToolTip(record)
                                        : row.first.toString(QUuid::WithoutBraces);
            }
            if (role == Qt::FontRole && !record.isValid()) {
                QFont font;
                font.setItalic(true);
                return font;
            }
            break;
        case ColumnAccess:
            if (role == Qt::DisplayRole) {
                return row.second == AuthDecision::Allowed ? tr("Allow") : tr("Deny");
            }
            if (role == Qt::EditRole) {
                return static_cast<int>(row.second);
            }
            break;
        default:
            break;
        }
        return {};
    }

} // namespace FdoSecrets

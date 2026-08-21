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

} // namespace FdoSecrets

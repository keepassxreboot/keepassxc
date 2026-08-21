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

#ifndef KEEPASSXC_FDOSECRETS_CLIENTAUTHMODELS_H
#define KEEPASSXC_FDOSECRETS_CLIENTAUTHMODELS_H

#include "fdosecrets/ClientAuth.h"

#include <QAbstractTableModel>
#include <QPointer>
#include <QStyledItemDelegate>

class Database;

namespace FdoSecrets
{
    /**
     * Client identity records, one row per record, as the settings page has
     * them staged. The page owns the list and hands it over again after every
     * edit; nothing is read from or written to the database here.
     */
    class ClientRecordsModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum Column
        {
            ColumnName,
            ColumnRules,
            ColumnDecisions,
        };
        // the creation time is in the name's tooltip and in the record editor:
        // a column of its own leaves the rules too little room to be readable
        static constexpr const char* ColumnNames[] = {QT_TR_NOOP("Name"),
                                                      QT_TR_NOOP("Rules"),
                                                      QT_TR_NOOP("Decisions")};

        explicit ClientRecordsModel(QObject* parent = nullptr);

        /**
         * Show @a records with the per-entry decision counts in @a counts
         * (record id → allowed/denied entry count). The page keeps both: the
         * records it is editing are not written before the settings are saved.
         */
        void setRecords(QList<ClientRecord> records, QHash<DBusClientId, QPair<int, int>> counts);

        ClientRecord recordAt(int row) const;
        const QList<ClientRecord>& records() const
        {
            return m_records;
        }

        int rowCount(const QModelIndex& parent = {}) const override;
        int columnCount(const QModelIndex& parent = {}) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;

        /**
         * One line describing the record's rules for table cells, e.g.
         * "path+hash: /usr/bin/python3 ← /usr/bin/zsh". Each depth shows its
         * anchoring path (or name, or abbreviated digest), innermost first.
         */
        static QString rulesSummary(const ClientRecord& record);
        /// Every rule with one line per condition, for tooltips.
        static QString rulesToolTip(const ClientRecord& record);
        /// "Caller", "Parent" or "Ancestor ×n" for a hierarchy depth.
        static QString processLabel(int depth);
        /// "Path", "Name", or the digest algorithm in display form ("SHA-256").
        static QString conditionKindLabel(RuleCondition::Kind kind, const QString& algo);

    private:
        QString decisionsSummary(const ClientRecord& record) const;

        QList<ClientRecord> m_records;
        /// per record: number of entries with an allow / deny decision
        QHash<DBusClientId, QPair<int, int>> m_decisionCounts;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_CLIENTAUTHMODELS_H

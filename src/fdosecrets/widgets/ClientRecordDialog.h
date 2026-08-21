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

#ifndef KEEPASSXC_FDOSECRETS_CLIENTRECORDDIALOG_H
#define KEEPASSXC_FDOSECRETS_CLIENTRECORDDIALOG_H

#include "fdosecrets/ClientAuth.h"

#include <QDialog>
#include <QPointer>

class Database;
class Entry;

namespace Ui
{
    class ClientRecordDialog;
}

namespace FdoSecrets
{
    class MatchRulesModel;
    class RecordEntryDecisionsModel;

    /**
     * Editor for one client identity record. The dialog only stages edits:
     * on accept the caller reads back record() and removedEntries() and
     * applies them to the database. Passing a record with a null id creates
     * a new record (id and creation time are assigned on accept).
     */
    class ClientRecordDialog : public QDialog
    {
        Q_OBJECT

    public:
        /**
         * @param siblings the other records as the settings page has them
         *                 staged, for the overlap warning
         */
        explicit ClientRecordDialog(QSharedPointer<Database> db,
                                    ClientRecord record,
                                    QList<ClientRecord> siblings,
                                    QWidget* parent = nullptr);
        ~ClientRecordDialog() override;

        /// The edited record. Valid only after the dialog was accepted.
        ClientRecord record() const;
        /// Entries whose decision referencing this record the user removed.
        QList<Entry*> removedEntries() const;

        void accept() override;

    private slots:
        void updateButtons();
        void updateOverlapWarning();
        void hashFromFile();

    private:
        QScopedPointer<Ui::ClientRecordDialog> m_ui;
        QSharedPointer<Database> m_db;
        ClientRecord m_record;
        /// staged records other than the edited one, for the overlap warning
        QList<ClientRecord> m_otherRecords;
        MatchRulesModel* m_rulesModel;
        RecordEntryDecisionsModel* m_decisionsModel;
    };
} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_CLIENTRECORDDIALOG_H

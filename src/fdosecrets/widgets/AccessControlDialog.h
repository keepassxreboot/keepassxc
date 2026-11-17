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

#ifndef KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H
#define KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QDialog>
#include <QHash>
#include <QPixmap>
#include <QSet>
#include <QUuid>

#include "core/Global.h"
#include "fdosecrets/ClientAuth.h"

class Entry;
class QTreeWidgetItem;

namespace Ui
{
    class AccessControlDialog;
}

enum class AuthOption
{
    None = 0,
    /// offer to store the decision in the database
    Persist = 1 << 1,
};
Q_DECLARE_FLAGS(AuthOptions, AuthOption);
Q_DECLARE_OPERATORS_FOR_FLAGS(AuthOptions);

/**
 * A fingerprint change detected against a persisted identity record: the client
 * is identified by its non-hash conditions, but the executable content at the
 * given hierarchy depths no longer matches the recorded hashes.
 */
struct FingerprintChangeInfo
{
    QSet<int> mismatchedDepths;
    /// when the changed record was originally created
    QDateTime authorizedOn;

    bool isChanged() const
    {
        return !mismatchedDepths.isEmpty();
    }
};

/**
 * The identity record the requesting client already has. A decision
 * remembered for a known client is stored under that record, so the dialog
 * shows what the record matches on instead of asking.
 */
struct KnownClientInfo
{
    /// the record's name, as listed in the database's Secret Service settings
    QString name;
    QDateTime since;
    /// the rule of the record that identifies the client
    FdoSecrets::MatchRule rule;

    bool isKnown() const
    {
        return since.isValid();
    }
};

/**
 * Asks whether a client may access entries of one database.
 */
class AccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessControlDialog(QWindow* parent,
                                 const QList<Entry*>& entries,
                                 const QString& databaseName,
                                 const QString& clientName,
                                 const FdoSecrets::PeerInfo& info,
                                 AuthOptions authOptions = AuthOption::Persist,
                                 const FingerprintChangeInfo& fingerprintChange = {},
                                 const KnownClientInfo& known = {});
    ~AccessControlDialog() override;

    enum DialogCode
    {
        Rejected,
        Allow,
        Deny,
    };

    QHash<QUuid, AuthDecision> decisions() const;

    /**
     * The hierarchy depths the Match column currently selects for a new rule.
     */
    QSet<int> selectedMatchDepths() const;

signals:
    /**
     * @param results per shown entry decision; Undecided for the entries left unchecked
     * @param forFutureEntries catch-all decision covering entries not shown
     * @param persist store the decisions in the affected databases
     * @param matchDepths hierarchy depths a newly created identity rule anchors on
     */
    void finished(const QHash<QUuid, AuthDecision>& results,
                  AuthDecision forFutureEntries,
                  bool persist,
                  const QSet<int>& matchDepths);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void dialogFinished(int result);
    void coversFutureChanged(bool coversFuture);
    void updateRecognition();

private:
    class EntryModel;

    // procTree columns
    enum ProcColumn
    {
        ColumnName = 0,
        ColumnMatch,
        ColumnPid,
        ColumnExe,
        ColumnCommand,
    };

    /// Fills the client block; returns whether it shows anything to look at twice.
    bool setupClient(const QString& clientName, const FdoSecrets::PeerInfo& info);
    void setupEntries();
    void setupRemember(const FdoSecrets::PeerInfo& info, bool hasWarning);
    void setupDetails(const FdoSecrets::PeerInfo& info,
                      const FingerprintChangeInfo& fingerprintChange,
                      const KnownClientInfo& known);
    void updateRememberNotes();
    /// Size the dialog to what it currently shows.
    void fitToContents();
    bool remembering() const;

    QScopedPointer<Ui::AccessControlDialog> m_ui;
    QScopedPointer<EntryModel> m_model;
    QHash<QUuid, AuthDecision> m_decisions;
    KnownClientInfo m_known;
    bool m_fingerprintChanged;
    bool m_genericClient = false;
    bool m_canPersist;
    // what Remember goes back to when the future row is unchecked again
    bool m_rememberBeforeFuture = false;
    // procTree row for each hierarchy depth
    QHash<int, QTreeWidgetItem*> m_procItems;
};

/**
 * The requested entries, each with a check box. While the decision covers
 * future entries it covers all of these as well, so they are all checked and
 * cannot be changed.
 */
class AccessControlDialog::EntryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EntryModel(const QList<Entry*>& entries, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setCoversFuture(bool coversFuture);
    /// Every entry with whether the decision applies to it.
    QHash<QUuid, bool> selection() const;

public slots:
    void toggleCheckState(const QModelIndex& index);

private:
    // A snapshot of everything shown about an entry, taken at construction.
    // The dialog must not keep Entry pointers: the entries are destroyed if
    // the database locks or closes while the dialog is open.
    struct EntryData
    {
        QUuid uuid;
        QString title;
        QString username;
        QPixmap icon;
    };

    bool isValid(const QModelIndex& index) const;

    QList<EntryData> m_entryDataList;
    QSet<QUuid> m_selected;
    bool m_coversFuture = false;
};

#endif // KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

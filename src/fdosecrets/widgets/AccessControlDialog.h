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
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QHash>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSet>
#include <QUuid>

#include "core/Global.h"

class Entry;
class QTreeWidgetItem;

namespace Ui
{
    class AccessControlDialog;
}

namespace FdoSecrets
{
    struct PeerInfo;
}

enum class AuthOption
{
    None = 0,
    /// offer the scopes that store the decision in the database
    Persist = 1 << 1,
    PerEntryDeny = 1 << 2,
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

class AccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessControlDialog(QWindow* parent,
                                 const QList<Entry*>& entries,
                                 const QString& app,
                                 const FdoSecrets::PeerInfo& info,
                                 AuthOptions authOptions = AuthOption::Persist | AuthOption::PerEntryDeny,
                                 const FingerprintChangeInfo& fingerprintChange = {});
    ~AccessControlDialog() override;

    enum DialogCode
    {
        Rejected,
        Allow,
        Deny,
    };

    /**
     * How far the verdict reaches. Selected and Future are stored in the
     * database; Future additionally covers entries the client has not asked
     * for yet, which is a statement that only makes sense stored.
     */
    enum class Scope
    {
        Once,
        Selected,
        Future,
    };

    QHash<QUuid, AuthDecision> decisions() const;

    /**
     * The hierarchy depths the Match column currently selects for a new rule.
     */
    QSet<int> selectedMatchDepths() const;

signals:
    /**
     * @param results per shown entry decision
     * @param forFutureEntries catch-all decision covering entries not shown
     * @param persist store the decisions in the affected databases
     * @param matchDepths hierarchy depths a newly created identity rule anchors on
     */
    void finished(const QHash<QUuid, AuthDecision>& results,
                  AuthDecision forFutureEntries,
                  bool persist,
                  const QSet<int>& matchDepths);

private slots:
    void denyEntryClicked(const QUuid& uuid, const QModelIndex& index);
    void dialogFinished(int result);

private:
    class EntryModel;
    class DenyButton;

    // procTree columns
    enum ProcColumn
    {
        ColumnName = 0,
        ColumnMatch,
        ColumnPid,
        ColumnExe,
        ColumnCommand,
    };

    void setupDetails(const FdoSecrets::PeerInfo& info, const FingerprintChangeInfo& fingerprintChange);
    void setMatchColumnEnabled(bool enabled);

    Scope scope() const;

    QScopedPointer<Ui::AccessControlDialog> m_ui;
    QPointer<QRadioButton> m_scopeOnce;
    QPointer<QRadioButton> m_scopeSelected;
    QPointer<QRadioButton> m_scopeFuture;
    QScopedPointer<EntryModel> m_model;
    QHash<QUuid, AuthDecision> m_decisions;
    // per-entry deny intents, mapped to decisions when the dialog concludes
    QSet<QUuid> m_deniedEntries;
    // procTree row for each hierarchy depth
    QHash<int, QTreeWidgetItem*> m_procItems;
};

class AccessControlDialog::EntryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EntryModel(const QList<Entry*>& entries, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool removeRows(int row, int count, const QModelIndex& parent) override;

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

    QList<EntryData> m_entries;
    QSet<QUuid> m_selected;
};

class AccessControlDialog::DenyButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QUuid uuid READ uuid WRITE setUuid USER true)

    QPersistentModelIndex m_index;
    QUuid m_uuid;

public:
    explicit DenyButton(QWidget* p, const QModelIndex& idx);

    void setUuid(const QUuid& uuid);
    QUuid uuid() const;

signals:
    void clicked(const QUuid& uuid, const QModelIndex& idx);
};

#endif // KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

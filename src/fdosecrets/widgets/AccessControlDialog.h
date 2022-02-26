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
#include <QDialog>
#include <QPointer>
#include <QPushButton>
#include <QSet>

#include "core/Global.h"

class Entry;

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
    Remember = 1 << 1,
    PerEntryDeny = 1 << 2,
};
Q_DECLARE_FLAGS(AuthOptions, AuthOption);
Q_DECLARE_OPERATORS_FOR_FLAGS(AuthOptions);

class AccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessControlDialog(QWindow* parent,
                                 const QList<Entry*>& entries,
                                 const QString& app,
                                 const FdoSecrets::PeerInfo& info,
                                 AuthOptions authOptions = AuthOption::Remember | AuthOption::PerEntryDeny);
    ~AccessControlDialog() override;

    enum DialogCode
    {
        Rejected,
        AllowSelected,
        AllowAll,
        DenyAll,
    };

    QHash<Entry*, AuthDecision> decisions() const;

signals:
    void finished(const QHash<Entry*, AuthDecision>& results, AuthDecision forFutureEntries);

private slots:
    void denyEntryClicked(Entry* entry, const QModelIndex& index);
    void dialogFinished(int result);

private:
    class EntryModel;
    class DenyButton;

    void setupDetails(const FdoSecrets::PeerInfo& info);

    QScopedPointer<Ui::AccessControlDialog> m_ui;
    QPointer<QCheckBox> m_rememberCheck;
    QScopedPointer<EntryModel> m_model;
    QHash<Entry*, AuthDecision> m_decisions;
};

class AccessControlDialog::EntryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EntryModel(QList<Entry*> entries, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool removeRows(int row, int count, const QModelIndex& parent) override;

public slots:
    void toggleCheckState(const QModelIndex& index);

private:
    bool isValid(const QModelIndex& index) const;

    QList<Entry*> m_entries;
    QSet<Entry*> m_selected;
};

class AccessControlDialog::DenyButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(Entry* entry READ entry WRITE setEntry USER true)

    QPersistentModelIndex m_index;
    QPointer<Entry> m_entry;

public:
    explicit DenyButton(QWidget* p, const QModelIndex& idx);

    void setEntry(Entry* e);
    Entry* entry() const;

signals:
    void clicked(Entry*, const QModelIndex& idx);
};

#endif // KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

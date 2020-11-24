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
#include <QDialog>
#include <QScopedPointer>
#include <QSet>

class Entry;

namespace Ui
{
    class AccessControlDialog;
}

class AccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessControlDialog(QWindow* parent, const QList<Entry*>& items, const QString& name, uint pid);
    ~AccessControlDialog() override;

    QList<int> getEntryIndices() const;

    enum DialogCode
    {
        DenyAll = QDialog::Rejected,
        AllowSelected = QDialog::Accepted,
        AllowAll,
    };

private:
    class EntryModel;

    QScopedPointer<Ui::AccessControlDialog> m_ui;
    QScopedPointer<EntryModel> m_model;
};

class AccessControlDialog::EntryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EntryModel(QList<Entry*> entries, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    QList<int> selectedIndices() const;

public slots:
    void toggleCheckState(const QModelIndex& index);

private:
    bool isValid(const QModelIndex& index) const;

    QList<Entry*> m_entries;
    QSet<int> m_selected;
};

#endif // KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include <QDialog>
#include <QScopedPointer>

class Entry;
class QTableWidgetItem;

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

    QList<int> getEntryIndices(bool selected = true) const;

    enum DialogCode
    {
        DenyAll = QDialog::Rejected,
        AllowSelected = QDialog::Accepted,
        AllowAll,
    };

signals:
    void disableAccess(Entry* entry);

private:
    void disableButtonPressed(QTableWidgetItem* item);

    QScopedPointer<Ui::AccessControlDialog> m_ui;
};

#endif // KEEPASSXC_FDOSECRETS_ACCESSCONTROLDIALOG_H

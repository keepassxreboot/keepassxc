/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_PASSKEYEXPORTDIALOG_H
#define KEEPASSXC_PASSKEYEXPORTDIALOG_H

#include <QDialog>
#include <QTableWidget>

class Entry;

namespace Ui
{
    class PasskeyExportDialog;
}

class PasskeyExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasskeyExportDialog(QWidget* parent = nullptr);
    ~PasskeyExportDialog() override;

    void setEntries(const QList<Entry*>& items);
    QList<QTableWidgetItem*> getSelectedItems() const;
    QString selectExportFolder();

private slots:
    void selectionChanged();

private:
    QScopedPointer<Ui::PasskeyExportDialog> m_ui;
    QList<Entry*> m_entriesToConfirm;
    QList<Entry*> m_allowedEntries;
};

#endif // KEEPASSXC_PASSKEYEXPORTDIALOG_H

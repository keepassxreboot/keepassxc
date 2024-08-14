/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2013 Francois Ferrand
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

#ifndef KEEPASSXC_BROWSERACCESSCONTROLDIALOG_H
#define KEEPASSXC_BROWSERACCESSCONTROLDIALOG_H

#include <QDialog>
#include <QTableWidget>

class Entry;

namespace Ui
{
    class BrowserAccessControlDialog;
}

enum SelectionType
{
    Selected,
    NonSelected,
    Disabled
};

class BrowserAccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowserAccessControlDialog(QWidget* parent = nullptr);
    ~BrowserAccessControlDialog() override;

    void setEntries(const QList<Entry*>& entriesToConfirm, const QString& urlString, bool httpAuth);
    bool remember() const;
    QList<QTableWidgetItem*> getEntries(SelectionType selectionType) const;

signals:
    void disableAccess(QTableWidgetItem* item);

private slots:
    void selectionChanged();

private:
    void addEntryToList(Entry* entry, int row);
    bool areAllDisabled() const;
    QList<QTableWidgetItem*> getAllItems() const;

private:
    QScopedPointer<Ui::BrowserAccessControlDialog> m_ui;
    QList<Entry*> m_entriesToConfirm;
};

#endif // KEEPASSXC_BROWSERACCESSCONTROLDIALOG_H

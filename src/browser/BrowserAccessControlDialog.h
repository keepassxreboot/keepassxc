/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#ifndef BROWSERACCESSCONTROLDIALOG_H
#define BROWSERACCESSCONTROLDIALOG_H

#include <QDialog>
#include <QTableWidget>

class Entry;

namespace Ui
{
    class BrowserAccessControlDialog;
}

class BrowserAccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowserAccessControlDialog(QWidget* parent = nullptr);
    ~BrowserAccessControlDialog() override;

    void setItems(const QList<Entry*>& entriesToConfirm,
                  const QList<Entry*>& allowedEntries,
                  const QString& urlString,
                  bool httpAuth);
    bool remember() const;
    bool entriesAccepted() const;

    QList<QTableWidgetItem*> getSelectedEntries() const;
    QList<QTableWidgetItem*> getNonSelectedEntries() const;

signals:
    void disableAccess(QTableWidgetItem* item);
    void acceptEntries(QList<QTableWidgetItem*> items, QList<Entry*> entriesToConfirm, QList<Entry*> allowedEntries);
    void rejectEntries(QList<QTableWidgetItem*> items, QList<Entry*> entriesToConfirm);
    void closed();

public slots:
    void acceptSelections();
    void rejectSelections();

private:
    void closeEvent(QCloseEvent* event) override;

private:
    QScopedPointer<Ui::BrowserAccessControlDialog> m_ui;
    QList<Entry*> m_entriesToConfirm;
    QList<Entry*> m_allowedEntries;
    bool m_entriesAccepted;
};

#endif // BROWSERACCESSCONTROLDIALOG_H

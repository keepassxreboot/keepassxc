/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REPORTSWIDGETPASSKEYS_H
#define KEEPASSXC_REPORTSWIDGETPASSKEYS_H

#include "gui/entry/EntryModel.h"
#include "gui/reports/ReportsWidgetBase.h"
#include <QWidget>

class Database;
class Entry;
class Group;
class PasswordHealth;
class QSortFilterProxyModel;
class QStandardItemModel;

namespace Ui
{
    class ReportsWidgetPasskeys;
}

class ReportsWidgetPasskeys : public ReportsWidgetBase
{
    Q_OBJECT
public:
    explicit ReportsWidgetPasskeys(QWidget* parent = nullptr);
    ~ReportsWidgetPasskeys() override;

protected:
    void showEvent(QShowEvent* event) override;
    void updateWidget() override;
    QTableView* getTableView() const override;

public slots:
    void updateEntries();
    void customMenuRequested(QPoint);

private slots:
    void selectionChanged();
    void importPasskey();
    void exportPasskey();

private:
    void addPasskeyRow(Group*, Entry*);

    QScopedPointer<Ui::ReportsWidgetPasskeys> m_ui;
};

#endif // KEEPASSXC_REPORTSWIDGETPASSKEYS_H

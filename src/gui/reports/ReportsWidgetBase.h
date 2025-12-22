/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REPORTSWIDGETBASE_H
#define KEEPASSXC_REPORTSWIDGETBASE_H

#include <QWidget>

#include "gui/entry/EntryModel.h"
#include "gui/reports/ProxyModels.h"

class Database;
class Entry;
class Group;
class PasswordHealth;
class QSortFilterProxyModel;
class QStandardItemModel;
class QMenu;
class QTableView;

/**
 * @brief The ReportsWidgetBase class implements functionality common across the various
 * database report widgets.
 */
class ReportsWidgetBase : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsWidgetBase(QWidget* parent, SortProxyModelKind);
    virtual ~ReportsWidgetBase();

    virtual void loadSettings(QSharedPointer<Database> db);
    virtual void saveSettings();

protected:
    virtual QTableView* getTableView() const = 0;
    virtual void updateWidget() = 0;

    QMenu* customMenuRequestedBase();

public slots:
    QList<Entry*> getSelectedEntries() const;
    void expireSelectedEntries();
    void deleteSelectedEntries();
    void emitEntryActivated(const QModelIndex& index);

signals:
    void entryActivated(Entry*);

protected:
    bool m_widgetDataCalculated = false;
    QScopedPointer<QStandardItemModel> m_referencesModel;
    QScopedPointer<QSortFilterProxyModel> m_modelProxy;
    QSharedPointer<Database> m_db;
    QList<QPair<Group*, Entry*>> m_rowToEntry;
};

#endif // KEEPASSXC_REPORTSWIDGETBASE_H

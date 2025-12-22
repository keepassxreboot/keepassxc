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

#include "ReportsWidgetBase.h"

#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/GuiTools.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>

ReportsWidgetBase::ReportsWidgetBase(QWidget* parent, SortProxyModelKind proxyModel)
    : QWidget{parent}
    , m_referencesModel(new QStandardItemModel(this))
{
    // We have to initialize this here; if we do it in the constructor initializer list,
    // the base object isn't setup enough and the constructor for QSortFilterProxyModel
    // crashes.
    switch (proxyModel) {
    case SortProxyModelKind::Default:
        m_modelProxy.reset(new QSortFilterProxyModel(this));
        break;
    case SortProxyModelKind::Healthcheck:
        m_modelProxy.reset(new HealthcheckReportSortProxyModel(this));
        break;
    case SortProxyModelKind::Hibp:
        m_modelProxy.reset(new HibpReportSortProxyModel(this));
        break;
    }
}

ReportsWidgetBase::~ReportsWidgetBase()
{
}

void ReportsWidgetBase::loadSettings(QSharedPointer<Database> db)
{
    m_db = std::move(db);
    m_widgetDataCalculated = false;
    m_referencesModel->clear();
    m_rowToEntry.clear();

    auto row = QList<QStandardItem*>();
    row << new QStandardItem(tr("Please wait, report is being calculated…"));
    m_referencesModel->appendRow(row);
}

void ReportsWidgetBase::saveSettings()
{
    // Most report tabs are passive, so override them in derived classes if they need to
    // save settings
}

QMenu* ReportsWidgetBase::customMenuRequestedBase()
{
    auto selected = getTableView()->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return nullptr;
    }

    // Create the context menu
    const auto menu = new QMenu(this);
    menu->setObjectName("customMenu");

    // Create the "edit entry" menu item (only if 1 row is selected)
    if (selected.size() == 1) {
        const auto edit = new QAction(icons()->icon("entry-edit"), tr("Edit Entry…"), this);
        edit->setObjectName("contextMenuEditAction");
        menu->addAction(edit);
        connect(edit, &QAction::triggered, edit, [this, selected] {
            auto row = m_modelProxy->mapToSource(selected[0]).row();
            auto entry = m_rowToEntry[row].second;
            emit entryActivated(entry);
        });
    }

    // Create the "Expire entry" menu item
    const auto expEntry = new QAction(icons()->icon("entry-expire"), tr("Expire Entry(s)…", "", selected.size()), this);
    expEntry->setObjectName("contextMenuExpireAction");
    menu->addAction(expEntry);
    connect(expEntry, &QAction::triggered, this, &ReportsWidgetBase::expireSelectedEntries);

    // Create the "delete entry" menu item
    const auto delEntry = new QAction(icons()->icon("entry-delete"), tr("Delete Entry(s)…", "", selected.size()), this);
    menu->addAction(delEntry);
    connect(delEntry, &QAction::triggered, this, &ReportsWidgetBase::deleteSelectedEntries);

    // Create the "exclude from reports" menu item
    const auto excludeAction = new QAction(icons()->icon("reports-exclude"), tr("Exclude Entry(s) from reports"), this);
    excludeAction->setObjectName("contextMenuExcludeAction");
    const auto excludeGroupsAction =
        new QAction(icons()->icon("reports-exclude"), tr("Exclude Group(s) from reports"), this);
    excludeGroupsAction->setObjectName("contextMenuExcludeGroupAction");

    bool isExcluded = false;
    bool isGroupExcluded = false;

    for (auto index : selected) {
        auto row = m_modelProxy->mapToSource(index).row();
        auto entry = m_rowToEntry[row].second;
        if (entry) {
            // If at least one entry is excluded switch to inclusion
            if (entry->excludeFromReports() || entry->group()->excludeFromReports()) {
                isExcluded = true;
            }
            if (entry->group()->excludeFromReports()) {
                isGroupExcluded = true;
            }

            break;
        }
    }
    excludeAction->setCheckable(true);
    excludeAction->setChecked(isExcluded);

    excludeGroupsAction->setCheckable(true);
    excludeGroupsAction->setChecked(isGroupExcluded);

    menu->addAction(excludeAction);
    connect(excludeAction, &QAction::toggled, excludeAction, [this, selected](bool checked) {
        QSet<Group*> groups;

        // If we are including entries (checked is false) but a group is excluded, ask the user if they
        // would like to include the rest of the group as well (or keep it excluded).
        // If they choose "No", we need to include the whole group, and then exclude
        // the entries that aren't selected here.
        if (!checked) {
            for (const auto index : selected) {
                auto row = m_modelProxy->mapToSource(index).row();
                auto entry = m_rowToEntry[row].second;

                if (entry) {
                    auto* group = entry->group();
                    if (group->excludeFromReports() && !groups.contains(group)) {
                        QString msg = tr("The Group for \"%1\" is excluded. Would you like to include all Entries from "
                                         "there as well?")
                                          .arg(entry->title());
                        auto response = MessageBox::question(this,
                                                             tr("Include Group?"),
                                                             msg,
                                                             MessageBox::Yes | MessageBox::No | MessageBox::Cancel,
                                                             MessageBox::No);

                        if (response == MessageBox::Cancel) {
                            return;
                        } else if (response == MessageBox::Yes) {
                            group->setExcludeFromReports(false);
                        } else if (response == MessageBox::No) {
                            // We'll exclude all entries from the group here and then
                            // include the selected ones below
                            group->setExcludeFromReports(false);
                            group->markAllEntriesExcludedFromReports();
                        }

                        groups.insert(group);
                    }
                }
            }
        }

        for (auto index : selected) {
            auto row = m_modelProxy->mapToSource(index).row();
            auto entry = m_rowToEntry[row].second;

            // If the containing group is excluded but the user wants to include
            // this entry, ask if they want to keep the remaining items in the group
            // excluded or included
            if (entry) {
                entry->setExcludeFromReports(checked);
            }
        }
        updateWidget();
    });

    menu->addAction(excludeGroupsAction);
    connect(excludeGroupsAction, &QAction::toggled, excludeGroupsAction, [this, selected](bool checked) {
        for (const auto index : selected) {
            auto row = m_modelProxy->mapToSource(index).row();
            auto entry = m_rowToEntry[row].second;
            if (entry) {
                entry->group()->setExcludeFromReports(checked);
            }
        }
        updateWidget();
    });

    return menu;
}

QList<Entry*> ReportsWidgetBase::getSelectedEntries() const
{
    QList<Entry*> selectedEntries;
    for (auto index : getTableView()->selectionModel()->selectedRows()) {
        auto row = m_modelProxy->mapToSource(index).row();
        auto entry = m_rowToEntry[row].second;
        if (entry) {
            selectedEntries << entry;
        }
    }
    return selectedEntries;
}

void ReportsWidgetBase::expireSelectedEntries()
{
    for (auto entry : getSelectedEntries()) {
        entry->expireNow();
    }

    updateWidget();
}

void ReportsWidgetBase::deleteSelectedEntries()
{
    const auto& selectedEntries = getSelectedEntries();
    bool permanent = !m_db->metadata()->recycleBinEnabled();

    if (GuiTools::confirmDeleteEntries(this, selectedEntries, permanent)) {
        GuiTools::deleteEntriesResolveReferences(this, selectedEntries, permanent);
    }

    updateWidget();
}

void ReportsWidgetBase::emitEntryActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    auto mappedIndex = m_modelProxy->mapToSource(index);
    const auto row = m_rowToEntry[mappedIndex.row()];
    const auto group = row.first;
    const auto entry = row.second;

    if (group && entry) {
        emit entryActivated(const_cast<Entry*>(entry));
    }
}

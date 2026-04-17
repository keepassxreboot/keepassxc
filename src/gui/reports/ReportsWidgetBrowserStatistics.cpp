/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsWidgetBrowserStatistics.h"
#include "ui_ReportsWidgetBrowserStatistics.h"

#include "browser/BrowserService.h"
#include "core/AsyncTask.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/GuiTools.h"
#include "gui/Icons.h"
#include "gui/styles/StateColorPalette.h"

#include <QJsonDocument>
#include <QMenu>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace
{
    class BrowserStatistics
    {
    public:
        struct Item
        {
            QPointer<Group> group;
            QPointer<Entry> entry;
            bool hasUrls;
            bool hasSettings;
            bool exclude = false;

            Item(Group* g, Entry* e, bool hU, bool hS)
                : group(g)
                , entry(e)
                , hasUrls(hU)
                , hasSettings(hS)
                , exclude(e->excludeFromReports() || g->excludeFromReports())
            {
            }
        };

        explicit BrowserStatistics(QSharedPointer<Database>);

        const QList<QSharedPointer<Item>>& items() const
        {
            return m_items;
        }

    private:
        QSharedPointer<Database> m_db;
        QList<QSharedPointer<Item>> m_items;
    };
} // namespace

BrowserStatistics::BrowserStatistics(QSharedPointer<Database> db)
    : m_db(db)
{
    for (auto group : db->rootGroup()->groupsRecursive(true)) {
        // Skip recycle bin
        if (group->isRecycled()) {
            continue;
        }

        for (auto entry : group->entries()) {
            if (entry->isRecycled()) {
                continue;
            }

            auto hasUrls = !entry->getAllUrls().isEmpty();
            auto hasSettings = entry->customData()->contains(BrowserService::KEEPASSXCBROWSER_NAME);

            const auto item = QSharedPointer<Item>(new Item(group, entry, hasUrls, hasSettings));
            m_items.append(item);
        }
    }
}

ReportsWidgetBrowserStatistics::ReportsWidgetBrowserStatistics(QWidget* parent)
    : ReportsWidgetBase(parent, SortProxyModelKind::Default)
    , m_ui(new Ui::ReportsWidgetBrowserStatistics())
{
    m_ui->setupUi(this);

    m_modelProxy->setSourceModel(m_referencesModel.data());
    m_modelProxy->setSortLocaleAware(true);
    m_ui->browserStatisticsTableView->setModel(m_modelProxy.data());
    m_ui->browserStatisticsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->browserStatisticsTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(m_ui->browserStatisticsTableView,
            SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(customMenuRequested(QPoint)));
    connect(
        m_ui->browserStatisticsTableView, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
    connect(m_ui->showEntriesWithUrlOnlyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(calculateBrowserStatistics()));
    connect(m_ui->showAllowDenyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(calculateBrowserStatistics()));
    connect(m_ui->showExpired, SIGNAL(stateChanged(int)), this, SLOT(calculateBrowserStatistics()));

    new QShortcut(Qt::Key_Delete, this, SLOT(deleteSelectedEntries()));
}

ReportsWidgetBrowserStatistics::~ReportsWidgetBrowserStatistics()
{
}

void ReportsWidgetBrowserStatistics::addStatisticsRow(bool hasUrls,
                                                      bool hasSettings,
                                                      Group* group,
                                                      Entry* entry,
                                                      bool excluded)
{
    StateColorPalette statePalette;

    auto urlList = entry->getAllUrls();
    auto urlToolTip = hasUrls ? tr("List of entry URLs") : tr("Entry has no URLs set");

    auto browserConfig = getBrowserConfigFromEntry(entry);
    auto allowedUrlsList = browserConfig["Allow"];
    auto deniedUrlsList = browserConfig["Deny"];

    auto allowedUrlsToolTip = hasSettings ? tr("Allowed URLs") : tr("Entry has no Browser Integration settings");
    auto deniedUrlsToolTip = hasSettings ? tr("Denied URLs") : tr("Entry has no Browser Integration settings");

    auto title = entry->title();
    if (excluded) {
        if (group->excludeFromReports()) {
            title.append(tr(" (Group Excluded)"));
        } else {
            title.append(tr(" (Excluded)"));
        }
    }
    if (entry->isExpired()) {
        title.append(tr(" (Expired)"));
    }

    auto row = QList<QStandardItem*>();
    row << new QStandardItem(Icons::entryIconPixmap(entry), title);
    row << new QStandardItem(Icons::groupIconPixmap(group), group->hierarchy().join("/"));
    row << new QStandardItem(urlList.join('\n'));
    row << new QStandardItem(allowedUrlsList.join('\n'));
    row << new QStandardItem(deniedUrlsList.join('\n'));

    // Set tooltips
    row[2]->setToolTip(urlToolTip);
    row[3]->setToolTip(allowedUrlsToolTip);
    row[4]->setToolTip(deniedUrlsToolTip);
    if (excluded) {
        if (group->excludeFromReports()) {
            row[0]->setToolTip(tr("The group for this entry is being excluded from reports"));
        } else {
            row[0]->setToolTip(tr("This entry is being excluded from reports"));
        }
    }

    // Store entry pointer per table row (used in double click handler)
    m_referencesModel->appendRow(row);
    m_rowToEntry.append({group, entry});
}

void ReportsWidgetBrowserStatistics::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (!m_widgetDataCalculated) {
        // Perform stats calculation on next event loop to allow widget to appear
        m_widgetDataCalculated = true;
        QTimer::singleShot(0, this, SLOT(calculateBrowserStatistics()));
    }
}

void ReportsWidgetBrowserStatistics::calculateBrowserStatistics()
{
    m_referencesModel->clear();

    // Perform the statistics check
    const QScopedPointer<BrowserStatistics> browserStatistics(
        AsyncTask::runAndWaitForFuture([this] { return new BrowserStatistics(m_db); }));

    const auto showExpired = m_ui->showExpired->isChecked();
    const auto showEntriesWithUrlOnly = m_ui->showEntriesWithUrlOnlyCheckBox->isChecked();
    const auto showOnlyEntriesWithSettings = m_ui->showAllowDenyCheckBox->isChecked();

    // Display the entries
    m_rowToEntry.clear();
    for (const auto& item : browserStatistics->items()) {
        // Check if the entry should be displayed
        if (!showExpired && item->entry->isExpired()) {
            continue;
        }

        // Exclude this entry if URL are not set
        if (showEntriesWithUrlOnly && !item->hasUrls) {
            continue;
        }

        // Exclude this entry if it doesn't have any Browser Integration settings
        if (showOnlyEntriesWithSettings
            && !item->entry->customData()->contains(BrowserService::KEEPASSXCBROWSER_NAME)) {
            continue;
        }

        // Show the entry in the report
        addStatisticsRow(item->hasUrls, item->hasSettings, item->group, item->entry, item->exclude);
    }

    // Set the table header
    if (m_referencesModel->rowCount() == 0) {
        m_referencesModel->setHorizontalHeaderLabels(
            QStringList() << tr("No entries with a URL, or none has browser extension settings saved."));
    } else {
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Path") << tr("URLs")
                                                                   << tr("Allowed URLs") << tr("Denied URLs"));
        m_ui->browserStatisticsTableView->sortByColumn(0, Qt::AscendingOrder);
    }

    m_ui->browserStatisticsTableView->resizeColumnsToContents();
}

void ReportsWidgetBrowserStatistics::customMenuRequested(QPoint pos)
{
    auto menu = customMenuRequestedBase();

    if (!menu) {
        return;
    }

    auto selected = getTableView()->selectionModel()->selectedRows();

    // Create the "delete plugin data" menu item
    const auto deletePluginData =
        new QAction(icons()->icon("entry-delete"), tr("Delete plugin data from Entry(s)…", "", selected.size()), this);
    menu->insertAction(menu->actions().at(3),
                       deletePluginData); // Index 3 is the one after "Delete Entry" so place "Delete plugin" before it
    connect(deletePluginData,
            &QAction::triggered,
            this,
            &ReportsWidgetBrowserStatistics::deletePluginDataFromSelectedEntries);

    // Show the context menu
    menu->popup(m_ui->browserStatisticsTableView->viewport()->mapToGlobal(pos));
}

void ReportsWidgetBrowserStatistics::deletePluginDataFromSelectedEntries()
{
    const auto& selectedEntries = getSelectedEntries();
    if (GuiTools::confirmDeletePluginData(this, selectedEntries)) {
        for (auto& entry : selectedEntries) {
            browserService()->removePluginData(entry);
        }
    }

    calculateBrowserStatistics();
}

QMap<QString, QStringList> ReportsWidgetBrowserStatistics::getBrowserConfigFromEntry(Entry* entry) const
{
    QMap<QString, QStringList> configList;

    auto config = entry->customData()->value(BrowserService::KEEPASSXCBROWSER_NAME);
    if (!config.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
        if (!doc.isNull()) {
            auto jsonObject = doc.object();
            auto allowedSites = jsonObject["Allow"].toArray();
            auto deniedSites = jsonObject["Deny"].toArray();

            QStringList allowed;
            foreach (const auto& value, allowedSites) {
                auto url = value.toString();
                if (!url.isEmpty()) {
                    allowed << url;
                }
            }

            QStringList denied;
            foreach (const auto& value, deniedSites) {
                auto url = value.toString();
                if (!url.isEmpty()) {
                    denied << url;
                }
            }

            configList.insert("Allow", allowed);
            configList.insert("Deny", denied);
        }
    }

    return configList;
}

QTableView* ReportsWidgetBrowserStatistics::getTableView() const
{
    return m_ui->browserStatisticsTableView;
}

void ReportsWidgetBrowserStatistics::updateWidget()
{
    calculateBrowserStatistics();
}

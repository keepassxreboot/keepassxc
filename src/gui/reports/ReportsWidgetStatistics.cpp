/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsWidgetStatistics.h"
#include "ui_ReportsWidgetStatistics.h"

#include "core/AsyncTask.h"
#include "core/DatabaseStats.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordHealth.h"
#include "gui/Icons.h"

#include <QStandardItemModel>

ReportsWidgetStatistics::ReportsWidgetStatistics(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ReportsWidgetStatistics())
    , m_errIcon(icons()->icon("dialog-error"))
{
    m_ui->setupUi(this);

    m_referencesModel.reset(new QStandardItemModel());
    m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
    m_ui->statisticsTableView->setModel(m_referencesModel.data());
    m_ui->statisticsTableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_ui->statisticsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

ReportsWidgetStatistics::~ReportsWidgetStatistics() = default;

void ReportsWidgetStatistics::addStatsRow(QString name, QString value, bool bad, QString badMsg)
{
    auto row = QList<QStandardItem*>();
    row << new QStandardItem(name);
    row << new QStandardItem(value);
    m_referencesModel->appendRow(row);

    if (bad) {
        m_referencesModel->item(m_referencesModel->rowCount() - 1, 1)->setIcon(m_errIcon);
        if (!badMsg.isEmpty()) {
            m_referencesModel->item(m_referencesModel->rowCount() - 1, 1)->setToolTip(badMsg);
        }
    }
};

void ReportsWidgetStatistics::loadSettings(QSharedPointer<Database> db)
{
    m_db = std::move(db);
    m_statsCalculated = false;
    m_referencesModel->clear();
    addStatsRow(tr("Please wait, database statistics are being calculated…"), "");
}

void ReportsWidgetStatistics::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (!m_statsCalculated) {
        // Perform stats calculation on next event loop to allow widget to appear
        m_statsCalculated = true;
        QTimer::singleShot(0, this, SLOT(calculateStats()));
    }
}

void ReportsWidgetStatistics::calculateStats()
{
    const QScopedPointer<DatabaseStats> stats(
        AsyncTask::runAndWaitForFuture([this] { return new DatabaseStats(m_db); }));

    m_referencesModel->clear();
    addStatsRow(tr("Database name"), m_db->metadata()->name());
    addStatsRow(tr("Description"), m_db->metadata()->description());
    addStatsRow(tr("Location"), m_db->filePath());
    addStatsRow(tr("Database created"),
                m_db->rootGroup()->timeInfo().creationTime().toString(Qt::DefaultLocaleShortDate));
    addStatsRow(tr("Last saved"), stats->modified.toString(Qt::DefaultLocaleShortDate));
    addStatsRow(tr("Unsaved changes"),
                m_db->isModified() ? tr("yes") : tr("no"),
                m_db->isModified(),
                tr("The database was modified, but the changes have not yet been saved to disk."));
    addStatsRow(tr("Number of groups"), QString::number(stats->groupCount));
    addStatsRow(tr("Number of entries"), QString::number(stats->entryCount));
    addStatsRow(tr("Number of expired entries"),
                QString::number(stats->expiredEntries),
                stats->isAnyExpired(),
                tr("The database contains entries that have expired."));
    addStatsRow(tr("Unique passwords"), QString::number(stats->uniquePasswords));
    addStatsRow(tr("Non-unique passwords"),
                QString::number(stats->reusedPasswords),
                stats->areTooManyPwdsReused(),
                tr("More than 10% of passwords are reused. Use unique passwords when possible."));
    addStatsRow(tr("Maximum password reuse"),
                QString::number(stats->maxPwdReuse()),
                stats->arePwdsReusedTooOften(),
                tr("Some passwords are used more than three times. Use unique passwords when possible."));
    addStatsRow(tr("Number of short passwords"),
                QString::number(stats->shortPasswords),
                stats->shortPasswords > 0,
                tr("Recommended minimum password length is at least 8 characters."));
    addStatsRow(tr("Number of weak passwords"),
                QString::number(stats->weakPasswords),
                stats->weakPasswords > 0,
                tr("Recommend using long, randomized passwords with a rating of 'good' or 'excellent'."));
    addStatsRow(tr("Entries excluded from reports"),
                QString::number(stats->excludedEntries),
                stats->excludedEntries > 0,
                tr("Excluding entries from reports, e. g. because they are known to have a poor password, isn't "
                   "necessarily a problem but you should keep an eye on them."));
    addStatsRow(tr("Average password length"),
                tr("%1 characters").arg(stats->averagePwdLength()),
                stats->isAvgPwdTooShort(),
                tr("Average password length is less than ten characters. Longer passwords provide more security."));
}

void ReportsWidgetStatistics::saveSettings()
{
    // nothing to do - the tab is passive
}

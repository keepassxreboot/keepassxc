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
#include "core/Database.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordHealth.h"
#include "gui/Icons.h"

#include <QFileInfo>
#include <QHash>
#include <QStandardItemModel>

namespace
{
    class Stats
    {
    public:
        // The statistics we collect:
        QDateTime modified; // File modification time
        int nGroups = 0; // Number of groups in the database
        int nEntries = 0; // Number of entries (across all groups)
        int nExpired = 0; // Number of expired entries
        int nPwdsWeak = 0; // Number of weak or poor passwords
        int nPwdsShort = 0; // Number of passwords 8 characters or less in size
        int nPwdsUnique = 0; // Number of unique passwords
        int nPwdsReused = 0; // Number of non-unique passwords
        int nKnownBad = 0; // Number of known bad entries
        int pwdTotalLen = 0; // Total length of all passwords

        // Ctor does all the work
        explicit Stats(QSharedPointer<Database> db)
            : modified(QFileInfo(db->filePath()).lastModified())
            , m_db(db)
        {
            gatherStats(db->rootGroup()->groupsRecursive(true));
        }

        // Get average password length
        int averagePwdLength() const
        {
            return m_passwords.empty() ? 0 : pwdTotalLen / m_passwords.size();
        }

        // Get max number of password reuse (=how many entries
        // share the same password)
        int maxPwdReuse() const
        {
            int ret = 0;
            for (const auto& count : m_passwords) {
                ret = std::max(ret, count);
            }
            return ret;
        }

        // A warning sign is displayed if one of the
        // following returns true.
        bool isAnyExpired() const
        {
            return nExpired > 0;
        }

        bool areTooManyPwdsReused() const
        {
            return nPwdsReused > nPwdsUnique / 10;
        }

        bool arePwdsReusedTooOften() const
        {
            return maxPwdReuse() > 3;
        }

        bool isAvgPwdTooShort() const
        {
            return averagePwdLength() < 10;
        }

    private:
        QSharedPointer<Database> m_db;
        QHash<QString, int> m_passwords;

        void gatherStats(const QList<Group*>& groups)
        {
            auto checker = HealthChecker(m_db);

            for (const auto* group : groups) {
                // Don't count anything in the recycle bin
                if (group->isRecycled()) {
                    continue;
                }

                ++nGroups;

                for (const auto* entry : group->entries()) {
                    // Don't count anything in the recycle bin
                    if (entry->isRecycled()) {
                        continue;
                    }

                    ++nEntries;

                    if (entry->isExpired()) {
                        ++nExpired;
                    }

                    // Get password statistics
                    const auto pwd = entry->password();
                    if (!pwd.isEmpty()) {
                        if (!m_passwords.contains(pwd)) {
                            ++nPwdsUnique;
                        } else {
                            ++nPwdsReused;
                        }

                        if (pwd.size() < 8) {
                            ++nPwdsShort;
                        }

                        // Speed up Zxcvbn process by excluding very long passwords and most passphrases
                        if (pwd.size() < 25 && checker.evaluate(entry)->quality() <= PasswordHealth::Quality::Weak) {
                            ++nPwdsWeak;
                        }

                        if (entry->customData()->contains(PasswordHealth::OPTION_KNOWN_BAD)
                            && entry->customData()->value(PasswordHealth::OPTION_KNOWN_BAD) == TRUE_STR) {
                            ++nKnownBad;
                        }

                        pwdTotalLen += pwd.size();
                        m_passwords[pwd]++;
                    }
                }
            }
        }
    };
} // namespace

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

ReportsWidgetStatistics::~ReportsWidgetStatistics()
{
}

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
    addStatsRow(tr("Please wait, database statistics are being calculated..."), "");
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
    const QScopedPointer<Stats> stats(AsyncTask::runAndWaitForFuture([this] { return new Stats(m_db); }));

    m_referencesModel->clear();
    addStatsRow(tr("Database name"), m_db->metadata()->name());
    addStatsRow(tr("Description"), m_db->metadata()->description());
    addStatsRow(tr("Location"), m_db->filePath());
    addStatsRow(tr("Last saved"), stats->modified.toString(Qt::DefaultLocaleShortDate));
    addStatsRow(tr("Unsaved changes"),
                m_db->isModified() ? tr("yes") : tr("no"),
                m_db->isModified(),
                tr("The database was modified, but the changes have not yet been saved to disk."));
    addStatsRow(tr("Number of groups"), QString::number(stats->nGroups));
    addStatsRow(tr("Number of entries"), QString::number(stats->nEntries));
    addStatsRow(tr("Number of expired entries"),
                QString::number(stats->nExpired),
                stats->isAnyExpired(),
                tr("The database contains entries that have expired."));
    addStatsRow(tr("Unique passwords"), QString::number(stats->nPwdsUnique));
    addStatsRow(tr("Non-unique passwords"),
                QString::number(stats->nPwdsReused),
                stats->areTooManyPwdsReused(),
                tr("More than 10% of passwords are reused. Use unique passwords when possible."));
    addStatsRow(tr("Maximum password reuse"),
                QString::number(stats->maxPwdReuse()),
                stats->arePwdsReusedTooOften(),
                tr("Some passwords are used more than three times. Use unique passwords when possible."));
    addStatsRow(tr("Number of short passwords"),
                QString::number(stats->nPwdsShort),
                stats->nPwdsShort > 0,
                tr("Recommended minimum password length is at least 8 characters."));
    addStatsRow(tr("Number of weak passwords"),
                QString::number(stats->nPwdsWeak),
                stats->nPwdsWeak > 0,
                tr("Recommend using long, randomized passwords with a rating of 'good' or 'excellent'."));
    addStatsRow(tr("Entries excluded from reports"),
                QString::number(stats->nKnownBad),
                stats->nKnownBad > 0,
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

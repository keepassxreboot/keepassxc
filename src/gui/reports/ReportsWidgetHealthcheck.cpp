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

#include "ReportsWidgetHealthcheck.h"
#include "ui_ReportsWidgetHealthcheck.h"

#include "core/AsyncTask.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/PasswordHealth.h"
#include "core/Resources.h"
#include "gui/styles/StateColorPalette.h"

#include <QSharedPointer>
#include <QStandardItemModel>

namespace
{
    class Health
    {
    public:
        struct Item
        {
            QPointer<const Group> group;
            QPointer<const Entry> entry;
            QSharedPointer<PasswordHealth> health;

            Item(const Group* g, const Entry* e, QSharedPointer<PasswordHealth> h)
                : group(g)
                , entry(e)
                , health(h)
            {
            }

            bool operator<(const Item& rhs) const
            {
                return health->score() < rhs.health->score();
            }
        };

        explicit Health(QSharedPointer<Database>);

        const QList<QSharedPointer<Item>>& items() const
        {
            return m_items;
        }

    private:
        QSharedPointer<Database> m_db;
        HealthChecker m_checker;
        QList<QSharedPointer<Item>> m_items;
    };
} // namespace

Health::Health(QSharedPointer<Database> db)
    : m_db(db)
    , m_checker(db)
{
    for (const auto* group : db->rootGroup()->groupsRecursive(true)) {
        // Skip recycle bin
        if (group->isRecycled()) {
            continue;
        }

        for (const auto* entry : group->entries()) {
            if (entry->isRecycled()) {
                continue;
            }

            // Skip entries with empty password
            if (entry->password().isEmpty()) {
                continue;
            }

            // Add entry if its password isn't at least "good"
            const auto item = QSharedPointer<Item>(new Item(group, entry, m_checker.evaluate(entry)));
            if (item->health->quality() < PasswordHealth::Quality::Good) {
                m_items.append(item);
            }
        }
    }

    // Sort the result so that the worst passwords (least score)
    // are at the top
    std::sort(m_items.begin(), m_items.end(), [](QSharedPointer<Item> x, QSharedPointer<Item> y) { return *x < *y; });
}

ReportsWidgetHealthcheck::ReportsWidgetHealthcheck(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ReportsWidgetHealthcheck())
    , m_errorIcon(Resources::instance()->icon("dialog-error"))
{
    m_ui->setupUi(this);

    m_referencesModel.reset(new QStandardItemModel());
    m_ui->healthcheckTableView->setModel(m_referencesModel.data());
    m_ui->healthcheckTableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_ui->healthcheckTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(m_ui->healthcheckTableView, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
}

ReportsWidgetHealthcheck::~ReportsWidgetHealthcheck()
{
}

void ReportsWidgetHealthcheck::addHealthRow(QSharedPointer<PasswordHealth> health,
                                            const Group* group,
                                            const Entry* entry)
{
    QString descr, tip;
    QColor qualityColor;
    StateColorPalette statePalette;
    const auto quality = health->quality();
    switch (quality) {
    case PasswordHealth::Quality::Bad:
        descr = tr("Bad", "Password quality");
        tip = tr("Bad — password must be changed");
        qualityColor = statePalette.color(StateColorPalette::HealthCritical);
        break;

    case PasswordHealth::Quality::Poor:
        descr = tr("Poor", "Password quality");
        tip = tr("Poor — password should be changed");
        qualityColor = statePalette.color(StateColorPalette::HealthBad);
        break;

    case PasswordHealth::Quality::Weak:
        descr = tr("Weak", "Password quality");
        tip = tr("Weak — consider changing the password");
        qualityColor = statePalette.color(StateColorPalette::HealthWeak);
        break;

    case PasswordHealth::Quality::Good:
    case PasswordHealth::Quality::Excellent:
        qualityColor = statePalette.color(StateColorPalette::HealthOk);
        break;
    }

    auto row = QList<QStandardItem*>();
    row << new QStandardItem(descr);
    row << new QStandardItem(entry->iconPixmap(), entry->title());
    row << new QStandardItem(group->iconPixmap(), group->hierarchy().join("/"));
    row << new QStandardItem(QString::number(health->score()));
    row << new QStandardItem(health->scoreReason());

    // Set background color of first column according to password quality.
    // Set the same as foreground color so the description is usually
    // invisible, it's just for screen readers etc.
    QBrush brush(qualityColor);
    row[0]->setForeground(brush);
    row[0]->setBackground(brush);

    // Set tooltips
    row[0]->setToolTip(tip);
    row[4]->setToolTip(health->scoreDetails());

    // Store entry pointer per table row (used in double click handler)
    m_referencesModel->appendRow(row);
    m_rowToEntry.append({group, entry});
}

void ReportsWidgetHealthcheck::loadSettings(QSharedPointer<Database> db)
{
    m_db = std::move(db);
    m_healthCalculated = false;
    m_referencesModel->clear();
    m_rowToEntry.clear();

    auto row = QList<QStandardItem*>();
    row << new QStandardItem(tr("Please wait, health data is being calculated..."));
    m_referencesModel->appendRow(row);
}

void ReportsWidgetHealthcheck::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (!m_healthCalculated) {
        // Perform stats calculation on next event loop to allow widget to appear
        m_healthCalculated = true;
        QTimer::singleShot(0, this, SLOT(calculateHealth()));
    }
}

void ReportsWidgetHealthcheck::calculateHealth()
{
    m_referencesModel->clear();

    const QScopedPointer<Health> health(AsyncTask::runAndWaitForFuture([this] { return new Health(m_db); }));
    if (health->items().empty()) {
        // No findings
        m_referencesModel->clear();
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Congratulations, everything is healthy!"));
    } else {
        // Show our findings
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("") << tr("Title") << tr("Path") << tr("Score")
                                                                   << tr("Reason"));
        for (const auto& item : health->items()) {
            addHealthRow(item->health, item->group, item->entry);
        }
    }

    m_ui->healthcheckTableView->resizeRowsToContents();
}

void ReportsWidgetHealthcheck::emitEntryActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    const auto row = m_rowToEntry[index.row()];
    const auto group = row.first;
    const auto entry = row.second;
    if (group && entry) {
        emit entryActivated(const_cast<Entry*>(entry));
    }
}

void ReportsWidgetHealthcheck::saveSettings()
{
    // nothing to do - the tab is passive
}

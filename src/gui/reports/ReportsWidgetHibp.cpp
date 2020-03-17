/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "ReportsWidgetHibp.h"
#include "ui_ReportsWidgetHibp.h"

#include "core/Database.h"
#include "core/Group.h"
#include "gui/MessageBox.h"

#include <QStandardItemModel>

namespace {
    /*
     * Convert the number of times a password has been pwned into
     * a display text for the third table column.
     */
    QString toDisplayText(int count) {
        if (count==1)               return QObject::tr("once");
        if (count <= 10)            return QObject::tr("10 times");
        if (count <= 100)           return QObject::tr("100 times");
        if (count <= 1000)          return QObject::tr("1000 times");
        if (count <= 10 * 1000)     return QObject::tr("10,000 times");
        if (count <= 100 * 1000)    return QObject::tr("100,000 times");
        if (count <= 1000 * 1000)   return QObject::tr("a million times");

        return QObject::tr("millions of times");
    }
}

ReportsWidgetHibp::ReportsWidgetHibp(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ReportsWidgetHibp())
{
    m_ui->setupUi(this);

    m_referencesModel.reset(new QStandardItemModel());
    m_ui->hibpTableView->setModel(m_referencesModel.data());
    m_ui->hibpTableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_ui->hibpTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(m_ui->hibpTableView, SIGNAL(doubleClicked(QModelIndex)), SLOT(emitEntryActivated(QModelIndex)));
}

ReportsWidgetHibp::~ReportsWidgetHibp()
{
}

void ReportsWidgetHibp::loadSettings(QSharedPointer<Database> db)
{
    // Re-initialize
    m_db = std::move(db);
    m_checkStarted = false;
    m_referencesModel->clear();
    m_pwQueue.clear();
    m_pwPwned.clear();
    m_error.clear();
    m_rowToEntry.clear();
    m_edEntry = nullptr;
    m_ui->progressBar->hide();

    // Collect all passwords in the database (unless recycled, and
    // unless empty).
    for (const auto* group : m_db->rootGroup()->groupsRecursive(true)) {
        if (!group->isRecycled()) {
            for (const auto* entry : group->entries()) {
                if (!entry->isRecycled() && !entry->password().isEmpty()) {
                    m_pwQueue << entry->password();
                }
            }
        }
    }

    // Store the number of passwords we need to check for the
    // progress bar
    m_qMax = m_pwQueue.size();
}

/*
 * Fill the table will all entries that have passwords that we've
 * found to have been pwned.
 */
void ReportsWidgetHibp::makeHibpTable()
{
    // Reset the table
    m_referencesModel->clear();
    m_referencesModel->setHorizontalHeaderLabels(QStringList()
                                                 << tr("Title") << tr("Path") << tr("Password exposed …"));
    m_rowToEntry.clear();

    // Search database for passwords that we've found so far
    QVector<QPair<const Entry*, int>> items;
    for (const auto* group : m_db->rootGroup()->groupsRecursive(true)) {
        if (!group->isRecycled()) {
            for (const auto* entry : group->entries()) {
                if (!entry->isRecycled()) {
                    const auto found = m_pwPwned.find(entry->password());
                    if (found != m_pwPwned.end()) {
                        items += qMakePair(entry, *found);
                    }
                }
            }
        }
    }

    // Sort decending by the number the password has been exposed
    qSort(
        items.begin(),
        items.end(),
        [](QPair<const Entry*, int>& lhs,
           QPair<const Entry*, int>& rhs) { return lhs.second > rhs.second; });

    // Build the table
    for (const auto& item : items) {
        const auto entry = item.first;
        const auto group = entry->group();
        const auto count = item.second;

        auto row = QList<QStandardItem*>();
        row << new QStandardItem(entry->iconPixmap(), entry->title())
            << new QStandardItem(group->iconPixmap(), group->hierarchy().join("/"))
            << new QStandardItem(toDisplayText(count));
        m_referencesModel->appendRow(row);
        row[2]->setForeground(QBrush(QColor("red")));

        // Store entry pointer per table row (used in double click handler)
        m_rowToEntry.append(entry);
    }

    // If there was an error, append the error message to the table
    if (!m_error.isEmpty()) {
        auto row = QList<QStandardItem*>();
        row << new QStandardItem(m_error);
        m_referencesModel->appendRow(row);
        row[0]->setForeground(QBrush(QColor("red")));
    }

    // If we're done and everything is good, display a motivational message
    if (m_pwQueue.isEmpty() && m_pwPwned.isEmpty() && m_error.isEmpty()) {
        m_referencesModel->clear();
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Congratulations, no exposed passwords!"));
    }

    m_ui->hibpTableView->resizeRowsToContents();
}

/*
 * Invoked when the downloader has finished checking one password.
 */
void ReportsWidgetHibp::checkFinished(const QString& password, int count)
{
    // If the password has been pwned:
    if (count > 0) {

        // Add the password to the list of our findings
        m_pwPwned[password] = count;

        // Refresh the findings table
        makeHibpTable();
    }

    // Remove this password from the to-do list
    if (m_pwQueue.remove(password)) {
        // Something was removed, so check the next password
        checkNext();
    }
}

/*
 * Invoked when a query to the HIBP server fails.
 *
 * Displays the table with the current findings and quits the online activity.
 */
void ReportsWidgetHibp::checkFailed(const QString& /*password*/, const QString& error)
{
    m_error = error;

#ifdef WITH_XC_NETWORKING
    m_pwQueue.clear();
    m_downloader.reset();
    m_ui->progressBar->hide();
    makeHibpTable();
#endif
}

/*
 * Run the background validator on the next password in the queue.
 */
void ReportsWidgetHibp::checkNext()
{
#ifdef WITH_XC_NETWORKING
    // If nothing's left to do, we're done
    if (m_pwQueue.empty()) {
        m_downloader.reset();
        m_ui->progressBar->hide();
        makeHibpTable();
        return;
    }

    // We're not set done, set up the progress bar
    m_ui->progressBar->setValue(m_qMax - m_pwQueue.size());
    m_ui->progressBar->setMaximum(m_qMax);
    // Note that we're not un-hiding the progress bar after the
    // initial pass through the password queue is done, i. e.
    // when re-validating a modified password after the user
    // editied an entry. We want that re-validation to happen
    // invisibly, popping the progress bar back into view would
    // just be visually disturbing.

    // Handle the next password
    m_downloader.reset(new HibpDownloader(*m_pwQueue.begin()));
    connect(&*m_downloader, SIGNAL(finished(const QString&, int)), this, SLOT(checkFinished(const QString&, int)));
    connect(&*m_downloader,
            SIGNAL(failed(const QString&, const QString&)),
            this,
            SLOT(checkFailed(const QString&, const QString&)));
#endif
}

void ReportsWidgetHibp::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // Do this only the first time
    if (m_checkStarted) {
        return;
    }

#ifdef WITH_XC_NETWORKING

    const auto ans = MessageBox::question(this,
                                          tr("Online Access Confirmation"),
                                          tr("Your passwords are about to be validated against an online database "
                                             "(https://haveibeenpwned.com/). Some information about your passwords "
                                             "(first five bytes of the SHA1 hash) will be sent to an Internet service. "
                                             "It is not possible to reconstruct your passwords from this information, "
                                             "yet certain information (e. g. number of passwords or your IP address) "
                                             "will be exposed. "
                                             "Are you sure you want to continue?"),
                                          MessageBox::Yes | MessageBox::No,
                                          MessageBox::No);
    if (ans == MessageBox::Yes) {

        // OK, do it
        m_checkStarted = true;
        m_ui->progressBar->show();
        checkNext();

    } else {

        // Don't do it
        m_referencesModel->clear();
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Online validation rejected."));
    }

#else

    // Compiled without networking, can't do anything
    MessageBox::critical(this,
                         tr("Online Access"),
                         tr("This build of KeePassXC doesn't contain online functions."),
                         MessageBox::Ok,
                         MessageBox::Ok);
    m_referencesModel->clear();
    m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Online validation not available."));

#endif
}

/*
 * Double-click handler
 */
void ReportsWidgetHibp::emitEntryActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    // Find which database entry was double-clicked
    const auto entry = m_rowToEntry[index.row()];
    if (entry) {
        // Found it, invoke entry editor
        m_edEntry = entry;
        m_edPw = entry->password();
        emit entryActivated(this, const_cast<Entry*>(entry));
    }
}

/*
 * Invoked after "OK" was clicked in the entry editor.
 * Re-validates the edited entry's new password.
 */
void ReportsWidgetHibp::refreshAfterEdit()
{
    // Sanity check
    if (!m_edEntry)
        return;

    // No need to re-validate if there was no change
    if (m_edEntry->password() == m_edPw)
        return;

    // Remove the previous password from the list of findings
    m_pwPwned.remove(m_edPw);

    // Is the downloader still running?
    const auto mustRun = m_pwQueue.empty();

    // Add the entry's new password to the queue
    m_pwQueue << m_edEntry->password();

    // Restart the downloader if it's not running yet
    if (mustRun) {
        checkNext();
    }
}

void ReportsWidgetHibp::saveSettings()
{
    // nothing to do - the tab is passive
}

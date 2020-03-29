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

#include "config-keepassx.h"
#include "core/Database.h"
#include "core/Group.h"
#include "gui/MessageBox.h"

#include <QStandardItemModel>

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
#ifdef WITH_XC_NETWORKING
    connect(&m_downloader, SIGNAL(hibpResult(QString, int)), SLOT(addHibpResult(QString, int)));
    connect(&m_downloader, SIGNAL(fetchFailed(QString, QString)), SLOT(fetchFailed(QString, QString)));

    connect(m_ui->validationButton, &QPushButton::pressed, [this] { startValidation(); });
#endif
}

ReportsWidgetHibp::~ReportsWidgetHibp()
{
}

void ReportsWidgetHibp::loadSettings(QSharedPointer<Database> db)
{
    // Re-initialize
    m_db = std::move(db);
    m_referencesModel->clear();
    m_pwndPasswords.clear();
    m_error.clear();
    m_rowToEntry.clear();
    m_editedEntry = nullptr;
#ifdef WITH_XC_NETWORKING
    m_ui->stackedWidget->setCurrentIndex(0);
    m_ui->validationButton->setEnabled(true);
    m_ui->progressBar->hide();
#else
    // Compiled without networking, can't do anything
    m_ui->stackedWidget->setCurrentIndex(2);
#endif
}

/*
 * Fill the table will all entries that have passwords that we've
 * found to have been pwned.
 */
void ReportsWidgetHibp::makeHibpTable()
{
    // Reset the table
    m_referencesModel->clear();
    m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Path") << tr("Password exposed…"));
    m_rowToEntry.clear();

    // Search database for passwords that we've found so far
    QList<QPair<const Entry*, int>> items;
    for (const auto* entry : m_db->rootGroup()->entriesRecursive()) {
        if (!entry->isRecycled()) {
            const auto found = m_pwndPasswords.find(entry->password());
            if (found != m_pwndPasswords.end()) {
                items.append({entry, found.value()});
            }
        }
    }

    // Sort decending by the number the password has been exposed
    qSort(items.begin(), items.end(), [](QPair<const Entry*, int>& lhs, QPair<const Entry*, int>& rhs) {
        return lhs.second > rhs.second;
    });

    // Build the table
    for (const auto& item : items) {
        const auto entry = item.first;
        const auto group = entry->group();
        const auto count = item.second;

        auto row = QList<QStandardItem*>();
        row << new QStandardItem(entry->iconPixmap(), entry->title())
            << new QStandardItem(group->iconPixmap(), group->hierarchy().join("/"))
            << new QStandardItem(countToText(count));
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
#ifdef WITH_XC_NETWORKING
    if (m_downloader.passwordsRemaining() == 0 && m_pwndPasswords.isEmpty() && m_error.isEmpty()) {
        m_referencesModel->clear();
        m_referencesModel->setHorizontalHeaderLabels(QStringList() << tr("Congratulations, no exposed passwords!"));
    }
#endif

    m_ui->hibpTableView->resizeRowsToContents();

    m_ui->stackedWidget->setCurrentIndex(1);
}

/*
 * Invoked when the downloader has finished checking one password.
 */
void ReportsWidgetHibp::addHibpResult(const QString& password, int count)
{
    // Add the password to the list of our findings if it has been pwned
    if (count > 0) {
        m_pwndPasswords[password] = count;
    }

#ifdef WITH_XC_NETWORKING
    // Update the progress bar
    int remaining = m_downloader.passwordsRemaining();
    if (remaining > 0) {
        m_ui->progressBar->setValue(m_ui->progressBar->maximum() - remaining);
    } else {
        // Finished, remove the progress bar and build the table
        m_ui->progressBar->hide();
        makeHibpTable();
    }
#endif
}

/*
 * Invoked when a query to the HIBP server fails.
 *
 * Displays the table with the current findings.
 */
void ReportsWidgetHibp::fetchFailed(const QString& /*password*/, const QString& error)
{
    m_error = error;
    m_ui->progressBar->hide();
    makeHibpTable();
}

/*
 * Add passwords to the downloader and start the actual online validation.
 */
void ReportsWidgetHibp::startValidation()
{
#ifdef WITH_XC_NETWORKING
    // Collect all passwords in the database (unless recycled, and
    // unless empty) and submit them to the downloader.
    for (const auto* entry : m_db->rootGroup()->entriesRecursive()) {
        if (!entry->isRecycled() && !entry->password().isEmpty()) {
            m_downloader.add(entry->password());
        }
    }

    // Store the number of passwords we need to check for the progress bar
    m_ui->progressBar->show();
    m_ui->progressBar->setMaximum(m_downloader.passwordsToValidate());
    m_ui->validationButton->setEnabled(false);

    m_downloader.validate();
#endif
}

/*
 * Convert the number of times a password has been pwned into
 * a display text for the third table column.
 */
QString ReportsWidgetHibp::countToText(int count)
{
    if (count == 1) {
        return tr("once");
    } else if (count <= 10) {
        return tr("up to 10 times");
    } else if (count <= 100) {
        return tr("up to 100 times");
    } else if (count <= 1000) {
        return tr("up to 1000 times");
    } else if (count <= 10000) {
        return tr("up to 10,000 times");
    } else if (count <= 100000) {
        return tr("up to 100,000 times");
    } else if (count <= 1000000) {
        return tr("up to a million times");
    }

    return tr("millions of times");
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
        m_editedEntry = entry;
        m_editedPassword = entry->password();
        emit entryActivated(const_cast<Entry*>(entry));
    }
}

/*
 * Invoked after "OK" was clicked in the entry editor.
 * Re-validates the edited entry's new password.
 */
void ReportsWidgetHibp::refreshAfterEdit()
{
    // Sanity check
    if (!m_editedEntry) {
        return;
    }

    // No need to re-validate if there was no change
    if (m_editedEntry->password() == m_editedPassword) {
        return;
    }

    // Remove the previous password from the list of findings
    m_pwndPasswords.remove(m_editedPassword);

    // Validate the new password against HIBP
#ifdef WITH_XC_NETWORKING
    m_downloader.add(m_editedEntry->password());
    m_downloader.validate();
#endif

    m_editedEntry = nullptr;
}

void ReportsWidgetHibp::saveSettings()
{
    // nothing to do - the tab is passive
}

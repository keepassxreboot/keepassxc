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

#include "SearchWidget.h"
#include "ui_SearchHelpWidget.h"
#include "ui_SearchWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QShortcut>
#include <QToolButton>

#include "core/Config.h"
#include "gui/Icons.h"
#include "gui/widgets/PopupHelpWidget.h"

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SearchWidget())
    , m_searchTimer(new QTimer(this))
    , m_clearSearchTimer(new QTimer(this))
{
    m_ui->setupUi(this);
    setFocusProxy(m_ui->searchEdit);

    m_helpWidget = new PopupHelpWidget(m_ui->searchEdit);
    Ui::SearchHelpWidget helpUi;
    helpUi.setupUi(m_helpWidget);

    m_searchTimer->setSingleShot(true);
    m_clearSearchTimer->setSingleShot(true);

    connect(m_ui->searchEdit, SIGNAL(textChanged(QString)), SLOT(startSearchTimer()));
    connect(m_ui->helpIcon, SIGNAL(triggered()), SLOT(toggleHelp()));
    connect(m_ui->searchIcon, SIGNAL(triggered()), SLOT(showSearchMenu()));
    connect(m_searchTimer, SIGNAL(timeout()), SLOT(startSearch()));
    connect(m_clearSearchTimer, SIGNAL(timeout()), m_ui->searchEdit, SLOT(clear()));
    connect(this, SIGNAL(escapePressed()), m_ui->searchEdit, SLOT(clear()));

    new QShortcut(QKeySequence::Find, this, SLOT(searchFocus()), nullptr, Qt::ApplicationShortcut);
    new QShortcut(Qt::Key_Escape, m_ui->searchEdit, SLOT(clear()), nullptr, Qt::ApplicationShortcut);

    m_ui->searchEdit->setPlaceholderText(tr("Search (%1)...", "Search placeholder text, %1 is the keyboard shortcut")
                                             .arg(QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
    m_ui->searchEdit->installEventFilter(this);

    m_searchMenu = new QMenu(this);
    m_actionCaseSensitive = m_searchMenu->addAction(tr("Case sensitive"), this, SLOT(updateCaseSensitive()));
    m_actionCaseSensitive->setObjectName("actionSearchCaseSensitive");
    m_actionCaseSensitive->setCheckable(true);

    m_actionLimitGroup = m_searchMenu->addAction(tr("Limit search to selected group"), this, SLOT(updateLimitGroup()));
    m_actionLimitGroup->setObjectName("actionSearchLimitGroup");
    m_actionLimitGroup->setCheckable(true);
    m_actionLimitGroup->setChecked(config()->get(Config::SearchLimitGroup).toBool());

    m_ui->searchIcon->setIcon(icons()->icon("system-search"));
    m_ui->searchEdit->addAction(m_ui->searchIcon, QLineEdit::LeadingPosition);

    m_ui->helpIcon->setIcon(icons()->icon("system-help"));
    m_ui->searchEdit->addAction(m_ui->helpIcon, QLineEdit::TrailingPosition);

    // Fix initial visibility of actions (bug in Qt)
    for (QToolButton* toolButton : m_ui->searchEdit->findChildren<QToolButton*>()) {
        toolButton->setVisible(toolButton->defaultAction()->isVisible());
    }
}

SearchWidget::~SearchWidget()
{
}

bool SearchWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit escapePressed();
            return true;
        } else if (keyEvent->matches(QKeySequence::Copy)) {
            // If Control+C is pressed in the search edit when no text
            // is selected, copy the password of the current entry.
            if (!m_ui->searchEdit->hasSelectedText()) {
                emit copyPressed();
                return true;
            }
        } else if (keyEvent->matches(QKeySequence::MoveToNextLine)) {
            if (m_ui->searchEdit->cursorPosition() == m_ui->searchEdit->text().length()) {
                // If down is pressed at EOL, move the focus to the entry view
                emit downPressed();
                return true;
            } else {
                // Otherwise move the cursor to EOL
                m_ui->searchEdit->setCursorPosition(m_ui->searchEdit->text().length());
                return true;
            }
        }
    } else if (event->type() == QEvent::FocusOut && !m_ui->searchEdit->text().isEmpty()) {
        if (config()->get(Config::Security_ClearSearch).toBool()) {
            int timeout = config()->get(Config::Security_ClearSearchTimeout).toInt();
            if (timeout > 0) {
                // Auto-clear search after set timeout (5 minutes by default)
                m_clearSearchTimer->start(timeout * 60000); // 60 sec * 1000 ms
            }
        }
    } else if (event->type() == QEvent::FocusIn) {
        // Never clear the search if we are using it
        m_clearSearchTimer->stop();
    }

    return QWidget::eventFilter(obj, event);
}

void SearchWidget::connectSignals(SignalMultiplexer& mx)
{
    // Connects basically only to the current DatabaseWidget, but allows to switch between instances!
    mx.connect(this, SIGNAL(search(QString)), SLOT(search(QString)));
    mx.connect(this, SIGNAL(caseSensitiveChanged(bool)), SLOT(setSearchCaseSensitive(bool)));
    mx.connect(this, SIGNAL(limitGroupChanged(bool)), SLOT(setSearchLimitGroup(bool)));
    mx.connect(this, SIGNAL(copyPressed()), SLOT(copyPassword()));
    mx.connect(this, SIGNAL(downPressed()), SLOT(focusOnEntries()));
    mx.connect(SIGNAL(clearSearch()), m_ui->searchEdit, SLOT(clear()));
    mx.connect(SIGNAL(entrySelectionChanged()), this, SLOT(resetSearchClearTimer()));
    mx.connect(SIGNAL(currentModeChanged(DatabaseWidget::Mode)), this, SLOT(resetSearchClearTimer()));
    mx.connect(SIGNAL(databaseUnlocked()), this, SLOT(searchFocus()));
    mx.connect(m_ui->searchEdit, SIGNAL(returnPressed()), SLOT(switchToEntryEdit()));
}

void SearchWidget::databaseChanged(DatabaseWidget* dbWidget)
{
    if (dbWidget != nullptr) {
        // Set current search text from this database
        m_ui->searchEdit->setText(dbWidget->getCurrentSearch());
        // Enforce search policy
        emit caseSensitiveChanged(m_actionCaseSensitive->isChecked());
        emit limitGroupChanged(m_actionLimitGroup->isChecked());
    } else {
        m_ui->searchEdit->clear();
    }
}

void SearchWidget::startSearchTimer()
{
    if (!m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }
    m_searchTimer->start(100);
}

void SearchWidget::startSearch()
{
    if (!m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }

    search(m_ui->searchEdit->text());
}

void SearchWidget::resetSearchClearTimer()
{
    // Restart the search clear timer if it is running
    if (m_clearSearchTimer->isActive()) {
        m_clearSearchTimer->start();
    }
}

void SearchWidget::updateCaseSensitive()
{
    emit caseSensitiveChanged(m_actionCaseSensitive->isChecked());
}

void SearchWidget::setCaseSensitive(bool state)
{
    m_actionCaseSensitive->setChecked(state);
    updateCaseSensitive();
}

void SearchWidget::updateLimitGroup()
{
    config()->set(Config::SearchLimitGroup, m_actionLimitGroup->isChecked());
    emit limitGroupChanged(m_actionLimitGroup->isChecked());
}

void SearchWidget::setLimitGroup(bool state)
{
    m_actionLimitGroup->setChecked(state);
    updateLimitGroup();
}

void SearchWidget::searchFocus()
{
    m_ui->searchEdit->setFocus();
    m_ui->searchEdit->selectAll();
}

void SearchWidget::toggleHelp()
{
    if (m_helpWidget->isVisible()) {
        m_helpWidget->hide();
    } else {
        m_helpWidget->show();
    }
}

void SearchWidget::showSearchMenu()
{
    m_searchMenu->exec(m_ui->searchEdit->mapToGlobal(m_ui->searchEdit->rect().bottomLeft()));
}

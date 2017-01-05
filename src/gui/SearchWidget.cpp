/*
 *  Copyright (C) 2016 Jonathan White <support@dmapps.us>
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
#include "ui_SearchWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QShortcut>

#include "core/FilePath.h"

SearchWidget::SearchWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SearchWidget())
{
    m_ui->setupUi(this);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);

    connect(m_ui->searchEdit, SIGNAL(textChanged(QString)), SLOT(startSearchTimer()));
    connect(m_ui->searchIcon, SIGNAL(triggered(QAction*)), m_ui->searchEdit, SLOT(setFocus()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));
    connect(this, SIGNAL(escapePressed()), m_ui->searchEdit, SLOT(clear()));

    new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(searchFocus()), nullptr, Qt::ApplicationShortcut);

    m_ui->searchEdit->installEventFilter(this);

    QMenu *searchMenu = new QMenu();
    m_actionCaseSensitive = searchMenu->addAction(tr("Case Sensitive"), this, SLOT(updateCaseSensitive()));
    m_actionCaseSensitive->setObjectName("actionSearchCaseSensitive");
    m_actionCaseSensitive->setCheckable(true);

    m_ui->searchIcon->setIcon(filePath()->icon("actions", "system-search"));
    m_ui->searchIcon->setMenu(searchMenu);
    m_ui->searchIcon->setPopupMode(QToolButton::MenuButtonPopup);
}

SearchWidget::~SearchWidget()
{

}

bool SearchWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit escapePressed();
            return true;
        }
        else if (keyEvent->matches(QKeySequence::Copy)) {
            // If Control+C is pressed in the search edit when no
            // text is selected, copy the password of the current
            // entry.
            if (!m_ui->searchEdit->hasSelectedText()) {
                emit copyPressed();
                return true;
            }
        }
        else if (keyEvent->matches(QKeySequence::MoveToNextLine)) {
            // If Down is pressed move the focus to the entry view.
            emit downPressed();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void SearchWidget::connectSignals(SignalMultiplexer& mx)
{
    mx.connect(this, SIGNAL(search(QString)), SLOT(search(QString)));
    mx.connect(this, SIGNAL(caseSensitiveChanged(bool)), SLOT(setSearchCaseSensitive(bool)));
    mx.connect(this, SIGNAL(copyPressed()), SLOT(copyPassword()));
    mx.connect(this, SIGNAL(downPressed()), SLOT(setFocus()));
    mx.connect(m_ui->searchEdit, SIGNAL(returnPressed()), SLOT(switchToEntryEdit()));
}

void SearchWidget::databaseChanged(DatabaseWidget *dbWidget)
{
    if (dbWidget != nullptr) {
        // Set current search text from this database
        m_ui->searchEdit->setText(dbWidget->getCurrentSearch());

        // Enforce search policy
        emit caseSensitiveChanged(m_actionCaseSensitive->isChecked());
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

void SearchWidget::updateCaseSensitive()
{
    emit caseSensitiveChanged(m_actionCaseSensitive->isChecked());
}

void SearchWidget::setCaseSensitive(bool state)
{
    m_actionCaseSensitive->setChecked(state);
    updateCaseSensitive();
}

void SearchWidget::searchFocus()
{
    m_ui->searchEdit->setFocus();
    m_ui->searchEdit->selectAll();
}

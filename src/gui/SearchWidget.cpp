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

bool SearchEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit escapePressed();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}


SearchWidget::SearchWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SearchWidget())
{
    m_ui->setupUi(this);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);

    connect(m_ui->searchEdit, SIGNAL(textChanged(QString)), SLOT(startSearchTimer()));
    connect(m_ui->searchEdit, SIGNAL(returnPressed()), SLOT(startSearch()));
    connect(m_ui->searchIcon, SIGNAL(triggered(QAction*)), m_ui->searchEdit, SLOT(setFocus()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));
    connect(&m_searchEventFilter, SIGNAL(escapePressed()), m_ui->searchEdit, SLOT(clear()));

    new QShortcut(Qt::CTRL + Qt::Key_F, m_ui->searchEdit, SLOT(setFocus()), nullptr, Qt::ApplicationShortcut);

    m_ui->searchEdit->installEventFilter(&m_searchEventFilter);

    QMenu *searchMenu = new QMenu();
    m_actionCaseSensitive = searchMenu->addAction(tr("Case Sensitive"), this, SLOT(updateCaseSensitive()));
    m_actionCaseSensitive->setCheckable(true);

    m_actionGroupSearch = searchMenu->addAction(tr("Search Current Group"), this, SLOT(updateGroupSearch()));
    m_actionGroupSearch->setCheckable(true);

    m_ui->searchIcon->setIcon(filePath()->icon("actions", "system-search"));
    m_ui->searchIcon->setMenu(searchMenu);
    m_ui->searchIcon->setPopupMode(QToolButton::MenuButtonPopup);
}

SearchWidget::~SearchWidget()
{

}

void SearchWidget::connectSignals(SignalMultiplexer& mx)
{
    mx.connect(this, SIGNAL(search(QString)), SLOT(search(QString)));
    mx.connect(this, SIGNAL(setCaseSensitive(bool)), SLOT(setSearchCaseSensitive(bool)));
    mx.connect(this, SIGNAL(setGroupSearch(bool)), SLOT(setSearchCurrentGroup(bool)));
    mx.connect(SIGNAL(groupChanged()), m_ui->searchEdit, SLOT(clear()));
}

void SearchWidget::databaseChanged(DatabaseWidget *dbWidget)
{
    if (dbWidget != nullptr) {
        // Set current search text from this database
        m_ui->searchEdit->setText(dbWidget->getCurrentSearch());

        // Enforce search policy
        emit setCaseSensitive(m_actionCaseSensitive->isChecked());
        emit setGroupSearch(m_actionGroupSearch->isChecked());
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
    emit setCaseSensitive(m_actionCaseSensitive->isChecked());
}

void SearchWidget::updateGroupSearch()
{
    emit setGroupSearch(m_actionGroupSearch->isChecked());
}

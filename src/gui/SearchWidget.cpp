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
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));
    connect(&m_searchEventFilter, SIGNAL(escapePressed()), m_ui->searchEdit, SLOT(clear()));

    m_ui->searchEdit->installEventFilter(&m_searchEventFilter);

    m_ui->searchIcon->setIcon(filePath()->icon("actions", "system-search"));

    /*
    connect(m_searchUi->caseSensitiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    connect(m_searchUi->searchCurrentRadioButton, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    connect(m_searchUi->searchRootRadioButton, SIGNAL(toggled(bool)), this, SLOT(startSearch()));
    */
}

SearchWidget::~SearchWidget()
{

}

void SearchWidget::connectSignals(SignalMultiplexer& mx)
{
    mx.connect(this, SIGNAL(search(QString)), SLOT(search(QString)));
}

void SearchWidget::hideEvent(QHideEvent *event)
{
    // TODO: might be better to disable the edit widget instead of clearing it
    //m_ui->searchEdit->clear();
    QWidget::hideEvent(event);
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

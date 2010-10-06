/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "EditEntryWidget.h"
#include "ui_EditEntryWidget.h"
#include "ui_EditEntryWidgetMain.h"
#include "ui_EditEntryWidgetNotes.h"

#include <QtGui/QListWidget>
#include <QtGui/QStackedLayout>

#include "core/Entry.h"
#include "core/Group.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : QWidget(parent)
    , m_entry(0)
    , m_ui(new Ui::EditEntryWidget())
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_notesUi(new Ui::EditEntryWidgetNotes())
    , m_mainWidget(new QWidget(this))
    , m_notesWidget(new QWidget(this))
{
    m_ui->setupUi(this);

    QFont headerLabelFont = m_ui->headerLabel->font();
    headerLabelFont.setBold(true);
    headerLabelFont.setPointSize(headerLabelFont.pointSize() + 2);
    m_ui->headerLabel->setFont(headerLabelFont);

    m_ui->categoryList->addItem(tr("Entry"));
    m_ui->categoryList->addItem(tr("Description"));

    m_mainUi->setupUi(m_mainWidget);
    m_ui->stackedWidget->addWidget(m_mainWidget);

    m_notesUi->setupUi(m_notesWidget);
    m_ui->stackedWidget->addWidget(m_notesWidget);

    Q_ASSERT(m_ui->categoryList->model()->rowCount() == m_ui->stackedWidget->count());

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveEntry()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

EditEntryWidget::~EditEntryWidget()
{
}

void EditEntryWidget::loadEntry(Entry* entry)
{
    m_entry = entry;

    m_ui->headerLabel->setText(m_entry->group()->name()+" > "+tr("Edit entry"));

    m_mainUi->titleEdit->setText(entry->title());
    m_mainUi->usernameEdit->setText(entry->username());
    m_mainUi->urlEdit->setText(entry->url());

    m_notesUi->notesEdit->setPlainText(entry->notes());

    m_ui->categoryList->setCurrentRow(0);
}

void EditEntryWidget::saveEntry()
{
    m_entry->setTitle(m_mainUi->titleEdit->text());
    m_entry->setUsername(m_mainUi->usernameEdit->text());
    m_entry->setUrl(m_mainUi->urlEdit->text());

    m_entry->setNotes(m_notesUi->notesEdit->toPlainText());

    cancel();
}

void EditEntryWidget::cancel()
{
    m_entry = 0;
    Q_EMIT editFinished();
}

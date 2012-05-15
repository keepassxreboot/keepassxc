/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "EditWidget.h"
#include "ui_EditWidget.h"

EditWidget::EditWidget(QWidget* parent)
    : DialogyWidget(parent)
    ,  m_ui(new Ui::EditWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)),
            m_ui->stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SIGNAL(accepted()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SIGNAL(rejected()));
}

EditWidget::~EditWidget()
{
}

void EditWidget::add(const QString& labelText, QWidget* widget)
{
    m_ui->categoryList->addItem(labelText);
    m_ui->stackedWidget->addWidget(widget);
}

void EditWidget::setCurrentRow(int index)
{
    m_ui->categoryList->setCurrentRow(index);
}

QLabel* EditWidget::headlineLabel()
{
    return m_ui->headerLabel;
}

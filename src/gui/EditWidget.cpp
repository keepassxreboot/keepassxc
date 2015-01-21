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
    , m_ui(new Ui::EditWidget())
{
    m_ui->setupUi(this);
    setReadOnly(false);

    m_ui->messageWidget->setHidden(true);

    QFont headerLabelFont = m_ui->headerLabel->font();
    headerLabelFont.setBold(true);
    headerLabelFont.setPointSize(headerLabelFont.pointSize() + 2);
    headlineLabel()->setFont(headerLabelFont);

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

void EditWidget::setRowHidden(QWidget* widget, bool hide)
{
    int row = m_ui->stackedWidget->indexOf(widget);
    if (row != -1) {
        m_ui->categoryList->item(row)->setHidden(hide);
    }
}

void EditWidget::setCurrentRow(int index)
{
    m_ui->categoryList->setCurrentRow(index);
}

void EditWidget::setHeadline(const QString& text)
{
    m_ui->headerLabel->setText(text);
}

QLabel* EditWidget::headlineLabel()
{
    return m_ui->headerLabel;
}

void EditWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;

    if (readOnly) {
        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    }
    else {
        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    }
}

bool EditWidget::readOnly() const
{
    return m_readOnly;
}

void EditWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    m_ui->messageWidget->showMessage(text, type);
}

void EditWidget::hideMessage()
{
    if (m_ui->messageWidget->isVisible()) {
        m_ui->messageWidget->animatedHide();
    }
}

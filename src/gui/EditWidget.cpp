/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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
#include <QPushButton>
#include <QScrollArea>

#include "core/FilePath.h"

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
    headlineLabel()->setTextFormat(Qt::PlainText);

    connect(m_ui->categoryList, SIGNAL(categoryChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SIGNAL(accepted()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SIGNAL(rejected()));
}

EditWidget::~EditWidget()
{
}

void EditWidget::addPage(const QString& labelText, const QIcon& icon, QWidget* widget)
{
    /*
     * Instead of just adding a widget we're going to wrap it into a scroll area. It will automatically show
     * scrollbars when the widget cannot fit into the page. This approach prevents the main window of the application
     * from automatic resizing and it now should be able to fit into a user's monitor even if the monitor is only 768
     * pixels high.
     */
    QScrollArea* scrollArea = new QScrollArea(m_ui->stackedWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(widget);
    scrollArea->setWidgetResizable(true);
    m_ui->stackedWidget->addWidget(scrollArea);
    m_ui->categoryList->addCategory(labelText, icon);
}

void EditWidget::setPageHidden(QWidget* widget, bool hidden)
{
    int index = m_ui->stackedWidget->indexOf(widget);
    if (index != -1) {
        m_ui->categoryList->setCategoryHidden(index, hidden);
    }

    if (index == m_ui->stackedWidget->currentIndex()) {
        int newIndex = m_ui->stackedWidget->currentIndex() - 1;
        if (newIndex < 0) {
            newIndex = m_ui->stackedWidget->count() - 1;
        }
        m_ui->stackedWidget->setCurrentIndex(newIndex);
    }
}

void EditWidget::setCurrentPage(int index)
{
    m_ui->categoryList->setCurrentCategory(index);
    m_ui->stackedWidget->setCurrentIndex(index);
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
    } else {
        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
        // Find and connect the apply button
        QPushButton* applyButton = m_ui->buttonBox->button(QDialogButtonBox::Apply);
        connect(applyButton, SIGNAL(clicked()), SIGNAL(apply()));
    }
}

bool EditWidget::readOnly() const
{
    return m_readOnly;
}

void EditWidget::enableApplyButton(bool enabled)
{
    QPushButton* applyButton = m_ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyButton) {
        applyButton->setEnabled(enabled);
    }
}

void EditWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    // Show error messages for a longer time to make sure the user can read them
    if (type == MessageWidget::Error) {
        m_ui->messageWidget->setCloseButtonVisible(true);
        m_ui->messageWidget->showMessage(text, type, 15000);
    } else {
        m_ui->messageWidget->setCloseButtonVisible(false);
        m_ui->messageWidget->showMessage(text, type, 2000);
    }
}

void EditWidget::hideMessage()
{
    if (m_ui->messageWidget->isVisible()) {
        m_ui->messageWidget->animatedHide();
    }
}

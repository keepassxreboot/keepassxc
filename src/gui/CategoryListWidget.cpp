/*
 * Copyright (C) 2017 KeePassXC Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CategoryListWidget.h"
#include "ui_CategoryListWidget.h"

#include <QListWidget>
#include <QScrollBar>
#include <QSize>
#include <QStyledItemDelegate>
#include <QPainter>

CategoryListWidget::CategoryListWidget(QWidget* parent)
    : QWidget(parent),
      m_ui(new Ui::CategoryListWidget())
{
    m_ui->setupUi(this);
    m_ui->categoryList->setItemDelegate(new CategoryListWidgetDelegate(this));

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)), SLOT(emitCategoryChanged(int)));

    connect(m_ui->scrollUp, SIGNAL(clicked()), SLOT(scrollCategoriesUp()));
    connect(m_ui->scrollDown, SIGNAL(clicked()), SLOT(scrollCategoriesDown()));
    connect(m_ui->categoryList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(updateCategoryScrollButtons()));
    connect(m_ui->categoryList->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), SLOT(updateCategoryScrollButtons()));
}

CategoryListWidget::~CategoryListWidget()
{
}

QSize CategoryListWidget::sizeHint() const
{
    QSize sizeHint = QWidget::sizeHint();

    int width = m_ui->categoryList->sizeHintForColumn(0) + m_ui->categoryList->frameWidth() * 2;
    if (m_ui->categoryList->verticalScrollBar()->isVisible()) {
        width += m_ui->categoryList->verticalScrollBar()->width();
    }
    sizeHint.setWidth(width);

    return sizeHint;
}

QSize CategoryListWidget::minimumSizeHint() const
{
    return QSize(sizeHint().width(), m_ui->categoryList->sizeHintForRow(0) * 2);
}

int CategoryListWidget::addCategory(const QString& labelText, const QIcon& icon)
{
    QListWidgetItem* item = new QListWidgetItem(m_ui->categoryList);
    item->setText(labelText);
    item->setIcon(icon);
    m_ui->categoryList->addItem(item);
    return m_ui->categoryList->count() - 1;
}

void CategoryListWidget::removeCategory(int index)
{
    m_ui->categoryList->removeItemWidget(m_ui->categoryList->item(index));
}

int CategoryListWidget::currentCategory()
{
    return m_ui->categoryList->currentRow();
}

void CategoryListWidget::setCurrentCategory(int index)
{
    m_ui->categoryList->setCurrentRow(index);
}

void CategoryListWidget::setCategoryHidden(int index, bool hidden)
{
    m_ui->categoryList->item(index)->setHidden(hidden);
}

bool CategoryListWidget::isCategoryHidden(int index)
{
    return m_ui->categoryList->item(index)->isHidden();
}

void CategoryListWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateCategoryScrollButtons();
}

void CategoryListWidget::updateCategoryScrollButtons()
{
    m_ui->scrollUp->setEnabled(m_ui->categoryList->verticalScrollBar()->value() != 0);
    m_ui->scrollDown->setEnabled(m_ui->categoryList->verticalScrollBar()->value()
                                 != m_ui->categoryList->verticalScrollBar()->maximum());

    m_ui->scrollUp->setVisible(m_ui->categoryList->verticalScrollBar()->maximum() > 0);
    m_ui->scrollDown->setVisible(m_ui->scrollUp->isVisible());
}

void CategoryListWidget::scrollCategoriesUp()
{
    m_ui->categoryList->verticalScrollBar()->setValue(
        m_ui->categoryList->verticalScrollBar()->value() - m_ui->categoryList->verticalScrollBar()->pageStep()
    );
}

void CategoryListWidget::scrollCategoriesDown()
{
    m_ui->categoryList->verticalScrollBar()->setValue(
        m_ui->categoryList->verticalScrollBar()->value() + m_ui->categoryList->verticalScrollBar()->pageStep()
    );
}

void CategoryListWidget::emitCategoryChanged(int index)
{
    emit categoryChanged(index);
}


/* =============================================================================================== */


CategoryListWidgetDelegate::CategoryListWidgetDelegate(QWidget* parent)
    : QStyledItemDelegate(parent),
      m_size(96, 96)
{}

void CategoryListWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();

    QIcon icon = opt.icon;
    QSize iconSize = opt.icon.actualSize(QSize(32, 32));
    opt.icon = QIcon();
    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    opt.decorationPosition = QStyleOptionViewItem::Top;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QRect fontRect = painter->fontMetrics().boundingRect(
      QRect(0, 0, m_size.width(), m_size.height()), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, opt.text);

    int paddingTop = fontRect.height() < 30 ? 15 : 10;
    int left = opt.rect.left() + opt.rect.width() / 2 - iconSize.width() / 2;
    painter->drawPixmap(left, opt.rect.top() + paddingTop, icon.pixmap(iconSize));

    painter->restore();
}

QSize CategoryListWidgetDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return m_size;
}

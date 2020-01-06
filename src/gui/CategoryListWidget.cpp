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
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSize>
#include <QStyledItemDelegate>

CategoryListWidget::CategoryListWidget(QWidget* parent)
    : QWidget(parent)
    , m_itemDelegate(nullptr)
    , m_ui(new Ui::CategoryListWidget())
{
    m_ui->setupUi(this);
    m_itemDelegate = new CategoryListWidgetDelegate(m_ui->categoryList);
    m_ui->categoryList->setItemDelegate(m_itemDelegate);

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)), SLOT(emitCategoryChanged(int)));

    connect(m_ui->scrollUp, SIGNAL(clicked()), SLOT(scrollCategoriesUp()));
    connect(m_ui->scrollDown, SIGNAL(clicked()), SLOT(scrollCategoriesDown()));
    connect(m_ui->categoryList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(updateCategoryScrollButtons()));
    // clang-format off
    connect(m_ui->categoryList->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), SLOT(updateCategoryScrollButtons()));
    // clang-format on
}

CategoryListWidget::~CategoryListWidget()
{
}

QSize CategoryListWidget::sizeHint() const
{
    QSize sizeHint = QWidget::sizeHint();

    int width = m_ui->categoryList->width();

    int min = minimumSizeHint().width();
    if (width < min) {
        width = min;
    }
    sizeHint.setWidth(width);

    return sizeHint;
}

QSize CategoryListWidget::minimumSizeHint() const
{
    return QSize(m_itemDelegate->minWidth() + m_ui->categoryList->frameWidth() * 2,
                 m_ui->categoryList->sizeHintForRow(0) * 2);
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

void CategoryListWidget::resizeEvent(QResizeEvent* event)
{
    auto newDelegate = new CategoryListWidgetDelegate(m_ui->categoryList);
    m_ui->categoryList->setItemDelegate(newDelegate);
    m_itemDelegate->deleteLater();
    m_itemDelegate = newDelegate;

    QWidget::resizeEvent(event);
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
    m_ui->categoryList->verticalScrollBar()->setValue(m_ui->categoryList->verticalScrollBar()->value()
                                                      - m_ui->categoryList->verticalScrollBar()->pageStep());
}

void CategoryListWidget::scrollCategoriesDown()
{
    m_ui->categoryList->verticalScrollBar()->setValue(m_ui->categoryList->verticalScrollBar()->value()
                                                      + m_ui->categoryList->verticalScrollBar()->pageStep());
}

void CategoryListWidget::emitCategoryChanged(int index)
{
    emit categoryChanged(index);
}

/* =============================================================================================== */

CategoryListWidgetDelegate::CategoryListWidgetDelegate(QListWidget* parent)
    : QStyledItemDelegate(parent)
    , m_listWidget(parent)
    , m_size(96, 96)
{
    m_size.setWidth(minWidth());
    if (m_listWidget && m_listWidget->width() > m_size.width()) {
        m_size.setWidth(m_listWidget->width());
    }
}

class IconSelectionCorrectedStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption* option,
                       QPainter* painter,
                       const QWidget* widget) const override
    {
        painter->save();

        if (PE_PanelItemViewItem == element) {
            // Qt on Windows and the Fusion/Phantom base styles draw selection backgrounds only for
            // the actual text/icon bounding box, not over the full width of a list item.
            // We therefore need to translate and stretch the painter before we can
            // tell Qt to draw its native styles.
            // Since we are scaling horizontally, we also need to move the right and left
            // edge pixels outside the drawing area to avoid thick border lines.
            QRect itemRect = subElementRect(QStyle::SE_ItemViewItemFocusRect, option, widget).adjusted(1, 0, 1, 0);
            painter->scale(static_cast<float>(option->rect.width()) / itemRect.width(), 1.0);
            painter->translate(option->rect.left() - itemRect.left() + 1, 0);
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);

        painter->restore();
    }
};

void CategoryListWidgetDelegate::paint(QPainter* painter,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();

    QIcon icon = opt.icon;
    QSize iconSize = opt.icon.actualSize(QSize(ICON_SIZE, ICON_SIZE));
    opt.icon = QIcon();
    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    opt.decorationPosition = QStyleOptionViewItem::Top;

    QScopedPointer<QStyle> style(new IconSelectionCorrectedStyle());
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QRect fontRect = painter->fontMetrics().boundingRect(
        QRect(0, 0, minWidth(), m_size.height()), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, opt.text);

    int paddingTop = fontRect.height() < 30 ? 15 : 10;
    int left = opt.rect.left() + opt.rect.width() / 2 - iconSize.width() / 2;
    painter->drawPixmap(left, opt.rect.top() + paddingTop, icon.pixmap(iconSize));

    painter->restore();
}

int CategoryListWidgetDelegate::minWidth() const
{
    int c = m_listWidget->count();
    int maxWidth = 0;

    for (int i = 0; i < c; ++i) {
        QFontMetrics fm(m_listWidget->font());
        QRect fontRect =
            fm.boundingRect(QRect(0, 0, 0, 0), Qt::TextWordWrap | Qt::ElideNone, m_listWidget->item(i)->text());

        if (fontRect.width() > maxWidth) {
            maxWidth = fontRect.width();
        }
    }

    // add some padding
    maxWidth += 10;

    return maxWidth < m_size.height() ? m_size.height() : maxWidth;
}

QSize CategoryListWidgetDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    int w = minWidth();
    if (m_listWidget->width() > w) {
        w = m_listWidget->width();
    }

    return QSize(w, m_size.height());
}

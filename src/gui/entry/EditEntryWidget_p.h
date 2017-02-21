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

#ifndef KEEPASSX_EDITENTRYWIDGET_P_H
#define KEEPASSX_EDITENTRYWIDGET_P_H

#include <QListWidget>
#include <QScrollBar>
#include <QSize>
#include <QStyledItemDelegate>
#include <QPainter>

class CategoryListViewDelegate : public QStyledItemDelegate
{
public:
    explicit CategoryListViewDelegate(QListView* parent = nullptr)
        : QStyledItemDelegate(parent), m_size(96, 96)
    {}

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
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

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return m_size;
    }

private:
    QSize m_size;
};

class CategoryListWidget : public QListWidget
{
public:
    explicit CategoryListWidget(QWidget* parent = 0)
        : QListWidget(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setItemDelegate(new CategoryListViewDelegate(this));
        setMovement(QListView::Static);
        setViewMode(QListWidget::IconMode);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setWordWrap(true);
    }
protected:
    QSize sizeHint() const override
    {
        QSize sizeHint = QListWidget::sizeHint();

        int width = sizeHintForColumn(0) + frameWidth() * 2;
        if (verticalScrollBar()->isVisible()) {
            width += verticalScrollBar()->width();
        }
        sizeHint.setWidth(width);

        return sizeHint;
    }

    QSize minimumSizeHint() const override
    {
        return QSize(sizeHint().width(), sizeHintForRow(0) * 2);
    }
};

class AttributesListView : public QListView
{
public:
    explicit AttributesListView(QWidget* parent = 0) : QListView(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    }

    QSize sizeHint() const override
    {
        QSize sizeHint = QListView::sizeHint();

        int width = sizeHintForColumn(0) + frameWidth() * 2;
        if (verticalScrollBar()->isVisible()) {
            width += verticalScrollBar()->width();
        }
        sizeHint.setWidth(width);

        return sizeHint;
    }
};

#endif // KEEPASSX_EDITENTRYWIDGET_P_H

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

class CategoryListViewDelegate : public QStyledItemDelegate
{
public:
    explicit CategoryListViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 22));
        return size;
    }
};

class CategoryListWidget : public QListWidget
{
public:
    explicit CategoryListWidget(QWidget* parent = 0) : QListWidget(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setItemDelegate(new CategoryListViewDelegate(this));
    }

    virtual QSize sizeHint() const
    {
        QSize sizeHint = QListWidget::sizeHint();

        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        if (verticalScrollBar()->isVisible()) {
            width += verticalScrollBar()->width();
        }
        sizeHint.setWidth(width);

        return sizeHint;
    }
};

class AttributesListView : public QListView
{
public:
    explicit AttributesListView(QWidget* parent = 0) : QListView(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setItemDelegate(new CategoryListViewDelegate(this));
    }

    virtual QSize sizeHint() const
    {
        QSize sizeHint = QListView::sizeHint();

        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        if (verticalScrollBar()->isVisible()) {
            width += verticalScrollBar()->width();
        }
        sizeHint.setWidth(width);

        return sizeHint;
    }
};

#endif // KEEPASSX_EDITENTRYWIDGET_P_H

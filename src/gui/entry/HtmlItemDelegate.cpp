/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "HtmlItemDelegate.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>

HtmlItemDelegate::HtmlItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void HtmlItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString markdownToDisplay(opt.text);

    // Draw control without text (includes drawing background)
    opt.text = QString();
    QStyle* style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);

    // Setup document
    QTextDocument doc;
    doc.setHtml(markdownToDisplay);

    // Configure text color (should change when item is selected)
    QAbstractTextDocumentLayout::PaintContext ctx;
    if (opt.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::HighlightedText));
    } else {
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::Text));
    }

    // Draw document
    painter->save();
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt);
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

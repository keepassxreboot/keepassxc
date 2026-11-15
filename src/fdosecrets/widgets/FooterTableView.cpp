/*
 *  Copyright (C) 2026 Aetf <aetf@unlimitedcodeworks.xyz>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FooterTableView.h"

#include <QHeaderView>

namespace
{
    // rows shown without scrolling before the view asks for no more height
    constexpr int MaxHintRows = 5;
    // characters of average width before the view asks for no more width
    constexpr int MaxHintChars = 80;
} // namespace

FooterTableView::FooterTableView(QWidget* parent)
    : QTableView(parent)
{
}

void FooterTableView::setFooter(QWidget* footer)
{
    if (m_footer) {
        m_footer->deleteLater();
    }
    m_footer = footer;
    if (m_footer) {
        m_footer->setParent(this);
        m_footer->show();
    }
    updateGeometry();
    updateGeometries();
}

int FooterTableView::footerHeight() const
{
    return m_footer && !m_footer->isHidden() ? m_footer->sizeHint().height() : 0;
}

QSize FooterTableView::sizeForRows(int rows) const
{
    const auto header = horizontalHeader()->isHidden() ? 0 : horizontalHeader()->sizeHint().height();
    // as wide as the columns need, within reason: a cell is elided rather
    // than allowed to decide how wide a window gets
    int width = verticalHeader()->isHidden() ? 0 : verticalHeader()->sizeHint().width();
    const auto columns = model() ? model()->columnCount(rootIndex()) : 0;
    for (int column = 0; column != columns; ++column) {
        if (!isColumnHidden(column)) {
            width += qMax(sizeHintForColumn(column), horizontalHeader()->sectionSizeHint(column));
        }
    }
    width = qMin(width, MaxHintChars * fontMetrics().averageCharWidth());
    if (footerHeight() > 0) {
        width = qMax(width, m_footer->sizeHint().width());
    }
    return {width + 2 * frameWidth(),
            verticalHeader()->defaultSectionSize() * rows + header + footerHeight() + 2 * frameWidth()};
}

QSize FooterTableView::sizeHint() const
{
    const auto rows = model() ? model()->rowCount(rootIndex()) : 0;
    return sizeForRows(qBound(1, rows, MaxHintRows));
}

QSize FooterTableView::minimumSizeHint() const
{
    return sizeForRows(1);
}

void FooterTableView::updateGeometries()
{
    // the base class lays out the headers and resets the margins to theirs
    QTableView::updateGeometries();
    if (!m_footer) {
        return;
    }

    const auto height = footerHeight();
    auto margins = viewportMargins();
    if (margins.bottom() != height) {
        margins.setBottom(height);
        setViewportMargins(margins);
    }
    const auto area = viewport()->geometry();
    m_footer->setGeometry(area.left(), area.bottom() + 1, area.width(), height);
}

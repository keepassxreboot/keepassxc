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

#ifndef KEEPASSXC_FDOSECRETS_FOOTERTABLEVIEW_H
#define KEEPASSXC_FDOSECRETS_FOOTERTABLEVIEW_H

#include <QPointer>
#include <QTableView>

/**
 * A table view with a widget pinned below its rows, inside its frame: the
 * footer stays in place while the rows scroll. It lives in the bottom
 * viewport margin, the way the headers live in the top and left ones.
 *
 * The view asks for the size of its contents, up to a few rows and a
 * reasonable width, instead of the fixed default of item views, and gives
 * any further height to the rows.
 */
class FooterTableView : public QTableView
{
    Q_OBJECT

public:
    explicit FooterTableView(QWidget* parent = nullptr);

    /// The view takes ownership of @a footer.
    void setFooter(QWidget* footer);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void updateGeometries() override;

private:
    int footerHeight() const;
    QSize sizeForRows(int rows) const;

    QPointer<QWidget> m_footer;
};

#endif // KEEPASSXC_FDOSECRETS_FOOTERTABLEVIEW_H

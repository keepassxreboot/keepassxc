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

#ifndef KEEPASSXC_TAGLABEL_H
#define KEEPASSXC_TAGLABEL_H

#include <QLabel>

/**
 * A label in the form of a tag. It is filled like the tags of TagsEdit
 * unless another fill is set.
 */
class TagLabel : public QLabel
{
    Q_OBJECT

public:
    explicit TagLabel(QWidget* parent = nullptr);

    void setFill(const QBrush& fill);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QBrush m_fill;
};

#endif // KEEPASSXC_TAGLABEL_H

/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DARKSTYLE_H
#define KEEPASSXC_DARKSTYLE_H

#include "gui/styles/base/BaseStyle.h"
#include <QApplication>

class DarkStyle : public BaseStyle
{
    Q_OBJECT

public:
    DarkStyle();
    QPalette standardPalette() const override;

    using BaseStyle::polish;
    void polish(QWidget* widget) override;

protected:
    QString getAppStyleSheet() const override;
};

#endif // KEEPASSXC_DARKSTYLE_H

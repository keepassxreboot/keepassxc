/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DIALOGYWIDGET_H
#define KEEPASSX_DIALOGYWIDGET_H

#include <QDialogButtonBox>

class DialogyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DialogyWidget(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* e) override;

private:
    bool clickButton(QDialogButtonBox::StandardButton standardButton);
};

#endif // KEEPASSX_DIALOGYWIDGET_H

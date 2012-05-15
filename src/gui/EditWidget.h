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

#ifndef KEEPASSX_EDITWIDGET_H
#define KEEPASSX_EDITWIDGET_H

#include <QtCore/QScopedPointer>
#include <QtGui/QLabel>

#include "gui/DialogyWidget.h"

namespace Ui {
    class EditWidget;
}

class EditWidget : public DialogyWidget
{
    Q_OBJECT

public:
    explicit EditWidget(QWidget* parent = 0);
    ~EditWidget();

    void add(const QString& labelText, QWidget* widget);
    void setCurrentRow(int index);
    QLabel* headlineLabel();

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    const QScopedPointer<Ui::EditWidget> m_ui;

    Q_DISABLE_COPY(EditWidget)
};

#endif // KEEPASSX_EDITWIDGET_H

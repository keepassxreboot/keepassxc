/*
 *  Copyright (C) 2015 Pedro Alves <devel@pgalves.com>
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

#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include "gui/kmessagewidget.h"

class MessageWidget : public KMessageWidget
{
    Q_OBJECT

public:
    explicit MessageWidget(QWidget* parent = 0);

public Q_SLOTS:
    void showMessage(const QString& text, MessageWidget::MessageType type);
    void hideMessage();

};

#endif // MESSAGEWIDGET_H

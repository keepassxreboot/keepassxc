/*
 *  Copyright (C) 2015 Felix Geyer <devel@pgalves.com>
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

#include "MessageWidget.h"

MessageWidget::MessageWidget(QWidget* parent)
    :KMessageWidget(parent)
{

}

void MessageWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    setMessageType(type);
    setText(text);
    animatedShow();
}

void MessageWidget::hideMessage()
{
    animatedHide();
}

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

#include "DialogyWidget.h"

#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#include <QtGui/QKeyEvent>
#include <QtCore/QDebug>

DialogyWidget::DialogyWidget(QWidget* parent)
    : QWidget(parent)
{
}

void DialogyWidget::keyPressEvent(QKeyEvent *e)
{
#ifdef Q_WS_MAC
    if(e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
        if (!clickButton(QDialogButtonBox::Cancel)) {
            e->ignore();
        }
    }
    else
#endif
    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
        switch (e->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (!clickButton(QDialogButtonBox::Ok)) {
                    e->ignore();
                }
                break;
            case Qt::Key_Escape:
                if (!clickButton(QDialogButtonBox::Cancel)) {
                    e->ignore();
                }
                break;
            default:
                e->ignore();
        }
    }
    else {
        e->ignore();
    }
}

bool DialogyWidget::clickButton(QDialogButtonBox::StandardButton button)
{
    Q_ASSERT(button == QDialogButtonBox::Ok || button == QDialogButtonBox::Cancel);

    QList<QDialogButtonBox*> buttonBoxes = findChildren<QDialogButtonBox*>();
    for (int i = 0; i < buttonBoxes.size(); ++i) {
        QDialogButtonBox* buttonBox = buttonBoxes.at(i);
        QPushButton* pb;
        QPushButton* pbCancel = buttonBox->button(QDialogButtonBox::Cancel);
        if (button == QDialogButtonBox::Ok) {
            if (pbCancel && pbCancel->isVisible() && pbCancel->isEnabled() && pbCancel->hasFocus()) {
                pbCancel->click();
                return true;
            }
            pb = buttonBox->button(button);
        }
        else {
            pb = pbCancel;
        }

        if (pb) {
            if (pb->isVisible() && pb->isEnabled()) {
                pb->click();
                return true;
            }
        }
    }
    return false;
}

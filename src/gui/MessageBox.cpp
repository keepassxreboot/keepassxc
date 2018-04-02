/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "MessageBox.h"

QMessageBox::StandardButton MessageBox::m_nextAnswer(QMessageBox::NoButton);

QMessageBox::StandardButton MessageBox::critical(QWidget* parent,
                                                 const QString& title,
                                                 const QString& text,
                                                 QMessageBox::StandardButtons buttons,
                                                 QMessageBox::StandardButton defaultButton)
{
    if (m_nextAnswer == QMessageBox::NoButton) {
        return QMessageBox::critical(parent, title, text, buttons, defaultButton);
    } else {
        QMessageBox::StandardButton returnButton = m_nextAnswer;
        m_nextAnswer = QMessageBox::NoButton;
        return returnButton;
    }
}

QMessageBox::StandardButton MessageBox::information(QWidget* parent,
                                                    const QString& title,
                                                    const QString& text,
                                                    QMessageBox::StandardButtons buttons,
                                                    QMessageBox::StandardButton defaultButton)
{
    if (m_nextAnswer == QMessageBox::NoButton) {
        return QMessageBox::information(parent, title, text, buttons, defaultButton);
    } else {
        QMessageBox::StandardButton returnButton = m_nextAnswer;
        m_nextAnswer = QMessageBox::NoButton;
        return returnButton;
    }
}

QMessageBox::StandardButton MessageBox::question(QWidget* parent,
                                                 const QString& title,
                                                 const QString& text,
                                                 QMessageBox::StandardButtons buttons,
                                                 QMessageBox::StandardButton defaultButton)
{
    if (m_nextAnswer == QMessageBox::NoButton) {
        return QMessageBox::question(parent, title, text, buttons, defaultButton);
    } else {
        QMessageBox::StandardButton returnButton = m_nextAnswer;
        m_nextAnswer = QMessageBox::NoButton;
        return returnButton;
    }
}

QMessageBox::StandardButton MessageBox::warning(QWidget* parent,
                                                const QString& title,
                                                const QString& text,
                                                QMessageBox::StandardButtons buttons,
                                                QMessageBox::StandardButton defaultButton)
{
    if (m_nextAnswer == QMessageBox::NoButton) {
        return QMessageBox::warning(parent, title, text, buttons, defaultButton);
    } else {
        QMessageBox::StandardButton returnButton = m_nextAnswer;
        m_nextAnswer = QMessageBox::NoButton;
        return returnButton;
    }
}

void MessageBox::setNextAnswer(QMessageBox::StandardButton button)
{
    m_nextAnswer = button;
}

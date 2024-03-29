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

#include "ShortcutWidget.h"

#include <QKeyEvent>
#include <QToolTip>

ShortcutWidget::ShortcutWidget(QWidget* parent)
    : QLineEdit(parent)
{
    setReadOnly(true);
}

Qt::Key ShortcutWidget::key() const
{
    return m_key;
}

Qt::KeyboardModifiers ShortcutWidget::modifiers() const
{
    return m_modifiers;
}

QKeySequence ShortcutWidget::sequence() const
{
    return (m_key == Qt::Key_unknown) ? QKeySequence() : QKeySequence(m_key | m_modifiers);
}

void ShortcutWidget::setShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    m_key = key;
    m_modifiers = modifiers;
    m_locked = true;

    displayShortcut(m_key, m_modifiers);
    emit shortcutChanged(m_key, m_modifiers);
}

void ShortcutWidget::resetShortcut()
{
    m_key = Qt::Key_unknown;
    m_modifiers = Qt::NoModifier;
    m_locked = false;
    emit shortcutReset();
}

void ShortcutWidget::keyPressEvent(QKeyEvent* event)
{
    keyEvent(event);
}

void ShortcutWidget::keyReleaseEvent(QKeyEvent* event)
{
    keyEvent(event);
}

void ShortcutWidget::keyEvent(QKeyEvent* event)
{
    event->accept();

    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease) {
        return;
    }

    bool release = (event->type() == QEvent::KeyRelease);

    if (m_locked && release) {
        return;
    }

    auto key = static_cast<Qt::Key>(event->key());

    if (key <= 0 || key == Qt::Key_unknown) {
        return;
    }

    Qt::KeyboardModifiers modifiers = event->modifiers() & (Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META);

    bool keyIsModifier;
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
        keyIsModifier = true;
        break;
    default:
        keyIsModifier = false;
    }

    if (!release && !keyIsModifier) {
        if (modifiers != 0) {
            setShortcut(key, modifiers);
        } else {
            resetShortcut();
            displayShortcut(key, modifiers);
        }
    } else {
        if (m_locked) {
            resetShortcut();
        }

        displayShortcut(static_cast<Qt::Key>(0), modifiers);
    }
}

void ShortcutWidget::displayShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    setText(QKeySequence(key | modifiers).toString(QKeySequence::NativeText));
}

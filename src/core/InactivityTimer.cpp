/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

#include "InactivityTimer.h"

#include <QCoreApplication>
#include <QTimer>

namespace
{
    // Minimum timeout is 10 seconds
    constexpr int MIN_TIMEOUT = 10000;
} // namespace

InactivityTimer::InactivityTimer(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(false);
    connect(m_timer, SIGNAL(timeout()), SLOT(timeout()));
}

void InactivityTimer::activate(int inactivityTimeout)
{
    if (!m_active) {
        qApp->installEventFilter(this);
    }
    m_active = true;
    m_resetBlocked = false;
    m_timer->setInterval(qMax(MIN_TIMEOUT, inactivityTimeout));
    m_timer->start();
}

void InactivityTimer::deactivate()
{
    qApp->removeEventFilter(this);
    m_active = false;
    m_timer->stop();
}

bool InactivityTimer::eventFilter(QObject* watched, QEvent* event)
{
    const auto type = event->type();
    // clang-format off
    if (!m_resetBlocked && 
        ((type >= QEvent::MouseButtonPress && type <= QEvent::KeyRelease) ||
         (type >= QEvent::HoverEnter && type <= QEvent::HoverMove) ||
          type == QEvent::Wheel)) {
        m_timer->start();
        m_resetBlocked = true;
        QTimer::singleShot(500, this, [this]() { m_resetBlocked = false; });
    }
    // clang-format on

    return QObject::eventFilter(watched, event);
}

void InactivityTimer::timeout()
{
    // make sure we don't emit the signal a second time while it's still processed
    if (!m_emitMutx.tryLock()) {
        return;
    }

    if (m_active) {
        emit inactivityDetected();
    }

    m_emitMutx.unlock();
}

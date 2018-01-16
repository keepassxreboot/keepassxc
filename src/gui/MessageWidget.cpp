/*
 *  Copyright (C) 2015 Pedro Alves <devel@pgalves.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "QTimer"

const int MessageWidget::DefaultAutoHideTimeout = 6000;
const int MessageWidget::DisableAutoHide = -1;

MessageWidget::MessageWidget(QWidget* parent)
    : KMessageWidget(parent)
    , m_autoHideTimer(new QTimer(this))
    , m_autoHideTimeout(DefaultAutoHideTimeout)
{
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, SIGNAL(timeout()), this, SLOT(animatedHide()));
    connect(this, SIGNAL(hideAnimationFinished()), m_autoHideTimer, SLOT(stop()));
}

int MessageWidget::autoHideTimeout() const
{
    return m_autoHideTimeout;
}

void MessageWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    showMessage(text, type, m_autoHideTimeout);
}

void MessageWidget::showMessage(const QString &text, KMessageWidget::MessageType type, int autoHideTimeout)
{
    setMessageType(type);
    setText(text);
    emit showAnimationStarted();
    animatedShow();
    if (autoHideTimeout > 0) {
        m_autoHideTimer->start(autoHideTimeout);
    } else {
        m_autoHideTimer->stop();
    }
}

void MessageWidget::hideMessage()
{
    animatedHide();
    m_autoHideTimer->stop();
}

void MessageWidget::setAutoHideTimeout(int autoHideTimeout)
{
    m_autoHideTimeout = autoHideTimeout;
    if (autoHideTimeout <= 0) {
        m_autoHideTimer->stop();
    }
}

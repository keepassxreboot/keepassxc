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

#include <QDesktopServices>
#include <QTimer>
#include <QUrl>

const int MessageWidget::DefaultAutoHideTimeout = 6000;
const int MessageWidget::LongAutoHideTimeout = 15000;
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

void MessageWidget::setAnimate(bool state)
{
    m_animate = state;
}

int MessageWidget::autoHideTimeout() const
{
    return m_autoHideTimeout;
}

void MessageWidget::showMessage(const QString& text, MessageWidget::MessageType type)
{
    showMessage(text, type, m_autoHideTimeout);
}

void MessageWidget::showMessage(const QString& text, KMessageWidget::MessageType type, int autoHideTimeout)
{
    setMessageType(type);
    setText(text);

    emit showAnimationStarted();
    if (m_animate) {
        animatedShow();
    } else {
        show();
        emit showAnimationFinished();
    }

    if (autoHideTimeout > 0) {
        m_autoHideTimer->start(autoHideTimeout);
    } else {
        m_autoHideTimer->stop();
    }
}

void MessageWidget::hideMessage()
{
    emit hideAnimationStarted();
    if (m_animate) {
        animatedHide();
    } else {
        hide();
        emit hideAnimationFinished();
    }

    m_autoHideTimer->stop();
}

void MessageWidget::setAutoHideTimeout(int autoHideTimeout)
{
    m_autoHideTimeout = autoHideTimeout;
    if (autoHideTimeout <= 0) {
        m_autoHideTimer->stop();
    }
}

/**
 * Open a link using the system's default handler.
 * Links that are not HTTP(S) links are ignored.
 *
 * @param link link URL
 */
void MessageWidget::openHttpUrl(const QString& link)
{
    if (link.startsWith("http://") || link.startsWith("https://")) {
        QDesktopServices::openUrl(QUrl(link));
    }
}

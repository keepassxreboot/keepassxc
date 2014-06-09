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

#include "Clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QTimer>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#include "core/Config.h"

Clipboard* Clipboard::m_instance(Q_NULLPTR);

Clipboard::Clipboard(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), SLOT(clearClipboard()));
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(cleanup()));
}

void Clipboard::setText(const QString& text)
{
    QClipboard* clipboard = QApplication::clipboard();

    clipboard->setText(text, QClipboard::Clipboard);
    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
    }

    if (config()->get("security/clearclipboard").toBool()) {
        int timeout = config()->get("security/clearclipboardtimeout").toInt();
        if (timeout > 0) {
            m_lastCopied = text;
            m_timer->start(timeout * 1000);
        }
    }
}

void Clipboard::clearClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();

    if (!clipboard) {
        qWarning("Unable to access the clipboard.");
        return;
    }

    if (clipboard->text(QClipboard::Clipboard) == m_lastCopied) {
        clipboard->clear(QClipboard::Clipboard);
    }

    if (clipboard->supportsSelection()
            && (clipboard->text(QClipboard::Selection) == m_lastCopied)) {
        clipboard->clear(QClipboard::Selection);
    }

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QDBusMessage message = QDBusMessage::createMethodCall("org.kde.klipper", "/klipper", "", "clearClipboardHistory");
    QDBusConnection::sessionBus().send(message);
#endif

    m_lastCopied.clear();
}

void Clipboard::cleanup()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        clearClipboard();
    }
}

Clipboard* Clipboard::instance()
{
    if (!m_instance) {
        m_instance = new Clipboard(qApp);
    }

    return m_instance;
}

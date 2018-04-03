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

#ifndef KEEPASSX_CLIPBOARD_H
#define KEEPASSX_CLIPBOARD_H

#include <QObject>
#ifdef Q_OS_MAC
#include "core/MacPasteboard.h"
#include <QPointer>
#endif

class QTimer;

class Clipboard : public QObject
{
    Q_OBJECT

public:
    void setText(const QString& text);

    static Clipboard* instance();

public slots:
    void clearCopiedText();

private slots:
    void clearClipboard();

private:
    explicit Clipboard(QObject* parent = nullptr);

    static Clipboard* m_instance;

    QTimer* m_timer;
#ifdef Q_OS_MAC
    // This object lives for the whole program lifetime and we cannot delete it on exit,
    // so ignore leak warnings. See https://bugreports.qt.io/browse/QTBUG-54832
    static QPointer<MacPasteboard> m_pasteboard;
#endif
    QString m_lastCopied;
};

inline Clipboard* clipboard()
{
    return Clipboard::instance();
}

#endif // KEEPASSX_CLIPBOARD_H

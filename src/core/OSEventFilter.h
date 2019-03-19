/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OSEVENTFILTER_H
#define OSEVENTFILTER_H
#include <QAbstractNativeEventFilter>

class QByteArray;

class OSEventFilter : public QAbstractNativeEventFilter
{
public:
    OSEventFilter();
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    Q_DISABLE_COPY(OSEventFilter)
};

#endif // OSEVENTFILTER_H

/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
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

#ifndef QTOTP_H
#define QTOTP_H

#include <QtCore/qglobal.h>

class QUrl;

class QTotp
{
public:
    QTotp();
    static QString parseOtpString(QString rawSecret, quint8& digits, quint8& step);
    static QString generateTotp(const QByteArray key, quint64 time, const quint8 numDigits, const quint8 step);
    static QUrl generateOtpString(const QString& secret,
                                  const QString& type,
                                  const QString& issuer,
                                  const QString& username,
                                  const QString& algorithm,
                                  const quint8& digits,
                                  const quint8& step);
    static const quint8 defaultStep;
    static const quint8 defaultDigits;
};

#endif // QTOTP_H

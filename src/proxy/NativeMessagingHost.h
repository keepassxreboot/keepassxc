/*
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef NATIVEMESSAGINGHOST_H
#define NATIVEMESSAGINGHOST_H

#include "NativeMessagingBase.h"

class NativeMessagingHost : public NativeMessagingBase
{
    Q_OBJECT
public:
    NativeMessagingHost();
    ~NativeMessagingHost();

public slots:
    void newLocalMessage();
    void deleteSocket();
    void socketStateChanged(QLocalSocket::LocalSocketState socketState);

private:
    void readLength();
    void readStdIn(const quint32 length);

private:
    QLocalSocket* m_localSocket;
};

#endif // NATIVEMESSAGINGHOST_H

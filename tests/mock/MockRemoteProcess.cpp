/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "MockRemoteProcess.h"

#include <QTest>
#include <utility>

MockRemoteProcess::MockRemoteProcess(QObject* parent, QString mockedTempFileLocation, QString source)
    : RemoteProcess(parent)
    , m_mockedTempFileLocation(std::move(mockedTempFileLocation))
    , m_source(std::move(source))
{
}

void MockRemoteProcess::start(const QString&)
{
    // nothing to do
}

qint64 MockRemoteProcess::write(const QString& data)
{
    return data.length();
}

bool MockRemoteProcess::waitForBytesWritten()
{
    return true;
}

void MockRemoteProcess::closeWriteChannel()
{
    // nothing to do
}

bool MockRemoteProcess::waitForFinished(int)
{
    return true; // no need to wait
}

int MockRemoteProcess::exitCode() const
{
    return 0; // always return success
}

QString MockRemoteProcess::getTempFileLocation()
{
    return m_mockedTempFileLocation;
}

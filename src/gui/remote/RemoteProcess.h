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

#ifndef KEEPASSXC_REMOTEPROCESS_H
#define KEEPASSXC_REMOTEPROCESS_H

#include <QProcess>

class RemoteProcess
{
public:
    explicit RemoteProcess(QObject* parent);
    virtual ~RemoteProcess() = default;

    virtual void start(const QString& program);
    virtual qint64 write(const QString& data);
    virtual bool waitForBytesWritten();
    virtual void closeWriteChannel();
    virtual bool waitForFinished(int msecs);
    [[nodiscard]] virtual int exitCode() const;

    virtual QString getTempFileLocation();

public slots:
    void kill();

private:
    QProcess* m_process;
};

#endif // KEEPASSXC_REMOTEPROCESS_H

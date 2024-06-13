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

#ifndef KEEPASSXC_MOCKREMOTEPROCESS_H
#define KEEPASSXC_MOCKREMOTEPROCESS_H

#include "gui/remote/RemoteProcess.h"

class MockRemoteProcess : public RemoteProcess
{
public:
    explicit MockRemoteProcess(QObject* parent, const QString& dbPath);
    ~MockRemoteProcess() override = default;

    void start(const QString& program) override;
    qint64 write(const QString& data) override;
    bool waitForBytesWritten() override;
    void closeWriteChannel() override;
    bool waitForFinished(int msecs) override;
    [[nodiscard]] int exitCode() const override;

private:
    QByteArray m_data;
    QString m_dbPath;
};

#endif // KEEPASSXC_MOCKREMOTEPROCESS_H

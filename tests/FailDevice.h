/*
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_FAILDEVICE_H
#define KEEPASSX_FAILDEVICE_H

#include <QBuffer>

class FailDevice : public QBuffer
{
    Q_OBJECT

public:
    explicit FailDevice(int failAfter, QObject* parent = nullptr);
    bool open(QIODevice::OpenMode openMode) override;

protected:
    qint64 readData(char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    int m_failAfter;
    int m_readCount;
    int m_writeCount;
};

#endif // KEEPASSX_FAILDEVICE_H

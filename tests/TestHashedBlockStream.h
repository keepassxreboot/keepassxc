/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TESTHASHEDBLOCKSTREAM_H
#define KEEPASSX_TESTHASHEDBLOCKSTREAM_H

#include <QObject>

class TestHashedBlockStream : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testWriteRead();
    void testReset();
    void testWriteFailure();
};

#endif // KEEPASSX_TESTHASHEDBLOCKSTREAM_H

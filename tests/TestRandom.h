/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TESTRANDOM_H
#define KEEPASSX_TESTRANDOM_H

#include "crypto/Random.h"

#include <QObject>

class RandomBackendTest : public RandomBackend
{
public:
    RandomBackendTest();
    void randomize(void* data, int len) override;
    void setNextBytes(const QByteArray& nextBytes);

private:
    QByteArray m_nextBytes;
    int m_bytesIndex;
};

class TestRandom : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testUInt();
    void testUIntRange();

private:
    RandomBackendTest* m_backend;
};

#endif // KEEPASSX_TESTRANDOM_H

/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_TESTRANDOM_H
#define KEEPASSXC_TESTRANDOM_H

#include "crypto/Random.h"

class RandomBackendPreset : public RandomBackend
{
public:
    RandomBackendPreset();
    void randomize(void* data, int len) override;
    void setNextBytes(const QByteArray& nextBytes);

private:
    QByteArray m_nextBytes;
    int m_bytesIndex;
};

class RandomBackendNull : public RandomBackend
{
public:
    void randomize(void* data, int len) override;
};

class TestRandom : public Random
{
public:
    static void setup(RandomBackend* backend);
    static void teardown();
};

#endif // KEEPASSXC_TESTRANDOM_H

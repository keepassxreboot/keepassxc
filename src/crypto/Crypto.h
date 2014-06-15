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

#ifndef KEEPASSX_CRYPTO_H
#define KEEPASSX_CRYPTO_H

#include <QString>

#include "core/Global.h"

class Crypto
{
public:
    static bool init();
    static bool initalized();
    static bool backendSelfTest();
    static QString errorString();

private:
    Crypto();
    static bool checkAlgorithms();
    static bool selfTest();

    static bool m_initalized;
    static QString m_errorStr;
};

#endif // KEEPASSX_CRYPTO_H

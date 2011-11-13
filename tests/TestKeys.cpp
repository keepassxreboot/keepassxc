/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "TestKeys.h"

#include <QtTest/QTest>

#include "crypto/Crypto.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

void TestKeys::initTestCase()
{
    Crypto::init();
}

void TestKeys::testComposite()
{
    CompositeKey* compositeKey1 = new CompositeKey();
    PasswordKey* passwordKey1 = new PasswordKey();
    PasswordKey* passwordKey2 = new PasswordKey("test");

    compositeKey1->addKey(*passwordKey1);
    compositeKey1->addKey(*passwordKey2);
    delete passwordKey1;
    delete passwordKey2;

    QVERIFY(compositeKey1->transform(QByteArray(32, '\0'), 1).size() == 32);

    // make sure the subkeys are copied
    CompositeKey* compositeKey2 = compositeKey1->clone();
    delete compositeKey1;

    QVERIFY(compositeKey2->transform(QByteArray(32, '\0'), 1).size() == 32);

    delete compositeKey2;
}

QTEST_MAIN(TestKeys);

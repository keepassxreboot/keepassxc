/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTSYMMETRICCIPHER_H
#define KEEPASSX_TESTSYMMETRICCIPHER_H

#include <QObject>

class TestSymmetricCipher : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCipherUuidToMode();
    void testEncryptionDecryption_data();
    void testEncryptionDecryption();
    void testAesCbcPadding_data();
    void testAesCbcPadding();
    void testAesKdf();
    void testTwofish256CbcEncryption();
    void testTwofish256CbcDecryption();
    void testSalsa20();
    void testChaCha20();
    void testPadding();
    void testStreamReset();
};

#endif // KEEPASSX_TESTSYMMETRICCIPHER_H

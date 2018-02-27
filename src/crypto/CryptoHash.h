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

#ifndef KEEPASSX_CRYPTOHASH_H
#define KEEPASSX_CRYPTOHASH_H

#include <QByteArray>

class CryptoHashPrivate;

class CryptoHash
{
public:
    enum Algorithm
    {
        Sha256,
        Sha512
    };

    explicit CryptoHash(Algorithm algo, bool hmac = false);
    ~CryptoHash();
    void addData(const QByteArray& data);
    void reset();
    QByteArray result() const;
    void setKey(const QByteArray& data);

    static QByteArray hash(const QByteArray& data, Algorithm algo);
    static QByteArray hmac(const QByteArray& data, const QByteArray& key, Algorithm algo);

private:
    CryptoHashPrivate* const d_ptr;

    Q_DECLARE_PRIVATE(CryptoHash)
};

#endif // KEEPASSX_CRYPTOHASH_H

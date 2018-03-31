/*
 *  Copyright (C) 2018 Toni Spets <toni.spets@iki.fi>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "ASN1Key.h"
#include <gcrypt.h>

namespace
{
    constexpr quint8 TAG_INT = 0x02;
    constexpr quint8 TAG_SEQUENCE = 0x30;
    constexpr quint8 KEY_ZERO = 0x0;

    bool nextTag(BinaryStream& stream, quint8& tag, quint32& len)
    {
        stream.read(tag);

        quint8 lenByte;
        stream.read(lenByte);

        if (lenByte & 0x80) {
            quint32 bytes = lenByte & ~0x80;
            if (bytes == 1) {
                stream.read(lenByte);
                len = lenByte;
            } else if (bytes == 2) {
                quint16 lenShort;
                stream.read(lenShort);
                len = lenShort;
            } else if (bytes == 4) {
                stream.read(len);
            } else {
                return false;
            }
        } else {
            len = lenByte;
        }

        return true;
    }

    bool parseHeader(BinaryStream& stream, quint8 wantedType)
    {
        quint8 tag;
        quint32 len;

        nextTag(stream, tag, len);

        if (tag != TAG_SEQUENCE) {
            return false;
        }

        nextTag(stream, tag, len);

        if (tag != TAG_INT || len != 1) {
            return false;
        }

        quint8 keyType;
        stream.read(keyType);

        return (keyType == wantedType);
    }

    bool readInt(BinaryStream& stream, QByteArray& target)
    {
        quint8 tag;
        quint32 len;

        nextTag(stream, tag, len);

        if (tag != TAG_INT) {
            return false;
        }

        target.resize(len);
        stream.read(target);
        return true;
    }

    QByteArray calculateIqmp(QByteArray& bap, QByteArray& baq)
    {
        gcry_mpi_t u, p, q;
        QByteArray iqmp_hex;

        u = gcry_mpi_snew(bap.length() * 8);
        gcry_mpi_scan(&p, GCRYMPI_FMT_HEX, bap.toHex().data(), 0, nullptr);
        gcry_mpi_scan(&q, GCRYMPI_FMT_HEX, baq.toHex().data(), 0, nullptr);

        mpi_invm(u, q, p);

        iqmp_hex.resize((bap.length() + 1) * 2);
        gcry_mpi_print(
            GCRYMPI_FMT_HEX, reinterpret_cast<unsigned char*>(iqmp_hex.data()), iqmp_hex.length(), nullptr, u);

        gcry_mpi_release(u);
        gcry_mpi_release(p);
        gcry_mpi_release(q);

        return QByteArray::fromHex(iqmp_hex);
    }
}

bool ASN1Key::parseDSA(QByteArray& ba, OpenSSHKey& key)
{
    BinaryStream stream(&ba);

    if (!parseHeader(stream, KEY_ZERO)) {
        return false;
    }

    QByteArray p, q, g, y, x;
    readInt(stream, p);
    readInt(stream, q);
    readInt(stream, g);
    readInt(stream, y);
    readInt(stream, x);

    QList<QByteArray> publicData;
    publicData.append(p);
    publicData.append(q);
    publicData.append(g);
    publicData.append(y);

    QList<QByteArray> privateData;
    privateData.append(p);
    privateData.append(q);
    privateData.append(g);
    privateData.append(y);
    privateData.append(x);

    key.setType("ssh-dss");
    key.setPublicData(publicData);
    key.setPrivateData(privateData);
    key.setComment("");
    return true;
}

bool ASN1Key::parseRSA(QByteArray& ba, OpenSSHKey& key)
{
    BinaryStream stream(&ba);

    if (!parseHeader(stream, KEY_ZERO)) {
        return false;
    }

    QByteArray n, e, d, p, q, dp, dq, qinv;
    readInt(stream, n);
    readInt(stream, e);
    readInt(stream, d);
    readInt(stream, p);
    readInt(stream, q);
    readInt(stream, dp);
    readInt(stream, dq);
    readInt(stream, qinv);

    QList<QByteArray> publicData;
    publicData.append(e);
    publicData.append(n);

    QList<QByteArray> privateData;
    privateData.append(n);
    privateData.append(e);
    privateData.append(d);
    privateData.append(calculateIqmp(p, q));
    privateData.append(p);
    privateData.append(q);

    key.setType("ssh-rsa");
    key.setPublicData(publicData);
    key.setPrivateData(privateData);
    key.setComment("");
    return true;
}

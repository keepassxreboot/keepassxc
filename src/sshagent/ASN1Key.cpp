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
#include "BinaryStream.h"
#include "OpenSSHKey.h"

#define VALIDATE_RETURN(call) if (!call) { return false; }

namespace
{
    constexpr quint8 TAG_INT = 0x02;
    constexpr quint8 TAG_SEQUENCE = 0x30;
    constexpr quint8 KEY_ZERO = 0x0;

    bool nextTag(BinaryStream& stream, quint8& tag, quint32& len)
    {
        VALIDATE_RETURN(stream.read(tag));

        quint8 lenByte;
        VALIDATE_RETURN(stream.read(lenByte));

        if (lenByte & 0x80) {
            quint32 bytes = lenByte & ~0x80;
            if (bytes == 1) {
                VALIDATE_RETURN(stream.read(lenByte));
                len = lenByte;
            } else if (bytes == 2) {
                quint16 lenShort;
                VALIDATE_RETURN(stream.read(lenShort));
                len = lenShort;
            } else if (bytes == 4) {
                VALIDATE_RETURN(stream.read(len));
            } else {
                return false;
            }
        } else {
            len = lenByte;
        }

        return true;
    }

    bool parsePrivateHeader(BinaryStream& stream, quint8 wantedType)
    {
        quint8 tag;
        quint32 len;

        VALIDATE_RETURN(nextTag(stream, tag, len));

        if (tag != TAG_SEQUENCE) {
            return false;
        }

        VALIDATE_RETURN(nextTag(stream, tag, len));

        if (tag != TAG_INT || len != 1) {
            return false;
        }

        quint8 keyType;
        VALIDATE_RETURN(stream.read(keyType));

        return (keyType == wantedType);
    }

    bool readInt(BinaryStream& stream, QByteArray& target)
    {
        quint8 tag;
        quint32 len;

        VALIDATE_RETURN(nextTag(stream, tag, len));

        if (tag != TAG_INT) {
            return false;
        }

        target.resize(len);
        VALIDATE_RETURN(stream.read(target));
        return true;
    }
} // namespace

bool ASN1Key::parseDSA(QByteArray& ba, OpenSSHKey& key)
{
    BinaryStream stream(&ba);

    if (!parsePrivateHeader(stream, KEY_ZERO)) {
        return false;
    }

    QByteArray p, q, g, y, x;
    VALIDATE_RETURN(stream.read(p));
    VALIDATE_RETURN(stream.read(q));
    VALIDATE_RETURN(stream.read(g));
    VALIDATE_RETURN(stream.read(y));
    VALIDATE_RETURN(stream.read(x));

    QByteArray publicData;
    BinaryStream publicDataStream(&publicData);
    VALIDATE_RETURN(publicDataStream.writeString(p));
    VALIDATE_RETURN(publicDataStream.writeString(q));
    VALIDATE_RETURN(publicDataStream.writeString(g));
    VALIDATE_RETURN(publicDataStream.writeString(y));

    QByteArray privateData;
    BinaryStream privateDataStream(&privateData);
    VALIDATE_RETURN(privateDataStream.writeString(p));
    VALIDATE_RETURN(privateDataStream.writeString(q));
    VALIDATE_RETURN(privateDataStream.writeString(g));
    VALIDATE_RETURN(privateDataStream.writeString(y));
    VALIDATE_RETURN(privateDataStream.writeString(x));

    key.setType("ssh-dss");
    key.setPublicData(publicData);
    key.setPrivateData(privateData);
    key.setComment("");
    return true;
}

bool ASN1Key::parseRSA(QByteArray& ba, OpenSSHKey& key)
{
    BinaryStream stream(&ba);

    VALIDATE_RETURN(parsePrivateHeader(stream, KEY_ZERO));

    QByteArray n, e, d, p, q, dp, dq, qinv;
    VALIDATE_RETURN(readInt(stream, n));
    VALIDATE_RETURN(readInt(stream, e));
    VALIDATE_RETURN(readInt(stream, d));
    VALIDATE_RETURN(readInt(stream, p));
    VALIDATE_RETURN(readInt(stream, q));
    VALIDATE_RETURN(readInt(stream, dp));
    VALIDATE_RETURN(readInt(stream, dq));
    VALIDATE_RETURN(readInt(stream, qinv));

    // Note: To properly calculate the key fingerprint, e and n are reversed per RFC 4253
    QByteArray publicData;
    BinaryStream publicDataStream(&publicData);
    VALIDATE_RETURN(publicDataStream.writeString(e));
    VALIDATE_RETURN(publicDataStream.writeString(n));

    QByteArray privateData;
    BinaryStream privateDataStream(&privateData);
    VALIDATE_RETURN(privateDataStream.writeString(n));
    VALIDATE_RETURN(privateDataStream.writeString(e));
    VALIDATE_RETURN(privateDataStream.writeString(d));
    VALIDATE_RETURN(privateDataStream.writeString(qinv));
    VALIDATE_RETURN(privateDataStream.writeString(p));
    VALIDATE_RETURN(privateDataStream.writeString(q));

    key.setType("ssh-rsa");
    key.setPublicData(publicData);
    key.setPrivateData(privateData);
    key.setComment("");
    return true;
}

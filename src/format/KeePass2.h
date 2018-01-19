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

#ifndef KEEPASSX_KEEPASS2_H
#define KEEPASSX_KEEPASS2_H

#include <QtGlobal>
#include <QMap>
#include <QVariantMap>
#include <QList>
#include <QSharedPointer>

#include "crypto/SymmetricCipher.h"
#include "crypto/kdf/Kdf.h"
#include "core/Uuid.h"

namespace KeePass2
{

constexpr quint32 SIGNATURE_1 = 0x9AA2D903;
constexpr quint32 SIGNATURE_2 = 0xB54BFB67;

constexpr quint32 FILE_VERSION_CRITICAL_MASK = 0xFFFF0000;
constexpr quint32 FILE_VERSION_4             = 0x00040000;
constexpr quint32 FILE_VERSION_3_1           = 0x00030001;
constexpr quint32 FILE_VERSION_3             = 0x00030000;
constexpr quint32 FILE_VERSION_2             = 0x00020000;
constexpr quint32 FILE_VERSION_MIN           = FILE_VERSION_2;

constexpr quint16 VARIANTMAP_VERSION       = 0x0100;
constexpr quint16 VARIANTMAP_CRITICAL_MASK = 0xFF00;

const QSysInfo::Endian BYTEORDER = QSysInfo::LittleEndian;

extern const Uuid CIPHER_AES;
extern const Uuid CIPHER_TWOFISH;
extern const Uuid CIPHER_CHACHA20;

extern const Uuid KDF_AES_KDBX3;
extern const Uuid KDF_AES_KDBX4;
extern const Uuid KDF_ARGON2;

extern const QByteArray INNER_STREAM_SALSA20_IV;

extern const QString KDFPARAM_UUID;
extern const QString KDFPARAM_AES_ROUNDS;
extern const QString KDFPARAM_AES_SEED;
extern const QString KDFPARAM_ARGON2_SALT;
extern const QString KDFPARAM_ARGON2_PARALLELISM;
extern const QString KDFPARAM_ARGON2_MEMORY;
extern const QString KDFPARAM_ARGON2_ITERATIONS;
extern const QString KDFPARAM_ARGON2_VERSION;
extern const QString KDFPARAM_ARGON2_SECRET;
extern const QString KDFPARAM_ARGON2_ASSOCDATA;

extern const QList<QPair<Uuid, QString>> CIPHERS;
extern const QList<QPair<Uuid, QString>> KDFS;

enum class HeaderFieldID
{
    EndOfHeader = 0,
    Comment = 1,
    CipherID = 2,
    CompressionFlags = 3,
    MasterSeed = 4,
    TransformSeed = 5,
    TransformRounds = 6,
    EncryptionIV = 7,
    ProtectedStreamKey = 8,
    StreamStartBytes = 9,
    InnerRandomStreamID = 10,
    KdfParameters = 11,
    PublicCustomData = 12
};

enum class InnerHeaderFieldID : quint8
{
    End = 0,
    InnerRandomStreamID = 1,
    InnerRandomStreamKey = 2,
    Binary = 3
};

enum class ProtectedStreamAlgo
{
    ArcFourVariant = 1,
    Salsa20 = 2,
    ChaCha20 = 3,
    InvalidProtectedStreamAlgo = -1
};

enum class VariantMapFieldType : quint8
{
    End = 0,
    // Byte = 0x02,
    // UInt16 = 0x03,
    UInt32 = 0x04,
    UInt64 = 0x05,
    // Signed mask: 0x08
    Bool = 0x08,
    // SByte = 0x0A,
    // Int16 = 0x0B,
    Int32 = 0x0C,
    Int64 = 0x0D,
    // Float = 0x10,
    // Double = 0x11,
    // Decimal = 0x12,
    // Char = 0x17, // 16-bit Unicode character
    String = 0x18,
    // Array mask: 0x40
    ByteArray = 0x42
};

QByteArray hmacKey(QByteArray masterSeed, QByteArray transformedMasterKey);
QSharedPointer<Kdf> kdfFromParameters(const QVariantMap& p);
QVariantMap kdfToParameters(QSharedPointer<Kdf> kdf);
QSharedPointer<Kdf> uuidToKdf(const Uuid& uuid);
Uuid kdfToUuid(QSharedPointer<Kdf> kdf);
ProtectedStreamAlgo idToProtectedStreamAlgo(quint32 id);

}   // namespace KeePass2

#endif // KEEPASSX_KEEPASS2_H

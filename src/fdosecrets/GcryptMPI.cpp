/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "GcryptMPI.h"

GcryptMPI MpiFromBytes(const QByteArray& bytes, bool secure, gcry_mpi_format format)
{
    auto bufLen = static_cast<size_t>(bytes.size());

    const char* buf = nullptr;

    // gcry_mpi_scan uses secure memory only if input buffer is secure memory, so we have to make a copy
    GcryptMemPtr secureBuf;
    if (secure) {
        secureBuf.reset(static_cast<char*>(gcry_malloc_secure(bufLen)));
        memcpy(secureBuf.get(), bytes.data(), bufLen);
        buf = secureBuf.get();
    } else {
        buf = bytes.data();
    }

    gcry_mpi_t rawMpi;
    auto err = gcry_mpi_scan(&rawMpi, format, buf, format == GCRYMPI_FMT_HEX ? 0 : bufLen, nullptr);
    GcryptMPI mpi(rawMpi);

    if (gcry_err_code(err) != GPG_ERR_NO_ERROR) {
        mpi.reset();
    }

    return mpi;
}

GcryptMPI MpiFromHex(const char* hex, bool secure)
{
    return MpiFromBytes(QByteArray::fromRawData(hex, static_cast<int>(strlen(hex) + 1)), secure, GCRYMPI_FMT_HEX);
}

QByteArray MpiToBytes(const GcryptMPI& mpi)
{
    unsigned char* buf = nullptr;
    size_t buflen = 0;
    gcry_mpi_aprint(GCRYMPI_FMT_USG, &buf, &buflen, mpi.get());

    QByteArray bytes(reinterpret_cast<char*>(buf), static_cast<int>(buflen));

    gcry_free(buf);

    return bytes;
}

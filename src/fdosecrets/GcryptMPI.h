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

#ifndef KEEPASSXC_GCRYPTMPI_H
#define KEEPASSXC_GCRYPTMPI_H

#include <QByteArray>

#include <gcrypt.h>

#include <memory>
#include <type_traits>

template <typename D, D fn> using deleter_from_fn = std::integral_constant<D, fn>;

/**
 * A simple RAII wrapper for gcry_mpi_t
 */
using GcryptMPI = std::unique_ptr<gcry_mpi, deleter_from_fn<decltype(&gcry_mpi_release), &gcry_mpi_release>>;

/**
 * A simple RAII wrapper for libgcrypt allocated memory
 */
using GcryptMemPtr = std::unique_ptr<char, deleter_from_fn<decltype(&gcry_free), &gcry_free>>;

/**
 * Parse a QByteArray as MPI
 * @param bytes
 * @param secure
 * @param format
 * @return
 */
GcryptMPI MpiFromBytes(const QByteArray& bytes, bool secure = true, gcry_mpi_format format = GCRYMPI_FMT_USG);

/**
 * Parse MPI from hex data
 * @param hex
 * @param secure
 * @return
 */
GcryptMPI MpiFromHex(const char* hex, bool secure = true);

/**
 * Dump MPI to bytes in USG format
 * @param mpi
 * @return
 */
QByteArray MpiToBytes(const GcryptMPI& mpi);

#endif // KEEPASSXC_GCRYPTMPI_H

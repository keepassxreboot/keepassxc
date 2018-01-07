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

#ifndef KEEPASSXC_KDBXWRITER_H
#define KEEPASSXC_KDBXWRITER_H

#include "KeePass2.h"
#include "core/Endian.h"

#include <QCoreApplication>

#define CHECK_RETURN_FALSE(x) if (!(x)) return false;

class QIODevice;
class Database;

/**
 * Abstract KDBX writer base class.
 */
class KdbxWriter
{
Q_DECLARE_TR_FUNCTIONS(KdbxWriter)

public:
    KdbxWriter() = default;
    virtual ~KdbxWriter() = default;

    bool writeMagicNumbers(QIODevice* device, quint32 sig1, quint32 sig2, quint32 version);

    /**
     * Write a database to a device in KDBX format.
     *
     * @param device output device
     * @param db source database
     * @return true on success
     */
    virtual bool writeDatabase(QIODevice* device, Database* db) = 0;

    bool hasError() const;
    QString errorString() const;

protected:

    /**
     * Helper method for writing a KDBX header field to a device.
     *
     * @tparam SizedQInt field width
     * @param device output device
     * @param fieldId field identifier
     * @param data field contents
     * @return true on success
     */
    template <typename SizedQInt>
    bool writeHeaderField(QIODevice* device, KeePass2::HeaderFieldID fieldId, const QByteArray& data)
    {
        Q_ASSERT(static_cast<unsigned long>(data.size()) < (1ull << (sizeof(SizedQInt) * 8)));

        QByteArray fieldIdArr;
        fieldIdArr[0] = static_cast<char>(fieldId);
        CHECK_RETURN_FALSE(writeData(device, fieldIdArr));
        CHECK_RETURN_FALSE(writeData(device, Endian::sizedIntToBytes<SizedQInt>(static_cast<SizedQInt>(data.size()),
                                                                                KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeData(device, data));

        return true;
    }

    bool writeData(QIODevice* device, const QByteArray& data);
    void raiseError(const QString& errorMessage);

    bool m_error = false;
    QString m_errorStr = "";
};


#endif //KEEPASSXC_KDBXWRITER_H

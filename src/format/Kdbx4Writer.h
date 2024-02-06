/*
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

#ifndef KEEPASSX_KDBX4WRITER_H
#define KEEPASSX_KDBX4WRITER_H

#include "KdbxWriter.h"
#include "format/KdbxXmlWriter.h"

/**
 * KDBX4 writer implementation.
 */
class Kdbx4Writer : public KdbxWriter
{
    Q_DECLARE_TR_FUNCTIONS(Kdbx4Writer)

public:
    bool writeDatabase(QIODevice* device, Database* db) override;

private:
    bool writeInnerHeaderField(QIODevice* device, KeePass2::InnerHeaderFieldID fieldId, const QByteArray& data);
    KdbxXmlWriter::BinaryIdxMap writeAttachments(QIODevice* device, Database* db);
    static bool serializeVariantMap(const QVariantMap& map, QByteArray& outputBytes);
};

#endif // KEEPASSX_KDBX4WRITER_H

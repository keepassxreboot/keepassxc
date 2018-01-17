/*
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_FILEKEY_H
#define KEEPASSX_FILEKEY_H

#include <QXmlStreamReader>

#include "keys/Key.h"

class QIODevice;

class FileKey: public Key
{
public:
    enum Type {
        None,
        Hashed,
        KeePass2XML,
        FixedBinary,
        FixedBinaryHex
    };

    bool load(QIODevice* device);
    bool load(const QString& fileName, QString* errorMsg = nullptr);
    QByteArray rawKey() const override;
    FileKey* clone() const override;
    Type type() const;
    static void create(QIODevice* device, int size = 128);
    static bool create(const QString& fileName, QString* errorMsg = nullptr, int size = 128);

private:
    bool loadXml(QIODevice* device);
    bool loadXmlMeta(QXmlStreamReader& xmlReader);
    QByteArray loadXmlKey(QXmlStreamReader& xmlReader);
    bool loadBinary(QIODevice* device);
    bool loadHex(QIODevice* device);
    bool loadHashed(QIODevice* device);

    QByteArray m_key;
    Type m_type = None;
};

#endif // KEEPASSX_FILEKEY_H

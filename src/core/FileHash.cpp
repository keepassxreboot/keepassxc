/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FileHash.h"
#include <QFile>

QString FileHash::getFileHash(const QString& filePath, QCryptographicHash::Algorithm hashType, int bufferSize)
{
    QCryptographicHash hash(hashType);

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }

    while (!file.atEnd()) {
        hash.addData(file.read(bufferSize));
    }

    file.close();
    return hash.result().toHex();
}

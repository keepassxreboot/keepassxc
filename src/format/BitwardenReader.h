/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef BITWARDEN_READER_H
#define BITWARDEN_READER_H

#include <QSharedPointer>

class Database;

/*!
 * Imports a Bitwarden vault in JSON format: https://bitwarden.com/help/encrypted-export/
 */
class BitwardenReader
{
public:
    explicit BitwardenReader() = default;
    ~BitwardenReader() = default;

    QSharedPointer<Database> convert(const QString& path, const QString& password = {});

    bool hasError();
    QString errorString();

private:
    QString m_error;
};

#endif // BITWARDEN_READER_H

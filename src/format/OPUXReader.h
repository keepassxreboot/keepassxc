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

#ifndef OPUX_READER_H
#define OPUX_READER_H

#include <QSharedPointer>

class Database;

/*!
 * Imports a 1Password vault in 1PUX format: https://support.1password.com/1pux-format/
 */
class OPUXReader
{
public:
    explicit OPUXReader() = default;
    ~OPUXReader() = default;

    QSharedPointer<Database> convert(const QString& path);

    bool hasError();
    QString errorString();

private:
    QString m_error;
};

#endif // OPUX_READER_H

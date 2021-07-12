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

#ifndef KEEPASSX_KEEPASS2READER_H
#define KEEPASSX_KEEPASS2READER_H

#include "KdbxReader.h"

class CompositeKey;

class KeePass2Reader
{
    Q_DECLARE_TR_FUNCTIONS(KdbxReader)

public:
    bool readDatabase(const QString& filename, QSharedPointer<const CompositeKey> key, Database* db);
    bool readDatabase(QIODevice* device, QSharedPointer<const CompositeKey> key, Database* db);

    bool hasError() const;
    QString errorString() const;

    QSharedPointer<KdbxReader> reader() const;
    quint32 version() const;

private:
    void raiseError(const QString& errorMessage);

    bool m_error = false;
    QString m_errorStr = "";

    QSharedPointer<KdbxReader> m_reader;
    quint32 m_version = 0;
};

#endif // KEEPASSX_KEEPASS2READER_H

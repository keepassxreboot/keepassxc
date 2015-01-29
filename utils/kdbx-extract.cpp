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

#include <stdio.h>

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (app.arguments().size() != 3) {
        qCritical("Usage: kdbx-extract <password/key file> <kdbx file>");
        return 1;
    }

    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
    }

    CompositeKey key;
    if (QFile::exists(app.arguments().at(1))) {
        FileKey fileKey;
        fileKey.load(app.arguments().at(1));
        key.addKey(fileKey);
    }
    else {
        PasswordKey password;
        password.setPassword(app.arguments().at(1));
        key.addKey(password);
    }

    QFile dbFile(app.arguments().at(2));
    if (!dbFile.exists()) {
        qCritical("File does not exist.");
        return 1;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open file.");
        return 1;
    }

    KeePass2Reader reader;
    reader.setSaveXml(true);
    Database* db = reader.readDatabase(&dbFile, key);
    delete db;

    QByteArray xmlData = reader.xmlData();

    if (reader.hasError()) {
        if (xmlData.isEmpty()) {
            qCritical("Error while reading the database:\n%s", qPrintable(reader.errorString()));
            return 1;
        }
        else {
            qWarning("Error while parsing the database:\n%s\n", qPrintable(reader.errorString()));
        }
    }

    QTextStream out(stdout);
    out << xmlData.constData() << "\n";

    return 0;
}

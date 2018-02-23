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

#include <cstdlib>
#include <stdio.h>

#include "Pawned.h"

#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/CryptoHash.h"

Pawned::Pawned()
{
    name = QString("pawned");
    description = QObject::tr("Queries the database's passwords against the haveibeenpwned dump.");
}

Pawned::~Pawned()
{
}

int Pawned::execute(const QStringList& arguments)
{
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addPositionalArgument("hibp-database", QObject::tr("Path of the HIBP database."));
    QCommandLineOption keyFile(QStringList() << "k"
                                             << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli pawned");
        return EXIT_FAILURE;
    }

    Database* db = Database::unlockFromStdin(args.at(0), parser.value(keyFile));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    QFile passwordDatabase(args.at(1));
    if (!passwordDatabase.open(QIODevice::ReadOnly)) {
        qCritical("Could not open pawned passwords database %s.", qPrintable(args.at(1)));
        return EXIT_FAILURE;
    }

    QMap<QString, QByteArray> entryMap;
    for (Entry* entry : db->rootGroup()->entriesRecursive()) {
      // This is to make sure the entry title is unique.
      QString entryTitle = entry->title() + " (" + entry->uuid().toHex() + ")";
      if (entry->password().isEmpty()) {
        continue;
      }
      entryMap[entryTitle] = CryptoHash::hash(entry->password().toLatin1(), CryptoHash::Sha1).toHex().toUpper();
    }

    while (!passwordDatabase.atEnd()) {
      QString pawnedPasswordLine = passwordDatabase.readLine();
      if (pawnedPasswordLine.split(":").length() != 2) {
        qCritical("Invalid format for pawned passwords database.");
        return EXIT_FAILURE;
      }

      QString pawnedPasswordHash = pawnedPasswordLine.split(":").at(0);
      QString pawnedPasswordCount = pawnedPasswordLine.split(":").at(1).simplified();
      
      for (QString entryTitle: entryMap.keys()) {
        QByteArray entryHash = entryMap[entryTitle];
        if (entryHash == pawnedPasswordHash) {
          out << "Password for " << entryTitle << " was pawned " << pawnedPasswordCount << " times." << endl;
        }
      }
    }

    return EXIT_SUCCESS;
}

/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "Locate.h"

#include <QStringList>

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"

Locate::Locate()
{
    name = QString("locate");
    description = QObject::tr("Find entries quickly.");
    positionalArguments.append({QString("term"), QObject::tr("Search term."), QString("")});
}

int Locate::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    const QString& searchTerm = args.at(1);

    QStringList results = database->rootGroup()->locate(searchTerm);
    if (results.isEmpty()) {
        err << "No results for that search term." << endl;
        return EXIT_FAILURE;
    }

    for (const QString& result : asConst(results)) {
        out << result << endl;
    }
    return EXIT_SUCCESS;
}

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

#include "Search.h"

#include <QCommandLineParser>

#include "Utils.h"
#include "core/EntrySearcher.h"
#include "core/Group.h"

Search::Search()
{
    name = QString("search");
    description = QObject::tr("Find entries quickly.");
    positionalArguments.append({QString("term"), QObject::tr("Search term."), QString("")});
}

int Search::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();

    EntrySearcher searcher;
    auto results = searcher.search(args.at(1), database->rootGroup(), true);
    if (results.isEmpty()) {
        err << "No results for that search term." << Qt::endl;
        return EXIT_FAILURE;
    }

    for (const Entry* result : asConst(results)) {
        out << result->path().prepend('/') << Qt::endl;
    }
    return EXIT_SUCCESS;
}

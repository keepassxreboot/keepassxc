/*
 *  Copyright (C) 2013 Florian Geyer <blueice@fobos.de>
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

#ifndef KEEPASSX_ARGUMENT_PARSER_H
#define KEEPASSX_ARGUMENT_PARSER_H

#include "core/Global.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>

class ArgumentParser
{

public:
    static QHash<QString, QString> parseArguments(const QStringList& args);
private:
    static const QStringList argumentKeys;
    static void parseOption(int& i, QHash<QString, QString>& argumentMap, const QStringList& args);
};

#endif // KEEPASSX_ARGUMENT_PARSER_H

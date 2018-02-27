/*
 *  Copyright (C) 2015 David Wu <lightvector@gmail.com>
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

#ifndef KEEPASSX_AUTOTYPEMATCH_H
#define KEEPASSX_AUTOTYPEMATCH_H

#include <QObject>
#include <QString>

class Entry;

struct AutoTypeMatch
{
    Entry* entry;
    QString sequence;

    AutoTypeMatch();
    AutoTypeMatch(Entry* entry, QString sequence);

    bool operator==(const AutoTypeMatch& other) const;
    bool operator!=(const AutoTypeMatch& other) const;
};

Q_DECLARE_TYPEINFO(AutoTypeMatch, Q_MOVABLE_TYPE);

#endif // KEEPASSX_AUTOTYPEMATCH_H

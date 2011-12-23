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

#ifndef KEEPASSX_DATABASEICONS_H
#define KEEPASSX_DATABASEICONS_H

#include <QtCore/QHash>
#include <QtGui/QIcon>

class DatabaseIcons
{
public:
    static QIcon icon(int index);
    static int iconCount();

private:
    static DatabaseIcons* m_instance;

private:
    DatabaseIcons();
    QIcon getIconInternal(int index);

    bool m_initalized;
    QList<QString> m_indexToName;
    QHash<int,QIcon> m_iconCache;
};

#endif // KEEPASSX_DATABASEICONS_H

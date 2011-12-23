/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DATAPATH_H
#define KEEPASSX_DATAPATH_H

#include <QtCore/QString>

class DataPath
{
public:
    static QString getPath(const QString& name);

private:
    DataPath();
    bool testSetDir(const QString& dir);

    static DataPath* m_instance;
    QString m_basePath;
};

#endif // KEEPASSX_DATAPATH_H

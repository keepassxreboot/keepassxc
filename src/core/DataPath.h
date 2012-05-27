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
#include <QtGui/QIcon>

class DataPath
{
public:
    QString path(const QString& name);
    QIcon applicationIcon();
    QIcon icon(const QString& category, const QString& name, bool fromTheme = true);

private:
    DataPath();
    bool testSetDir(const QString& dir);

    QString m_basePath;

    Q_DISABLE_COPY(DataPath)

    friend DataPath* dataPath();
};

DataPath* dataPath();

#endif // KEEPASSX_DATAPATH_H

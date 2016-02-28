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

#include <QImage>
#include <QPixmap>
#include <QPixmapCache>
#include <QVector>

class DatabaseIcons
{
public:
    QImage icon(int index);
    QPixmap iconPixmap(int index);

    static DatabaseIcons* instance();

    static const int IconCount;
    static const int ExpiredIconIndex;

private:
    DatabaseIcons();

    static DatabaseIcons* m_instance;

    static const char* const m_indexToName[];
    QVector<QImage> m_iconCache;
    QVector<QPixmapCache::Key> m_pixmapCacheKeys;

    Q_DISABLE_COPY(DatabaseIcons)
};

inline DatabaseIcons* databaseIcons() {
    return DatabaseIcons::instance();
}

#endif // KEEPASSX_DATABASEICONS_H

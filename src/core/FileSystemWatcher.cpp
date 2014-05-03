/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "FileSystemWatcher.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

FileSystemWatcher::FileSystemWatcher (): QFileSystemWatcher ( )
{
    connect(this,SIGNAL( fileChanged ( const QString & )),
            this,SLOT( fileChangedSlot ( const QString & )));
    connect(this,SIGNAL( directoryChanged ( const QString & )),
            this,SLOT( directoryChangedSlot ( const QString & )));
}

void FileSystemWatcher::watchFile(const QString &file)
{
    _file=file;
    if (!files().isEmpty())
        removePaths(files());
    if (!directories().isEmpty())
        removePaths(directories());
    QFileInfo fileInfo(file);
    if (fileInfo.exists())
        addPath(file);
    QString filePath=fileInfo.absoluteDir().path();
    QFileInfo filePathInfo(filePath);
    if (filePathInfo.exists())
        addPath(filePath);
}

void FileSystemWatcher::stopWatching()
{
    watchFile( QString() );
}

void FileSystemWatcher::fileChangedSlot ( const QString & )
{
    QFileInfo fileInfo(_file);
    if ( fileInfo.exists() )
        fileChanged();
}

void FileSystemWatcher::directoryChangedSlot ( const QString & )
{
    if (!files().contains(_file))
    {
        QFileInfo fileInfo(_file);
        if ( fileInfo.exists() )
        {
            addPath(_file);
            fileChanged();
        }
    }
}

FileSystemWatcher::~FileSystemWatcher()
{
}


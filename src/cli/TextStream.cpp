/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "TextStream.h"

#include <QProcessEnvironment>
#include <QTextCodec>

TextStream::TextStream()
{
    detectCodec();
}

TextStream::TextStream(QIODevice* device)
    : QTextStream(device)
{
    detectCodec();
}

TextStream::TextStream(FILE* fileHandle, QIODevice::OpenMode openMode)
    : QTextStream(fileHandle, openMode)
{
    detectCodec();
}

TextStream::TextStream(QString* string, QIODevice::OpenMode openMode)
    : QTextStream(string, openMode)
{
    detectCodec();
}

TextStream::TextStream(QByteArray* array, QIODevice::OpenMode openMode)
    : QTextStream(array, openMode)
{
    detectCodec();
}

TextStream::TextStream(const QByteArray& array, QIODevice::OpenMode openMode)
    : QTextStream(array, openMode)
{
    detectCodec();
}

void TextStream::detectCodec()
{
    QString codecName = "UTF-8";
    auto env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    if (!env.contains("SHELL")) {
        // native shell (no Msys or cygwin)
        codecName = "Windows-850";
    }
#endif
    codecName = env.value("ENCODING_OVERRIDE", codecName);
    auto* codec = QTextCodec::codecForName(codecName.toLatin1());
    if (codec) {
        setCodec(codec);
    }
}

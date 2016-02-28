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

#include "Tools.h"

#include <QCoreApplication>
#include <QImageReader>
#include <QIODevice>
#include <QLocale>
#include <QStringList>

#include <QElapsedTimer>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep(), SetDllDirectoryA() and SetSearchPathMode()
#endif

#ifdef Q_OS_UNIX
#include <time.h> // for nanosleep()
#endif

#include "config-keepassx.h"

#if defined(HAVE_RLIMIT_CORE)
#include <sys/resource.h>
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
#include <sys/prctl.h>
#endif

#ifdef HAVE_PT_DENY_ATTACH
#include <sys/types.h>
#include <sys/ptrace.h>
#endif

namespace Tools {

QString humanReadableFileSize(qint64 bytes)
{
    double size = bytes;

    QStringList units = QStringList() << "B" << "KiB" << "MiB" << "GiB";
    int i = 0;
    int maxI = units.size() - 1;

    while ((size >= 1024) && (i < maxI)) {
        size /= 1024;
        i++;
    }

    return QString("%1 %2").arg(QLocale().toString(size, 'f', 2), units.at(i));
}

bool hasChild(const QObject* parent, const QObject* child)
{
    if (!parent || !child) {
        return false;
    }
    Q_FOREACH (QObject* c, parent->children()) {
        if (child == c || hasChild(c, child)) {
            return true;
        }
    }
    return false;
}

bool readFromDevice(QIODevice* device, QByteArray& data, int size)
{
    QByteArray buffer;
    buffer.resize(size);

    qint64 readResult = device->read(buffer.data(), size);
    if (readResult == -1) {
        return false;
    }
    else {
        buffer.resize(readResult);
        data = buffer;
        return true;
    }
}

bool readAllFromDevice(QIODevice* device, QByteArray& data)
{
    QByteArray result;
    qint64 readBytes = 0;
    qint64 readResult;
    do {
        result.resize(result.size() + 16384);
        readResult = device->read(result.data() + readBytes, result.size() - readBytes);
        if (readResult > 0) {
            readBytes += readResult;
        }
    } while (readResult > 0);

    if (readResult == -1) {
        return false;
    }
    else {
        result.resize(static_cast<int>(readBytes));
        data = result;
        return true;
    }
}

QString imageReaderFilter()
{
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QStringList formatsStringList;

    Q_FOREACH (const QByteArray& format, formats) {
        for (int i = 0; i < format.size(); i++) {
            if (!QChar(format.at(i)).isLetterOrNumber()) {
                continue;
            }
        }

        formatsStringList.append("*." + QString::fromLatin1(format).toLower());
    }

    return formatsStringList.join(" ");
}

bool isHex(const QByteArray& ba)
{
    Q_FOREACH (char c, ba) {
        if ( !( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ) ) {
            return false;
        }
    }

    return true;
}

bool isBase64(const QByteArray& ba)
{
    QRegExp regexp("^(?:[a-z0-9+/]{4})*(?:[a-z0-9+/]{3}=|[a-z0-9+/]{2}==)?$",
                   Qt::CaseInsensitive, QRegExp::RegExp2);

    QString base64 = QString::fromLatin1(ba.constData(), ba.size());

    return regexp.exactMatch(base64);
}

void sleep(int ms)
{
    Q_ASSERT(ms >= 0);

    if (ms == 0) {
        return;
    }

#ifdef Q_OS_WIN
    Sleep(uint(ms));
#else
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, nullptr);
#endif
}

void wait(int ms)
{
    Q_ASSERT(ms >= 0);

    if (ms == 0) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    if (ms <= 50) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
        sleep(qMax(ms - static_cast<int>(timer.elapsed()), 0));
    }
    else {
        int timeLeft;
        do {
            timeLeft = ms - timer.elapsed();
            if (timeLeft > 0) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, timeLeft);
                sleep(10);
            }
        } while (!timer.hasExpired(ms));
    }
}

void disableCoreDumps()
{
    // default to true
    // there is no point in printing a warning if this is not implemented on the platform
    bool success = true;

#if defined(HAVE_RLIMIT_CORE)
    struct rlimit limit;
    limit.rlim_cur = 0;
    limit.rlim_max = 0;
    success = success && (setrlimit(RLIMIT_CORE, &limit) == 0);
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
    success = success && (prctl(PR_SET_DUMPABLE, 0) == 0);
#endif

    // Mac OS X
#ifdef HAVE_PT_DENY_ATTACH
    success = success && (ptrace(PT_DENY_ATTACH, 0, 0, 0) == 0);
#endif

    if (!success) {
        qWarning("Unable to disable core dumps.");
    }
}

void setupSearchPaths()
{
#ifdef Q_OS_WIN
    // Make sure Windows doesn't load DLLs from the current working directory
    SetDllDirectoryA("");
    SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE);
#endif
}

} // namespace Tools

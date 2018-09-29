/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 Lennart Glauer <mail@lennart-glauer.de>
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

#include "Tools.h"
#include "core/Config.h"
#include "core/Translator.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QImageReader>
#include <QLocale>
#include <QStringList>
#include <QElapsedTimer>

#include <cctype>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep()
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
// clang-format off
#include <sys/types.h>
#include <sys/ptrace.h>
// clang-format on
#endif

namespace Tools
{
QString humanReadableFileSize(qint64 bytes, quint32 precision)
{
    constexpr auto kibibyte = 1024;
    double size = bytes;

    QStringList units = QStringList() << "B"
                                      << "KiB"
                                      << "MiB"
                                      << "GiB";
    int i = 0;
    int maxI = units.size() - 1;

    while ((size >= kibibyte) && (i < maxI)) {
        size /= kibibyte;
        i++;
    }

    return QString("%1 %2").arg(QLocale().toString(size, 'f', precision), units.at(i));
}

bool hasChild(const QObject* parent, const QObject* child)
{
    if (!parent || !child) {
        return false;
    }

    const QObjectList children = parent->children();
    for (QObject* c : children) {
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
    } else {
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
    }
    while (readResult > 0);

    if (readResult == -1) {
        return false;
    } else {
        result.resize(static_cast<int>(readBytes));
        data = result;
        return true;
    }
}

QString imageReaderFilter()
{
    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QStringList formatsStringList;

    for (const QByteArray& format : formats) {
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
    for (const unsigned char c : ba) {
        if (!std::isxdigit(c)) {
            return false;
        }
    }

    return true;
}

bool isBase64(const QByteArray& ba)
{
    constexpr auto pattern = R"(^(?:[a-z0-9+]{4})*(?:[a-z0-9+]{3}=|[a-z0-9+]{2}==)?$)";
    QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);

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
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)*1000*1000;
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
    } else {
        int timeLeft;
        do {
            timeLeft = ms - timer.elapsed();
            if (timeLeft > 0) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, timeLeft);
                sleep(10);
            }
        }
        while (!timer.hasExpired(ms));
    }
}

} // namespace Tools

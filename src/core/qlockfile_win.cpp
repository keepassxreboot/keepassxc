/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include "qlockfile_p.h"

#include <qt_windows.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

QT_BEGIN_NAMESPACE

static inline QByteArray localHostName()
{
    return qgetenv("COMPUTERNAME");
}

static inline bool fileExists(const wchar_t *fileName)
{
    WIN32_FILE_ATTRIBUTE_DATA  data;
    return GetFileAttributesEx(fileName, GetFileExInfoStandard, &data);
}

QLockFile::LockError QLockFilePrivate::tryLock_sys()
{
    const ushort* nativePath = QDir::toNativeSeparators(fileName).utf16();
    // When writing, allow others to read.
    // When reading, QFile will allow others to read and write, all good.
    // Adding FILE_SHARE_DELETE would allow forceful deletion of stale files,
    // but Windows doesn't allow recreating it while this handle is open anyway,
    // so this would only create confusion (can't lock, but no lock file to read from).
    const DWORD dwShareMode = FILE_SHARE_READ;
#ifndef Q_OS_WINRT
    SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    HANDLE fh = CreateFile((const wchar_t*)nativePath,
                           GENERIC_WRITE,
                           dwShareMode,
                           &securityAtts,
                           CREATE_NEW, // error if already exists
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
#else // !Q_OS_WINRT
    HANDLE fh = CreateFile2((const wchar_t*)nativePath,
                            GENERIC_WRITE,
                            dwShareMode,
                            CREATE_NEW, // error if already exists
                            NULL);
#endif // Q_OS_WINRT
    if (fh == INVALID_HANDLE_VALUE) {
        const DWORD lastError = GetLastError();
        switch (lastError) {
        case ERROR_SHARING_VIOLATION:
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return QLockFile::LockFailedError;
        case ERROR_ACCESS_DENIED:
            // readonly file, or file still in use by another process.
            // Assume the latter if the file exists, since we don't create it readonly.
            return fileExists((const wchar_t*)nativePath)
                ? QLockFile::LockFailedError
                : QLockFile::PermissionError;
        default:
            qWarning() << "Got unexpected locking error" << lastError;
            return QLockFile::UnknownError;
        }
    }

    // We hold the lock, continue.
    fileHandle = fh;
    // Assemble data, to write in a single call to write
    // (otherwise we'd have to check every write call)
    QByteArray fileData;
    fileData += QByteArray::number(QCoreApplication::applicationPid());
    fileData += '\n';
    fileData += QCoreApplication::applicationName().toUtf8();
    fileData += '\n';
    fileData += localHostName();
    fileData += '\n';
    DWORD bytesWritten = 0;
    QLockFile::LockError error = QLockFile::NoError;
    if (!WriteFile(fh, fileData.constData(), fileData.size(), &bytesWritten, NULL) || !FlushFileBuffers(fh))
        error = QLockFile::UnknownError; // partition full
    return error;
}

bool QLockFilePrivate::removeStaleLock()
{
    // QFile::remove fails on Windows if the other process is still using the file, so it's not stale.
    return QFile::remove(fileName);
}

bool QLockFilePrivate::isApparentlyStale() const
{
    qint64 pid;
    QString hostname, appname;

    // On WinRT there seems to be no way of obtaining information about other
    // processes due to sandboxing
#ifndef Q_OS_WINRT
    if (getLockInfo(&pid, &hostname, &appname)) {
        if (hostname.isEmpty() || hostname == QString::fromLocal8Bit(localHostName())) {
            HANDLE procHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (!procHandle)
                return true;
            // We got a handle but check if process is still alive
            DWORD dwR = ::WaitForSingleObject(procHandle, 0);
            ::CloseHandle(procHandle);
            if (dwR == WAIT_TIMEOUT)
                return true;
            const QString processName = processNameByPid(pid);
            if (!processName.isEmpty() && processName != appname)
                return true; // PID got reused by a different application.
        }
    }
#else // !Q_OS_WINRT
    Q_UNUSED(pid);
    Q_UNUSED(hostname);
    Q_UNUSED(appname);
#endif // Q_OS_WINRT
    const qint64 age = QFileInfo(fileName).lastModified().msecsTo(QDateTime::currentDateTime());
    return staleLockTime > 0 && age > staleLockTime;
}

QString QLockFilePrivate::processNameByPid(qint64 pid)
{
#if !defined(Q_OS_WINRT) && !defined(Q_OS_WINCE)
    typedef DWORD (WINAPI *GetModuleFileNameExFunc)(HANDLE, HMODULE, LPTSTR, DWORD);

    HMODULE hPsapi = LoadLibraryA("psapi");
    if (!hPsapi)
        return QString();

    GetModuleFileNameExFunc qGetModuleFileNameEx
            = (GetModuleFileNameExFunc)GetProcAddress(hPsapi, "GetModuleFileNameExW");
    if (!qGetModuleFileNameEx) {
        FreeLibrary(hPsapi);
        return QString();
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, DWORD(pid));
    if (!hProcess) {
        FreeLibrary(hPsapi);
        return QString();
    }
    wchar_t buf[MAX_PATH];
    const DWORD length = qGetModuleFileNameEx(hProcess, NULL, buf, sizeof(buf) / sizeof(wchar_t));
    CloseHandle(hProcess);
    FreeLibrary(hPsapi);
    if (!length)
        return QString();
    QString name = QString::fromWCharArray(buf, length);
    int i = name.lastIndexOf(QLatin1Char('\\'));
    if (i >= 0)
        name.remove(0, i + 1);
    i = name.lastIndexOf(QLatin1Char('.'));
    if (i >= 0)
        name.truncate(i);
    return name;
#else
    Q_UNUSED(pid);
    return QString();
#endif
}

void QLockFile::unlock()
{
    Q_D(QLockFile);
     if (!d->isLocked)
        return;
     CloseHandle(d->fileHandle);
     int attempts = 0;
     static const int maxAttempts = 500; // 500ms
     while (!QFile::remove(d->fileName) && ++attempts < maxAttempts) {
         // Someone is reading the lock file right now (on Windows this prevents deleting it).
         QLockFileThread::msleep(1);
     }
     if (attempts == maxAttempts) {
        qWarning() << "Could not remove our own lock file" << d->fileName << ". Either other users of the lock file are reading it constantly for 500 ms, or we (no longer) have permissions to delete the file";
        // This is bad because other users of this lock file will now have to wait for the stale-lock-timeout...
     }
     d->lockError = QLockFile::NoError;
     d->isLocked = false;
}

QT_END_NAMESPACE

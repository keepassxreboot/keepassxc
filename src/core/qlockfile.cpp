/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlockfile.h"
#include "qlockfile_p.h"
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#    include <QElapsedTimer>
#else
#    include <QTime>
#endif
#include <QDateTime>

QT_BEGIN_NAMESPACE

/*!
    \class QLockFile
    \inmodule QtCore
    \brief The QLockFile class provides locking between processes using a file.
    \since 5.1

    A lock file can be used to prevent multiple processes from accessing concurrently
    the same resource. For instance, a configuration file on disk, or a socket, a port,
    a region of shared memory...

    Serialization is only guaranteed if all processes that access the shared resource
    use QLockFile, with the same file path.

    QLockFile supports two use cases:
    to protect a resource for a short-term operation (e.g. verifying if a configuration
    file has changed before saving new settings), and for long-lived protection of a
    resource (e.g. a document opened by a user in an editor) for an indefinite amount of time.

    When protecting for a short-term operation, it is acceptable to call lock() and wait
    until any running operation finishes.
    When protecting a resource over a long time, however, the application should always
    call setStaleLockTime(0) and then tryLock() with a short timeout, in order to
    warn the user that the resource is locked.

    If the process holding the lock crashes, the lock file stays on disk and can prevent
    any other process from accessing the shared resource, ever. For this reason, QLockFile
    tries to detect such a "stale" lock file, based on the process ID written into the file,
    and (in case that process ID got reused meanwhile), on the last modification time of
    the lock file (30s by default, for the use case of a short-lived operation).
    If the lock file is found to be stale, it will be deleted.

    For the use case of protecting a resource over a long time, you should therefore call
    setStaleLockTime(0), and when tryLock() returns LockFailedError, inform the user
    that the document is locked, possibly using getLockInfo() for more details.
*/

/*!
    \enum QLockFile::LockError

    This enum describes the result of the last call to lock() or tryLock().

    \value NoError The lock was acquired successfully.
    \value LockFailedError The lock could not be acquired because another process holds it.
    \value PermissionError The lock file could not be created, for lack of permissions
                           in the parent directory.
    \value UnknownError Another error happened, for instance a full partition
                        prevented writing out the lock file.
*/

/*!
    Constructs a new lock file object.
    The object is created in an unlocked state.
    When calling lock() or tryLock(), a lock file named \a fileName will be created,
    if it doesn't already exist.

    \sa lock(), unlock()
*/
QLockFile::QLockFile(const QString &fileName)
    : d_ptr(new QLockFilePrivate(fileName))
{
}

/*!
    Destroys the lock file object.
    If the lock was acquired, this will release the lock, by deleting the lock file.
*/
QLockFile::~QLockFile()
{
    unlock();
}

/*!
    Sets \a staleLockTime to be the time in milliseconds after which
    a lock file is considered stale.
    The default value is 30000, i.e. 30 seconds.
    If your application typically keeps the file locked for more than 30 seconds
    (for instance while saving megabytes of data for 2 minutes), you should set
    a bigger value using setStaleLockTime().

    The value of \a staleLockTime is used by lock() and tryLock() in order
    to determine when an existing lock file is considered stale, i.e. left over
    by a crashed process. This is useful for the case where the PID got reused
    meanwhile, so the only way to detect a stale lock file is by the fact that
    it has been around for a long time.

    \sa staleLockTime()
*/
void QLockFile::setStaleLockTime(int staleLockTime)
{
    Q_D(QLockFile);
    d->staleLockTime = staleLockTime;
}

/*!
    Returns the time in milliseconds after which
    a lock file is considered stale.

    \sa setStaleLockTime()
*/
int QLockFile::staleLockTime() const
{
    Q_D(const QLockFile);
    return d->staleLockTime;
}

/*!
    Returns \c true if the lock was acquired by this QLockFile instance,
    otherwise returns \c false.

    \sa lock(), unlock(), tryLock()
*/
bool QLockFile::isLocked() const
{
    Q_D(const QLockFile);
    return d->isLocked;
}

/*!
    Creates the lock file.

    If another process (or another thread) has created the lock file already,
    this function will block until that process (or thread) releases it.

    Calling this function multiple times on the same lock from the same
    thread without unlocking first is not allowed. This function will
    \e dead-lock when the file is locked recursively.

    Returns \c true if the lock was acquired, false if it could not be acquired
    due to an unrecoverable error, such as no permissions in the parent directory.

    \sa unlock(), tryLock()
*/
bool QLockFile::lock()
{
    return tryLock(-1);
}

/*!
    Attempts to create the lock file. This function returns \c true if the
    lock was obtained; otherwise it returns \c false. If another process (or
    another thread) has created the lock file already, this function will
    wait for at most \a timeout milliseconds for the lock file to become
    available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lock(), i.e. this function will wait forever until the lock
    file can be locked if \a timeout is negative.

    If the lock was obtained, it must be released with unlock()
    before another process (or thread) can successfully lock it.

    Calling this function multiple times on the same lock from the same
    thread without unlocking first is not allowed, this function will
    \e always return false when attempting to lock the file recursively.

    \sa lock(), unlock()
*/
bool QLockFile::tryLock(int timeout)
{
    Q_D(QLockFile);
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    QElapsedTimer timer;
#else
    QTime timer;
#endif
    if (timeout > 0)
        timer.start();
    int sleepTime = 100;
    Q_FOREVER {
        d->lockError = d->tryLock_sys();
        switch (d->lockError) {
        case NoError:
            d->isLocked = true;
            return true;
        case PermissionError:
        case UnknownError:
            return false;
        case LockFailedError:
            if (!d->isLocked && d->isApparentlyStale()) {
                // Stale lock from another thread/process
                // Ensure two processes don't remove it at the same time
                QLockFile rmlock(d->fileName + QLatin1String(".rmlock"));
                if (rmlock.tryLock()) {
                    if (d->isApparentlyStale() && d->removeStaleLock())
                        continue;
                }
            }
            break;
        }
        if (timeout == 0 || (timeout > 0 && (timer.elapsed() > timeout)))
            return false;
        QLockFileThread::msleep(sleepTime);
        if (sleepTime < 5 * 1000)
            sleepTime *= 2;
    }
    // not reached
    return false;
}

/*!
    \fn void QLockFile::unlock()
    Releases the lock, by deleting the lock file.

    Calling unlock() without locking the file first, does nothing.

    \sa lock(), tryLock()
*/

/*!
    Retrieves information about the current owner of the lock file.

    If tryLock() returns \c false, and error() returns LockFailedError,
    this function can be called to find out more information about the existing
    lock file:
    \list
    \li the PID of the application (returned in \a pid)
    \li the \a hostname it's running on (useful in case of networked filesystems),
    \li the name of the application which created it (returned in \a appname),
    \endlist

    Note that tryLock() automatically deleted the file if there is no
    running application with this PID, so LockFailedError can only happen if there is
    an application with this PID (it could be unrelated though).

    This can be used to inform users about the existing lock file and give them
    the choice to delete it. After removing the file using removeStaleLockFile(),
    the application can call tryLock() again.

    This function returns \c true if the information could be successfully retrieved, false
    if the lock file doesn't exist or doesn't contain the expected data.
    This can happen if the lock file was deleted between the time where tryLock() failed
    and the call to this function. Simply call tryLock() again if this happens.
*/
bool QLockFile::getLockInfo(qint64 *pid, QString *hostname, QString *appname) const
{
    Q_D(const QLockFile);
    return d->getLockInfo(pid, hostname, appname);
}

bool QLockFilePrivate::getLockInfo(qint64 *pid, QString *hostname, QString *appname) const
{
    QFile reader(fileName);
    if (!reader.open(QIODevice::ReadOnly))
        return false;

    QByteArray pidLine = reader.readLine();
    pidLine.chop(1);
    QByteArray appNameLine = reader.readLine();
    appNameLine.chop(1);
    QByteArray hostNameLine = reader.readLine();
    hostNameLine.chop(1);
    if (pidLine.isEmpty())
        return false;

    qint64 thePid = pidLine.toLongLong();
    if (pid)
        *pid = thePid;
    if (appname)
        *appname = QString::fromUtf8(appNameLine);
    if (hostname)
        *hostname = QString::fromUtf8(hostNameLine);
    return thePid > 0;
}

/*!
    Attempts to forcefully remove an existing lock file.

    Calling this is not recommended when protecting a short-lived operation: QLockFile
    already takes care of removing lock files after they are older than staleLockTime().

    This method should only be called when protecting a resource for a long time, i.e.
    with staleLockTime(0), and after tryLock() returned LockFailedError, and the user
    agreed on removing the lock file.

    Returns \c true on success, false if the lock file couldn't be removed. This happens
    on Windows, when the application owning the lock is still running.
*/
bool QLockFile::removeStaleLockFile()
{
    Q_D(QLockFile);
    if (d->isLocked) {
        qWarning("removeStaleLockFile can only be called when not holding the lock");
        return false;
    }
    return d->removeStaleLock();
}

/*!
    Returns the lock file error status.

    If tryLock() returns \c false, this function can be called to find out
    the reason why the locking failed.
*/
QLockFile::LockError QLockFile::error() const
{
    Q_D(const QLockFile);
    return d->lockError;
}

QT_END_NAMESPACE

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

#ifndef QLOCKFILE_P_H
#define QLOCKFILE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qlockfile.h"

#include <QFile>
#include <QThread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

class QLockFileThread : public QThread
{
public:
    static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
};

class QLockFilePrivate
{
public:
    QLockFilePrivate(const QString &fn)
        : fileName(fn),
#ifdef Q_OS_WIN
          fileHandle(INVALID_HANDLE_VALUE),
#else
          fileHandle(-1),
#endif
          staleLockTime(30 * 1000), // 30 seconds
          lockError(QLockFile::NoError),
          isLocked(false)
    {
    }
    QLockFile::LockError tryLock_sys();
    bool removeStaleLock();
    bool getLockInfo(qint64 *pid, QString *hostname, QString *appname) const;
    // Returns \c true if the lock belongs to dead PID, or is old.
    // The attempt to delete it will tell us if it was really stale or not, though.
    bool isApparentlyStale() const;
    static QString processNameByPid(qint64 pid);

    QString fileName;
#ifdef Q_OS_WIN
    Qt::HANDLE fileHandle;
#else
    int fileHandle;
#endif
    int staleLockTime; // "int milliseconds" is big enough for 24 days
    QLockFile::LockError lockError;
    bool isLocked;
};

QT_END_NAMESPACE

#endif /* QLOCKFILE_P_H */

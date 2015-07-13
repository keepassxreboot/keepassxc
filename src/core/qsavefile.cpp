/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsavefile.h"
#include "qsavefile_p.h"

#include <QAbstractFileEngine>
#include <QFileInfo>
#include <QTemporaryFile>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <unistd.h>
#endif

QSaveFilePrivate::QSaveFilePrivate()
    : tempFile(0), error(QFile::NoError)
{
}

QSaveFilePrivate::~QSaveFilePrivate()
{
}

/*!
    \class QSaveFile
    \brief The QSaveFile class provides an interface for safely writing to files.

    \ingroup io

    \reentrant

    QSaveFile is an I/O device for writing text and binary files, without losing
    existing data if the writing operation fails.

    While writing, the contents will be written to a temporary file, and if
    no error happened, commit() will move it to the final file. This ensures that
    no data at the final file is lost in case an error happens while writing,
    and no partially-written file is ever present at the final location. Always
    use QSaveFile when saving entire documents to disk.

    QSaveFile automatically detects errors while writing, such as the full partition
    situation, where write() cannot write all the bytes. It will remember that
    an error happened, and will discard the temporary file in commit().

    Much like with QFile, the file is opened with open(). Data is usually read
    and written using QDataStream or QTextStream, but you can also call the
    QIODevice-inherited functions read(), readLine(), readAll(), write().

    Unlike QFile, calling close() is not allowed. commit() replaces it. If commit()
    was not called and the QSaveFile instance is destroyed, the temporary file is
    discarded.

    \sa QTextStream, QDataStream, QFileInfo, QDir, QFile, QTemporaryFile
*/

/*!
    \internal
*/
QSaveFile::QSaveFile()
    : QIODevice(), d_ptr(new QSaveFilePrivate)
{
}
/*!
    Constructs a new file object with the given \a parent.
*/
QSaveFile::QSaveFile(QObject *parent)
    : QIODevice(parent), d_ptr(new QSaveFilePrivate)
{
}
/*!
    Constructs a new file object to represent the file with the given \a name.
*/
QSaveFile::QSaveFile(const QString &name)
    : QIODevice(0), d_ptr(new QSaveFilePrivate)
{
    Q_D(QSaveFile);
    d->fileName = name;
}
/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QSaveFile::QSaveFile(const QString &name, QObject *parent)
    : QIODevice(parent), d_ptr(new QSaveFilePrivate)
{
    Q_D(QSaveFile);
    d->fileName = name;
}

/*!
    Destroys the file object, discarding the saved contents unless commit() was called.
*/
QSaveFile::~QSaveFile()
{
    Q_D(QSaveFile);
    if (d->tempFile) {
        d->tempFile->setAutoRemove(true);
        delete d->tempFile;
    }
    QIODevice::close();
    delete d;
}

/*!
    Returns false since temporary files support random access.

    \sa QIODevice::isSequential()
*/
bool QSaveFile::isSequential() const
{
    return false;
}

/*!
    Returns the file error status.

    The I/O device status returns an error code. For example, if open()
    returns false, or a read/write operation returns -1, this function can
    be called to find out the reason why the operation failed.

    Unlike QFile which clears the error on the next operation, QSaveFile remembers
    the error until the file is closed, in order to discard the file contents in close().

    \sa unsetError()
*/

QFile::FileError QSaveFile::error() const
{
    return d_func()->error;
}

/*!
    Sets the file's error to QFile::NoError.

    This will make QSaveFile forget that an error happened during saving, so you
    probably don't want to call this, unless you're really sure that you want to
    save the file anyway.

    \sa error()
*/
void QSaveFile::unsetError()
{
    d_func()->error = QFile::NoError;
    setErrorString(QString());
}

/*!
    Returns the name set by setFileName() or to the QSaveFile
    constructor.

    \sa setFileName()
*/
QString QSaveFile::fileName() const
{
    return d_func()->fileName;
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    \sa QFile::setFileName(), fileName()
*/
void QSaveFile::setFileName(const QString &name)
{
    d_func()->fileName = name;
}

/*!
    Opens the file using OpenMode \a mode, returning true if successful;
    otherwise false.

    Important: the \a mode must be QIODevice::WriteOnly.
    It may also have additional flags, such as QIODevice::Text and QIODevice::Unbuffered.

    QIODevice::ReadWrite and QIODevice::Append are not supported at the moment.

    \sa QIODevice::OpenMode, setFileName()
*/
bool QSaveFile::open(OpenMode mode)
{
    Q_D(QSaveFile);
    if (isOpen()) {
        qWarning("QSaveFile::open: File (%s) already open", qPrintable(fileName()));
        return false;
    }
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QSaveFile::open: Open mode not specified");
        return false;
    }
    // In the future we could implement Append and ReadWrite by copying from the existing file to the temp file...
    if ((mode & ReadOnly) || (mode & Append)) {
        qWarning("QSaveFile::open: Unsupported open mode %d", int(mode));
        return false;
    }

    // check if existing file is writable
    QFileInfo existingFile(d->fileName);
    if (existingFile.exists() && !existingFile.isWritable()) {
        d->error = QFile::WriteError;
        setErrorString(QSaveFile::tr("Existing file %1 is not writable").arg(d->fileName));
        return false;
    }
    d->tempFile = new QTemporaryFile;
    d->tempFile->setAutoRemove(false);
    d->tempFile->setFileTemplate(d->fileName);
    if (!d->tempFile->open()) {
        d->error = d->tempFile->error();
        setErrorString(d->tempFile->errorString());
        delete d->tempFile;
        d->tempFile = 0;
        return false;
    }
    QIODevice::open(mode);
    if (existingFile.exists())
        d->tempFile->setPermissions(existingFile.permissions());
    return true;
}

/*!
  \reimp
  Cannot be called.
  Call commit() instead.
*/
void QSaveFile::close()
{
    qFatal("QSaveFile::close called");
}

/*
  Commits the changes to disk, if all previous writes were successful.

  It is mandatory to call this at the end of the saving operation, otherwise the file will be
  discarded.

  If an error happened during writing, deletes the temporary file and returns false.
  Otherwise, renames it to the final fileName and returns true on success.
  Finally, closes the device.

  \sa cancelWriting()
*/
bool QSaveFile::commit()
{
    Q_D(QSaveFile);
    if (!d->tempFile)
        return false;
    if (!isOpen()) {
        qWarning("QSaveFile::commit: File (%s) is not open", qPrintable(fileName()));
        return false;
    }
    flush();
#ifdef Q_OS_WIN
    FlushFileBuffers(reinterpret_cast<HANDLE>(handle()));
#elif defined(_POSIX_SYNCHRONIZED_IO) && _POSIX_SYNCHRONIZED_IO > 0
    fdatasync(d->tempFile->handle());
#else
    fsync(d->tempFile->handle());
#endif
    QIODevice::close();
    if (d->error != QFile::NoError) {
        d->tempFile->remove();
        unsetError();
        delete d->tempFile;
        d->tempFile = 0;
        return false;
    }
    d->tempFile->close();
#ifdef Q_OS_WIN
    // On Windows QAbstractFileEngine::rename() fails if the the target exists,
    // so we have to rename the target.
    // Ideally the winapi ReplaceFile() method should be used.
    QString bakname = d->fileName + "~";
    QFile::remove(bakname);
    QFile::rename(d->fileName, bakname);
#endif
    QAbstractFileEngine* fileEngine = d->tempFile->fileEngine();
    Q_ASSERT(fileEngine);
    if (!fileEngine->rename(d->fileName)) {
        d->error = fileEngine->error();
        setErrorString(fileEngine->errorString());
        d->tempFile->remove();
        delete d->tempFile;
        d->tempFile = 0;
#ifdef Q_OS_WIN
        QFile::rename(bakname, d->fileName);
#endif
        return false;
    }
    delete d->tempFile;
    d->tempFile = 0;
#ifdef Q_OS_WIN
    QFile::remove(bakname);
#endif
    return true;
}

/*!
  Sets an error code so that commit() discards the temporary file.

  Further write operations are possible after calling this method, but none
  of it will have any effect, the written file will be discarded.

  \sa commit()
*/
void QSaveFile::cancelWriting()
{
    if (!isOpen())
        return;
    d_func()->error = QFile::WriteError;
    setErrorString(QSaveFile::tr("Writing canceled by application"));
}

/*!
  Returns the size of the file.
  \sa QFile::size()
*/
qint64 QSaveFile::size() const
{
    Q_D(const QSaveFile);
    return d->tempFile ? d->tempFile->size() : qint64(-1);
}

/*!
  \reimp
*/
qint64 QSaveFile::pos() const
{
    Q_D(const QSaveFile);
    return d->tempFile ? d->tempFile->pos() : qint64(-1);
}

/*!
  \reimp
*/
bool QSaveFile::seek(qint64 offset)
{
    Q_D(QSaveFile);
    return d->tempFile ? d->tempFile->seek(offset) : false;
}

/*!
  \reimp
*/
bool QSaveFile::atEnd() const
{
    Q_D(const QSaveFile);
    return d->tempFile ? d->tempFile->atEnd() : true;
}

/*!
    Flushes any buffered data to the file. Returns true if successful;
    otherwise returns false.
*/
bool QSaveFile::flush()
{
    Q_D(QSaveFile);
    if (d->tempFile) {
        if (!d->tempFile->flush()) {
            d->error = d->tempFile->error();
            setErrorString(d->tempFile->errorString());
            return false;
        }
        return true;
    }
    return false;
}

/*!
  Returns the file handle of the temporary file.

  \sa QFile::handle()
*/
int QSaveFile::handle() const
{
    Q_D(const QSaveFile);
    return d->tempFile ? d->tempFile->handle() : -1;
}

/*!
  \reimp
*/
qint64 QSaveFile::readData(char *data, qint64 maxlen)
{
    Q_D(QSaveFile);
    return d->tempFile ? d->tempFile->read(data, maxlen) : -1;
}

/*!
  \reimp
*/
qint64 QSaveFile::writeData(const char *data, qint64 len)
{
    Q_D(QSaveFile);
    if (!d->tempFile)
        return -1;
    const qint64 written = d->tempFile->write(data, len);
    if (written != len) {
        d->error = QFile::WriteError;
        setErrorString(QSaveFile::tr("Partial write. Partition full?"));
    }
    return written;
}

/*!
  \reimp
*/
qint64 QSaveFile::readLineData(char *data, qint64 maxlen)
{
    Q_D(QSaveFile);
    return d->tempFile ? d->tempFile->readLine(data, maxlen) : -1;
}

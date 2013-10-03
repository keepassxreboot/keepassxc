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

#ifndef QSAVEFILE_H
#define QSAVEFILE_H

#include <QFile>
#include <QString>

#ifdef open
#error qsavefile.h must be included before any header file that defines open
#endif

class QAbstractFileEngine;
class QSaveFilePrivate;

class QSaveFile : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSaveFile)

public:

    QSaveFile();
    explicit QSaveFile(const QString &name);
    explicit QSaveFile(QObject *parent);
    QSaveFile(const QString &name, QObject *parent);
    ~QSaveFile();

    QFile::FileError error() const;
    void unsetError();

    QString fileName() const;
    void setFileName(const QString &name);

    bool isSequential() const;

    virtual bool open(OpenMode flags);
    bool commit();

    void cancelWriting();

    qint64 size() const;
    qint64 pos() const;
    bool seek(qint64 offset);
    bool atEnd() const;
    bool flush();

    bool resize(qint64 sz);

    int handle() const;

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 readLineData(char *data, qint64 maxlen);

private:
    virtual void close();

private:
    Q_DISABLE_COPY(QSaveFile)

    QSaveFilePrivate* const d_ptr;
};

#endif // QSAVEFILE_H

/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOMMANDLINEPARSER_H
#define QCOMMANDLINEPARSER_H

#include <QStringList>
#include <QCoreApplication>

#include "qcommandlineoption.h"

class QCommandLineParserPrivate;
class QCoreApplication;

class QCommandLineParser
{
    Q_DECLARE_TR_FUNCTIONS(QCommandLineParser)
public:
    QCommandLineParser();
    ~QCommandLineParser();

    enum SingleDashWordOptionMode {
        ParseAsCompactedShortOptions,
        ParseAsLongOptions
    };
    void setSingleDashWordOptionMode(SingleDashWordOptionMode parsingMode);

    bool addOption(const QCommandLineOption &commandLineOption);

    QCommandLineOption addVersionOption();
    QCommandLineOption addHelpOption();
    void setApplicationDescription(const QString &description);
    QString applicationDescription() const;
    void addPositionalArgument(const QString &name, const QString &description, const QString &syntax = QString());
    void clearPositionalArguments();

    void process(const QStringList &arguments);
    void process(const QCoreApplication &app);

    bool parse(const QStringList &arguments);
    QString errorText() const;

    bool isSet(const QString &name) const;
    QString value(const QString &name) const;
    QStringList values(const QString &name) const;

    bool isSet(const QCommandLineOption &option) const;
    QString value(const QCommandLineOption &option) const;
    QStringList values(const QCommandLineOption &option) const;

    QStringList positionalArguments() const;
    QStringList optionNames() const;
    QStringList unknownOptionNames() const;

    void showHelp(int exitCode = 0);
    QString helpText() const;

private:
    Q_DISABLE_COPY(QCommandLineParser)

    QCommandLineParserPrivate * const d;
};

#endif // QCOMMANDLINEPARSER_H

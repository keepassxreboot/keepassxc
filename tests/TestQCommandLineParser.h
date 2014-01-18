/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef KEEPASSX_TESTQCOMMANDLINEPARSER_H
#define KEEPASSX_TESTQCOMMANDLINEPARSER_H

#include <QObject>

class TestQCommandLineParser : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void parsingModes_data();

    // In-process tests
    void testInvalidOptions();
    void testPositionalArguments();
    void testBooleanOption_data();
    void testBooleanOption();
    void testMultipleNames_data();
    void testMultipleNames();
    void testSingleValueOption_data();
    void testSingleValueOption();
    void testValueNotSet();
    void testMultipleValuesOption();
    void testUnknownOptionErrorHandling_data();
    void testUnknownOptionErrorHandling();
    void testDoubleDash_data();
    void testDoubleDash();
    void testDefaultValue();
    void testProcessNotCalled();
    void testEmptyArgsList();
    void testMissingOptionValue();
    void testStdinArgument_data();
    void testStdinArgument();
    void testSingleDashWordOptionModes_data();
    void testSingleDashWordOptionModes();
};

#endif // KEEPASSX_TESTQCOMMANDLINEPARSER_H

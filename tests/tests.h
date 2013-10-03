/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef KEEPASSX_TESTS_H
#define KEEPASSX_TESTS_H

#include <QTest>

// backport QTEST_GUILESS_MAIN, QTRY_VERIFY and QTRY_COMPARE from Qt 5

#ifndef QTEST_GUILESS_MAIN
#define QTEST_GUILESS_MAIN(TestObject) \
int main(int argc, char* argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}
#endif // QTEST_GUILESS_MAIN


#ifndef QTRY_VERIFY
#define KEEPASSX_VERIFY_WITH_TIMEOUT(__expr, __timeout) \
do { \
    const int __step = 50; \
    const int __timeoutValue = __timeout; \
    if (!(__expr)) { \
        QTest::qWait(0); \
    } \
    for (int __i = 0; __i < __timeoutValue && !(__expr); __i+=__step) { \
        QTest::qWait(__step); \
    } \
    QVERIFY(__expr); \
} while (0)

#define QTRY_VERIFY(__expr) KEEPASSX_VERIFY_WITH_TIMEOUT(__expr, 5000)
#endif // QTRY_VERIFY


#ifndef QTRY_COMPARE
#define KEEPASSX_COMPARE_WITH_TIMEOUT(__expr, __expected, __timeout) \
do { \
    const int __step = 50; \
    const int __timeoutValue = __timeout; \
    if ((__expr) != (__expected)) { \
        QTest::qWait(0); \
    } \
    for (int __i = 0; __i < __timeoutValue && ((__expr) != (__expected)); __i+=__step) { \
        QTest::qWait(__step); \
    } \
    QCOMPARE(__expr, __expected); \
} while (0)

#define QTRY_COMPARE(__expr, __expected) KEEPASSX_COMPARE_WITH_TIMEOUT(__expr, __expected, 5000)
#endif // QTRY_COMPARE

#endif // KEEPASSX_TESTS_H

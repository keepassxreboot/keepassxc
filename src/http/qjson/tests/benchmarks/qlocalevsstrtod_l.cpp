/* This file is part of QJson
 *
 * Copyright (C) 2014 Sune Vuorela <sune@ange.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software Foundation.
 * 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QObject>
#include <QtTest>
#include <locale.h>

class QLocaleVsStrtod_l : public QObject {
    Q_OBJECT
    private Q_SLOTS:
        void benchmark();
        void benchmark_data();

};

void QLocaleVsStrtod_l::benchmark() {
    QFETCH(bool, useQLocale);
    QList<char*> l;
    l << strdup("0.123") << strdup("0.947834") << strdup("8.8373") << strdup("884.82921");

    double result;

    if(useQLocale) {
        QLocale c(QLocale::C);
        QBENCHMARK {
            Q_FOREACH(const char* str, l) {
                result = c.toDouble(QString(str));
            }
        }
    } else {
        locale_t c = newlocale(LC_NUMERIC_MASK, "C", NULL);
        QBENCHMARK {
            Q_FOREACH(const char* str, l) {
                result = strtod_l(str, NULL, c);
            }
        }
    }


    Q_FOREACH(char* str, l) {
        free(str);
    }
}

void QLocaleVsStrtod_l::benchmark_data() {
    QTest::addColumn<bool>("useQLocale");

    QTest::newRow("using QLocale") << true;
    QTest::newRow("using strtod_l") << false;
}

QTEST_MAIN(QLocaleVsStrtod_l);
#include "qlocalevsstrtod_l.moc"

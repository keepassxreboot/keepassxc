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

#include <QJson/Parser>
#include <QJson/Serializer>
#include <QtTest/QTest>
#include <QFile>

class ParsingBenchmark: public QObject {
    Q_OBJECT
    private Q_SLOTS:
        void benchmark();
};

void ParsingBenchmark::benchmark() {
    QString path = QFINDTESTDATA("largefile.json");

    QVERIFY(QFile::exists(path));

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));

    QByteArray data = f.readAll();

    QVariant result;

    QJson::Parser parser;
    QBENCHMARK {
        result = parser.parse(data);
    }

    Q_UNUSED(result);
}


QTEST_MAIN(ParsingBenchmark)

#include "parsingbenchmark.moc"

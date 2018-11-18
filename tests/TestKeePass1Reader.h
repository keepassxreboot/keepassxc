/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TESTKEEPASS1READER_H
#define KEEPASSX_TESTKEEPASS1READER_H

#include <QDateTime>
#include <QObject>
#include <QSharedPointer>

class Database;

class TestKeePass1Reader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasic();
    void testMasterKey();
    void testCustomIcons();
    void testGroupExpanded();
    void testAutoType();
    void testFileKey();
    void testFileKey_data();
    void testCompositeKey();
    void testTwofish();
    void testCP1252Password();
    void cleanupTestCase();

private:
    static QDateTime genDT(int year, int month, int day, int hour, int min);
    static void reopenDatabase(QSharedPointer<Database> db, const QString& password, const QString& keyfileName);

    QSharedPointer<Database> m_db;
};

#endif // KEEPASSX_TESTKEEPASS1READER_H

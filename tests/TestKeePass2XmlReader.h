/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TESTKEEPASS2XMLREADER_H
#define KEEPASSX_TESTKEEPASS2XMLREADER_H

#include <QDateTime>
#include <QObject>

class Database;

class TestKeePass2XmlReader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMetadata();
    void testCustomIcons();
    void testCustomData();
    void testGroupRoot();
    void testGroup1();
    void testGroup2();
    void testEntry1();
    void testEntry2();
    void testEntryHistory();
    void testDeletedObjects();
    void testBroken();
    void testBroken_data();
    void testEmptyUuids();
    void testInvalidXmlChars();
    void testRepairUuidHistoryItem();
    void cleanupTestCase();

private:
    static QDateTime genDT(int year, int month, int day, int hour, int min, int second);
    static QByteArray strToBytes(const QString& str);

    Database* m_db;
};

#endif // KEEPASSX_TESTKEEPASS2XMLREADER_H

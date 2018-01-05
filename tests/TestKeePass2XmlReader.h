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
#include <QBuffer>

class Database;

class TestKeePass2XmlReader : public QObject
{
    Q_OBJECT

protected slots:
    virtual void initTestCase() = 0;
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

protected:
    virtual void readDatabase(QBuffer* buf, bool strictMode, Database*& db, bool& hasError, QString& errorString) = 0;
    virtual void readDatabase(QString path, bool strictMode, Database*& db, bool& hasError, QString& errorString) = 0;
    virtual void writeDatabase(QBuffer* buf, Database* db, bool& hasError, QString& errorString) = 0;
    static QDateTime genDT(int year, int month, int day, int hour, int min, int second);
    static QByteArray strToBytes(const QString& str);

    Database* m_db;
};

class TestKdbx3XmlReader : public TestKeePass2XmlReader
{
    Q_OBJECT

private slots:
    virtual void initTestCase() override;

protected:
    virtual void readDatabase(QBuffer* buf, bool strictMode, Database*& db, bool& hasError, QString& errorString) override;
    virtual void readDatabase(QString path, bool strictMode, Database*& db, bool& hasError, QString& errorString) override;
    virtual void writeDatabase(QBuffer* buf, Database* db, bool& hasError, QString& errorString) override;
};

class TestKdbx4XmlReader : public TestKeePass2XmlReader
{
    Q_OBJECT

private slots:
    virtual void initTestCase() override;

protected:
    virtual void readDatabase(QBuffer* buf, bool strictMode, Database*& db, bool& hasError, QString& errorString) override;
    virtual void readDatabase(QString path, bool strictMode, Database*& db, bool& hasError, QString& errorString) override;
    virtual void writeDatabase(QBuffer* buf, Database* db, bool& hasError, QString& errorString) override;
};

#endif // KEEPASSX_TESTKEEPASS2XMLREADER_H

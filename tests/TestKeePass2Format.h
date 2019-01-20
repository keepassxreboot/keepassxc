/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTKEEPASS2FORMAT_H
#define KEEPASSXC_TESTKEEPASS2FORMAT_H

#include <QBuffer>
#include <QDateTime>
#include <QObject>
#include <QSharedPointer>

#include "core/Database.h"

/**
 * Abstract base class for KeePass2 file format tests.
 */
class TestKeePass2Format : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    /**
     * XML Reader / writer tests.
     */
    void testXmlMetadata();
    void testXmlCustomIcons();
    void testXmlGroupRoot();
    void testXmlGroup1();
    void testXmlGroup2();
    void testXmlEntry1();
    void testXmlEntry2();
    void testXmlEntryHistory();
    void testXmlDeletedObjects();
    void testXmlBroken();
    void testXmlBroken_data();
    void testXmlEmptyUuids();
    void testXmlInvalidXmlChars();
    void testXmlRepairUuidHistoryItem();

    /**
     * KDBX binary format tests.
     */
    void testReadBackTargetDb();
    void testKdbxBasic();
    void testKdbxProtectedAttributes();
    void testKdbxAttachments();
    void testKdbxNonAsciiPasswords();
    void testKdbxDeviceFailure();
    void testDuplicateAttachments();

protected:
    virtual void initTestCaseImpl() = 0;

    virtual QSharedPointer<Database> readXml(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString) = 0;
    virtual QSharedPointer<Database>
    readXml(const QString& path, bool strictMode, bool& hasError, QString& errorString) = 0;
    virtual void writeXml(QBuffer* buf, Database* db, bool& hasError, QString& errorString) = 0;

    virtual void readKdbx(QIODevice* device,
                          QSharedPointer<const CompositeKey> key,
                          QSharedPointer<Database> db,
                          bool& hasError,
                          QString& errorString) = 0;
    virtual void readKdbx(const QString& path,
                          QSharedPointer<const CompositeKey> key,
                          QSharedPointer<Database> db,
                          bool& hasError,
                          QString& errorString) = 0;
    virtual void writeKdbx(QIODevice* device, Database* db, bool& hasError, QString& errorString) = 0;

    QSharedPointer<Database> m_xmlDb;
    QSharedPointer<Database> m_kdbxSourceDb;
    QSharedPointer<Database> m_kdbxTargetDb;

private:
    QBuffer m_kdbxTargetBuffer;
};

#endif // KEEPASSXC_TESTKEEPASS2FORMAT_H

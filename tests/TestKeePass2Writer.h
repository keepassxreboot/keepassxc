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

#ifndef KEEPASSX_TESTKEEPASS2WRITER_H
#define KEEPASSX_TESTKEEPASS2WRITER_H

#include <QObject>

class Database;

class TestKeePass2Writer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testBasic();
    void testProtectedAttributes();
    void testAttachments();
    void testNonAsciiPasswords();
    void testDeviceFailure();
    void testRepair();
    void cleanupTestCase();

private:
    Database* m_dbOrg;
    Database* m_dbTest;
};

#endif // KEEPASSX_TESTKEEPASS2WRITER_H

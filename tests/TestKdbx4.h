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

#ifndef KEEPASSXC_TEST_KDBX4_H
#define KEEPASSXC_TEST_KDBX4_H

#include "TestKeePass2XmlReader.h"

class TestKdbx4 : public TestKeePass2XmlReader
{
Q_OBJECT

private slots:
    virtual void initTestCase() override;

protected:
    virtual Database* readDatabase(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString) override;
    virtual Database* readDatabase(QString path, bool strictMode, bool& hasError, QString& errorString) override;
    virtual void writeDatabase(QBuffer* buf, Database* db, bool& hasError, QString& errorString) override;
};

#endif // KEEPASSXC_TEST_KDBX4_H

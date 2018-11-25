/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TESTCSVEXPORTER_H
#define KEEPASSX_TESTCSVEXPORTER_H

#include <QObject>
#include <QSharedPointer>

class Database;
class CsvExporter;

class TestCsvExporter : public QObject
{
    Q_OBJECT

public:
    static const QString ExpectedHeaderLine;

private slots:
    void init();
    void initTestCase();
    void cleanup();
    void testExport();
    void testEmptyDatabase();
    void testNestedGroups();

private:
    QSharedPointer<Database> m_db;
    QSharedPointer<CsvExporter> m_csvExporter;
};

#endif // KEEPASSX_TESTCSVEXPORTER_H

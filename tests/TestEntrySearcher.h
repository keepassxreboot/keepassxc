/*
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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


#ifndef KEEPASSX_TESTENTRYSEARCHER_H
#define KEEPASSX_TESTENTRYSEARCHER_H

#include <QObject>

#include "core/EntrySearcher.h"
#include "core/Group.h"

class TestEntrySearcher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testAndConcatenationInSearch();
    void testSearch();
    void testAllAttributesAreSearched();

private:
    Group* m_groupRoot;
    EntrySearcher m_entrySearcher;
    QList<Entry*> m_searchResult;
};

#endif // KEEPASSX_TESTENTRYSEARCHER_H

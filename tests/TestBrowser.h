/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTBROWSER_H
#define KEEPASSXC_TESTBROWSER_H

#include <QObject>

#include "browser/BrowserAction.h"
#include "browser/BrowserService.h"
#include "core/Group.h"

class TestBrowser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testChangePublicKeys();
    void testEncryptMessage();
    void testDecryptMessage();
    void testGetBase64FromKey();
    void testIncrementNonce();

    void testBaseDomain();
    void testSortPriority();
    void testSearchEntries();
    void testSearchEntriesWithPort();
    void testSearchEntriesWithAdditionalURLs();
    void testInvalidEntries();
    void testSubdomainsAndPaths();
    void testSortEntries();
    void testGetDatabaseGroups();

private:
    QScopedPointer<BrowserAction> m_browserAction;
    QScopedPointer<BrowserService> m_browserService;
};

#endif // KEEPASSXC_TESTBROWSER_H

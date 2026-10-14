/*
 *  Copyright (C) 2026 Frederic Ruget <fred@atlant.is>
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

#ifndef KEEPASSXC_TESTDEVICESTORAGE_H
#define KEEPASSXC_TESTDEVICESTORAGE_H

#include <QObject>

class TestDeviceStorage : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void testLocations();
    void testSaveAndOpen();
    void testOpenUsesTheCache();
    void testChangedVaultIsFetchedAgain();
    void testStoreFullKeepsTheDatabaseModified();
    void testPinIsAskedOnceAndForgotten();
    void testHeaderReadDoesNotAskForThePin();
    void testMissingVault();
    void testWrongPin();
    void testListVaults();
    void testStoredMasterPassword();
};

#endif // KEEPASSXC_TESTDEVICESTORAGE_H

/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTFDOSECRETSCLIENTAUTH_H
#define KEEPASSXC_TESTFDOSECRETSCLIENTAUTH_H

#include <QObject>

class MockClock;

class TestFdoSecretsClientAuth : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testRecordJsonRoundTrip();
    void testRecordJsonRejectsMalformed();
    void testRecordStoreRoundTrip();
    void testRemoveRecordSweepsEntries();
    void testEntryClientDecisions();
    void testRecordKeyProtected();
    void testMergeKeepsRecords();
    void testExeHash();
    void testRuleMatching();
    void testBuildMatchRule();
    void testFingerprintChanged();
    void testResolveOverlap();
    void testRecordsOverlap();
    void testResolverDecision();

private:
    MockClock* m_clock = nullptr;
};

#endif // KEEPASSXC_TESTFDOSECRETSCLIENTAUTH_H

/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTTPM2_H
#define KEEPASSX_TESTTPM2_H

#include <QObject>
#include <QTemporaryFile>

/**
 * Tests for the TPM2 challenge-response support that need no TPM:
 * slot encoding, challenge padding, PIN handling, quick unlock serialization
 * and the PIN prompting flow of ChallengeResponseKey.
 */
class TestTpm2 : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testSlotHelpers_data();
    void testSlotHelpers();

    void testBuildMessage_data();
    void testBuildMessage();

    void testPinFromString_data();
    void testPinFromString();

    void testPinProvider();
    void testPinProviderRunsOnMainThread();

    void testQuickUnlockPinSerialization();
    void testQuickUnlockPinIgnoredWithoutOptIn();
    void testCompositeKeyPinLifecycle();
    void testQuickUnlockTpm2PinOptionAvailability_data();
    void testQuickUnlockTpm2PinOptionAvailability();
    void testQuickUnlockStoreDecision_data();
    void testQuickUnlockStoreDecision();

    void testPinSlotWithoutPinNeverContactsTpm();
    void testKeyPromptCancelled();
    void testKeyPromptNonAuthErrorStops();
    void testNoticeIsEmptyWhenTpmDisabled();

private:
    QTemporaryFile m_configFile;
    QTemporaryFile m_localConfigFile;
};

#endif // KEEPASSX_TESTTPM2_H

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

#ifndef KEEPASSX_TESTTPM2SWTPM_H
#define KEEPASSX_TESTTPM2SWTPM_H

#include <QObject>
#include <QProcess>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QTemporaryFile>

/**
 * Exercises the TPM2 driver against a software TPM (swtpm). The test is skipped
 * when swtpm or the tpm2-tools programs used for provisioning are not installed.
 */
class TestTpm2Swtpm : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testListKeys();
    void testResponseMatchesReference_data();
    void testResponseMatchesReference();
    void testPinKeyGivesSameResponse();
    void testMissingHandle();
    void testPinSlotWithoutPinCostsNoAttempt();
    void testUnusableKeysAreExplained();
    void testPinNoticeCleanTpm();
    void testWrongPinCounted();
    void testWrongPinNoDa();
    void testPlainEntryOnPinKeyGivesHint();
    void testLockout();
    void testPlainEntryOnPinKeyIsTriedOnce();
    void testPinEntryRetriesUntilCancelled();
    void testPinCachedAcrossChallenges();
    void testQuickUnlockRestoresRememberedPin();
    void testQuickUnlockDataFollowsSettingChanges();
    void cleanup();

private:
    bool run(const QString& program, const QStringList& args);
    bool provision(uint handle, const QString& attributes, const QString& auth, const QString& policyFile = {});
    bool provisionRsa(uint handle);
    int lockoutCounter();
    bool clearLockout();

    QScopedPointer<QTemporaryDir> m_dir;
    QProcess m_swtpm;
    QByteArray m_secret;
    QTemporaryFile m_configFile;
    QTemporaryFile m_localConfigFile;
    QString m_tcti;
};

#endif // KEEPASSX_TESTTPM2SWTPM_H

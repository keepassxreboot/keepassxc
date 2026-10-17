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

#include "TestSSHAgent.h"
#include "config-keepassx-tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "sshagent/KeeAgentSettings.h"
#include "sshagent/OpenSSHKeyGen.h"
#include "sshagent/SSHAgent.h"

#include <QElapsedTimer>
#include <QTest>

QTEST_GUILESS_MAIN(TestSSHAgent)

void TestSSHAgent::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());

    // Create temporary config file
    Config::createConfigFromFile(TemporaryFile::createTempConfigFile(), {});

    // default config must not enable agent
    SSHAgent agent;
    QVERIFY(!agent.isEnabled());

    m_agentSocketFile.reset(new TemporaryFile(this));

    m_agentSocketFileName = m_agentSocketFile->fileName();
    QVERIFY(!m_agentSocketFileName.isEmpty());

    QStringList arguments;
    arguments << "-D" << "-a" << m_agentSocketFileName;

    QElapsedTimer timer;
    timer.start();

    qDebug() << "ssh-agent starting with arguments" << arguments;
    m_agentProcess.setProcessChannelMode(QProcess::ForwardedChannels);
    m_agentProcess.start("ssh-agent", arguments);
    m_agentProcess.closeWriteChannel();

    if (!m_agentProcess.waitForStarted()) {
        QSKIP("ssh-agent could not be started");
    }

    qDebug() << "ssh-agent started as pid" << m_agentProcess.processId();

    // we need to wait for the agent to open the socket before going into real tests
    QFileInfo socketFileInfo(m_agentSocketFileName);
    while (!timer.hasExpired(2000)) {
        if (socketFileInfo.exists()) {
            break;
        }
        QTest::qWait(10);
    }

    QVERIFY(socketFileInfo.exists());
    qDebug() << "ssh-agent initialized in" << timer.elapsed() << "ms";

    // initialize test key
    const QString keyString = QString("-----BEGIN OPENSSH PRIVATE KEY-----\n"
                                      "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n"
                                      "QyNTUxOQAAACDdlO5F2kF2WzedrBAHBi9wBHeISzXZ0IuIqrp0EzeazAAAAKjgCfj94An4\n"
                                      "/QAAAAtzc2gtZWQyNTUxOQAAACDdlO5F2kF2WzedrBAHBi9wBHeISzXZ0IuIqrp0EzeazA\n"
                                      "AAAEBe1iilZFho8ZGAliiSj5URvFtGrgvmnEKdiLZow5hOR92U7kXaQXZbN52sEAcGL3AE\n"
                                      "d4hLNdnQi4iqunQTN5rMAAAAH29wZW5zc2hrZXktdGVzdC1wYXJzZUBrZWVwYXNzeGMBAg\n"
                                      "MEBQY=\n"
                                      "-----END OPENSSH PRIVATE KEY-----\n");

    const QByteArray keyData = keyString.toLatin1();

    QVERIFY(m_key.parsePKCS1PEM(keyData));
}

void TestSSHAgent::init()
{
    // Reset the config state
    SSHAgent agent;
    agent.setEnabled(false);
    QString empty;
    agent.setAuthSockOverride(empty);
}

void TestSSHAgent::testConfiguration()
{
    SSHAgent agent;
    agent.setEnabled(true);
    QVERIFY(agent.isEnabled());

    // this will either be an empty string or the real ssh-agent socket path, doesn't matter
    QString defaultSocketPath = agent.socketPath(false);

    // overridden path must match default before setting an override
    QCOMPARE(agent.socketPath(true), defaultSocketPath);

    agent.setAuthSockOverride(m_agentSocketFileName);

    // overridden path must match what we set
    QCOMPARE(agent.socketPath(true), m_agentSocketFileName);

    // non-overridden path must match the default
    QCOMPARE(agent.socketPath(false), defaultSocketPath);
}

void TestSSHAgent::testIdentity()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    KeeAgentSettings settings;
    bool keyInAgent;

    // test adding a key works
    QVERIFY(agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && keyInAgent);

    // test non-conflicting key ownership doesn't throw an error
    QVERIFY(agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));

    // test conflicting key ownership throws an error
    QUuid secondUuid("{11111111-1111-1111-1111-111111111111}");
    QVERIFY(!agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, secondUuid, QUuid::createUuid())));

    // test removing a key works
    QVERIFY(agent.removeIdentity(m_key));
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testRemoveOnClose()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    KeeAgentSettings settings;
    bool keyInAgent;

    settings.setRemoveAtDatabaseClose(true);
    QVERIFY(agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && keyInAgent);
    agent.setEnabled(false);
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testLifetimeConstraint()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    KeeAgentSettings settings;
    bool keyInAgent;

    settings.setUseLifetimeConstraintWhenAdding(true);
    settings.setLifetimeConstraintDuration(2); // two seconds

    // identity should be in agent immediately after adding
    QVERIFY(agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && keyInAgent);

    QElapsedTimer timer;
    timer.start();

    // wait for the identity to time out
    while (!timer.hasExpired(5000)) {
        QVERIFY(agent.checkIdentity(m_key, keyInAgent));

        if (!keyInAgent) {
            break;
        }

        QTest::qWait(100);
    }

    QVERIFY(!keyInAgent);
}

void TestSSHAgent::testConfirmConstraint()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    KeeAgentSettings settings;
    bool keyInAgent;

    settings.setUseConfirmConstraintWhenAdding(true);

    QVERIFY(agent.addIdentity(m_key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));

    // we can't test confirmation itself is working but we can test the agent accepts the key
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && keyInAgent);

    QVERIFY(agent.removeIdentity(m_key));
    QVERIFY(agent.checkIdentity(m_key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testToOpenSSHKey()
{
    KeeAgentSettings settings;
    settings.setSelectedType("file");
    settings.setFileName(QString("%1/id_rsa-encrypted-asn1").arg(QString(KEEPASSX_TEST_DATA_DIR)));

    OpenSSHKey key;
    settings.toOpenSSHKey("username", "correctpassphrase", QString(), nullptr, key, false);

    QVERIFY(!key.publicKey().isEmpty());
}

void TestSSHAgent::testKeyGenRSA()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    OpenSSHKey key;
    KeeAgentSettings settings;
    bool keyInAgent;

    QVERIFY(OpenSSHKeyGen::generateRSA(key, 2048));

    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
    QVERIFY(agent.removeIdentity(key));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testKeyGenECDSA()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    OpenSSHKey key;
    KeeAgentSettings settings;
    bool keyInAgent;

    QVERIFY(OpenSSHKeyGen::generateECDSA(key, 256));

    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
    QVERIFY(agent.removeIdentity(key));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testKeyGenEd25519()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);

    QVERIFY(agent.isAgentRunning());

    OpenSSHKey key;
    KeeAgentSettings settings;
    bool keyInAgent;

    QVERIFY(OpenSSHKeyGen::generateEd25519(key));

    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, m_uuid, QUuid::createUuid())));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
    QVERIFY(agent.removeIdentity(key));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);
}

bool TestSSHAgent::buildTestEntry(QSharedPointer<Database>& db, Entry*& entry, OpenSSHKey& key)
{
    db = QSharedPointer<Database>::create();
    entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(db->rootGroup());

    if (!OpenSSHKeyGen::generateEd25519(key)) {
        return false;
    }
    entry->attachments()->set("id_ed25519", key.privateKey().toUtf8());

    KeeAgentSettings settings;
    settings.setAllowUseOfSshKey(true);
    settings.setAddAtDatabaseOpen(true);
    settings.setSelectedType("attachment");
    settings.setAttachmentName("id_ed25519");
    settings.toEntry(entry);

    return true;
}

void TestSSHAgent::testReloadReAddsMissingIdentity()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));

    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    auto sshKeySettings = keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid());
    QVERIFY(agent.addIdentity(key, sshKeySettings));

    bool keyInAgent = false;
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);

    QVERIFY(agent.removeIdentity(key));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);

    QVERIFY(agent.reloadAllAgentIdentities({db}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
}

void TestSSHAgent::testReloadTerminatesWhenAlreadyLoaded()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));

    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    auto sshKeySettings = keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid());
    QVERIFY(agent.addIdentity(key, sshKeySettings));

    // Regression test: reloading an already loaded entity has no effect.
    QVERIFY(agent.reloadAllAgentIdentities({db}));

    bool keyInAgent = false;
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
}

void TestSSHAgent::testReloadProcessesAllIdentitiesDespiteEarlierFailure()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    // First identity: its database will not be passed to reloadAllAgentIdentities(),
    // so it can't be recovered.
    QSharedPointer<Database> unresolvableDb;
    Entry* unresolvableEntry;
    OpenSSHKey unresolvableKey;
    QVERIFY(buildTestEntry(unresolvableDb, unresolvableEntry, unresolvableKey));
    KeeAgentSettings unresolvableSettings;
    QVERIFY(unresolvableSettings.fromEntry(unresolvableEntry));
    QVERIFY(agent.addIdentity(
        unresolvableKey, keeAgentToSshKeySettings(unresolvableSettings, unresolvableDb->uuid(), unresolvableEntry->uuid())));
    QVERIFY(agent.removeIdentity(unresolvableKey));

    // Second identity: fully reloadable.
    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));
    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid())));
    QVERIFY(agent.removeIdentity(key));

    // Regression test: an identity that can't be resolved (since unresolvableDb is not passed as an argument here) 
    // must not stop later identities from being reloaded.
    agent.reloadAllAgentIdentities({db});

    bool keyInAgent = false;
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
}

void TestSSHAgent::testReloadKeepsTrackingWhenDatabaseClosed()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));

    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid())));
    QVERIFY(agent.removeIdentity(key));

    bool keyInAgent = false;

    // Database closed: reload can't recover the key right now
    QVERIFY(agent.reloadAllAgentIdentities({}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);

    // Database open: reload should recover the key now
    QVERIFY(agent.reloadAllAgentIdentities({db}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && keyInAgent);
}

void TestSSHAgent::testReloadForgetsDeletedEntry()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));

    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid())));
    QVERIFY(agent.removeIdentity(key));

    delete entry;

    bool keyInAgent = false;
    QVERIFY(agent.reloadAllAgentIdentities({db}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::testReloadForgetsWhenSshKeyUseDisabled()
{
    SSHAgent agent;
    agent.setEnabled(true);
    agent.setAuthSockOverride(m_agentSocketFileName);
    QVERIFY(agent.isAgentRunning());

    QSharedPointer<Database> db;
    Entry* entry;
    OpenSSHKey key;
    QVERIFY(buildTestEntry(db, entry, key));

    KeeAgentSettings settings;
    QVERIFY(settings.fromEntry(entry));
    QVERIFY(agent.addIdentity(key, keeAgentToSshKeySettings(settings, db->uuid(), entry->uuid())));
    QVERIFY(agent.removeIdentity(key));

    // User disables SSH agent use for this entry before the next reload.
    settings.setAllowUseOfSshKey(false);
    settings.toEntry(entry);

    bool keyInAgent = false;
    QVERIFY(agent.reloadAllAgentIdentities({db}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);

    // Re-enabling afterwards must NOT resurrect it - the tracked identity was already dropped.
    settings.setAllowUseOfSshKey(true);
    settings.toEntry(entry);
    QVERIFY(agent.reloadAllAgentIdentities({db}));
    QVERIFY(agent.checkIdentity(key, keyInAgent) && !keyInAgent);
}

void TestSSHAgent::cleanupTestCase()
{
    if (m_agentProcess.state() != QProcess::NotRunning) {
        qDebug() << "Killing ssh-agent pid" << m_agentProcess.processId();
        m_agentProcess.terminate();
        m_agentProcess.waitForFinished();
    }
}

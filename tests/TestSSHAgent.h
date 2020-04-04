/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef TESTSSHAGENT_H
#define TESTSSHAGENT_H

#include "crypto/ssh/OpenSSHKey.h"
#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

class TestSSHAgent : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConfiguration();
    void testIdentity();
    void testRemoveOnClose();
    void testLifetimeConstraint();
    void testConfirmConstraint();
    void cleanupTestCase();

private:
    QTemporaryFile m_agentSocketFile;
    QString m_agentSocketFileName;
    QProcess m_agentProcess;
    OpenSSHKey m_key;
    QUuid m_uuid;
};

#endif // TESTSSHAGENT_H

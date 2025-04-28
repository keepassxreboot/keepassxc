/*
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
 *  Copyright (C) 2017-2021 KeePassXC Team <team@keepassxc.org>
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

#include "YubiKey.h"
#include "YubiKeyInterfacePCSC.h"
#include "YubiKeyInterfaceUSB.h"

#include <QMutexLocker>
#include <QSet>
#include <QtConcurrent>

QMutex YubiKey::s_interfaceMutex;

YubiKey::YubiKey()
{
    int num_interfaces = 0;

    if (YubiKeyInterfaceUSB::instance()->isInitialized()) {
        ++num_interfaces;
        connect(YubiKeyInterfaceUSB::instance(), SIGNAL(challengeStarted()), this, SIGNAL(challengeStarted()));
        connect(YubiKeyInterfaceUSB::instance(), SIGNAL(challengeCompleted()), this, SIGNAL(challengeCompleted()));
    } else {
        qDebug("YubiKey: USB interface is not initialized.");
    }

    if (YubiKeyInterfacePCSC::instance()->isInitialized()) {
        ++num_interfaces;
        connect(YubiKeyInterfacePCSC::instance(), SIGNAL(challengeStarted()), this, SIGNAL(challengeStarted()));
        connect(YubiKeyInterfacePCSC::instance(), SIGNAL(challengeCompleted()), this, SIGNAL(challengeCompleted()));
    } else {
        qDebug("YubiKey: PC/SC interface is disabled or not initialized.");
    }

    m_initialized = num_interfaces > 0;

    m_interactionTimer.setSingleShot(true);
    m_interactionTimer.setInterval(200);
    connect(&m_interactionTimer, SIGNAL(timeout()), this, SIGNAL(userInteractionRequest()));
    connect(this, &YubiKey::challengeStarted, this, [this] { m_interactionTimer.start(); });
    connect(this, &YubiKey::challengeCompleted, this, [this] { m_interactionTimer.stop(); });
}

YubiKey* YubiKey::m_instance(nullptr);

YubiKey* YubiKey::instance()
{
    if (!m_instance) {
        m_instance = new YubiKey();
    }

    return m_instance;
}

bool YubiKey::isInitialized()
{
    return m_initialized;
}

bool YubiKey::findValidKeys()
{
    // Block operations on hardware keys while scanning
    QMutexLocker lock(&s_interfaceMutex);

    m_connectedKeys = 0;
    m_findingKeys = true;
    m_usbKeys = YubiKeyInterfaceUSB::instance()->findValidKeys(m_connectedKeys);
    m_pcscKeys = YubiKeyInterfacePCSC::instance()->findValidKeys(m_connectedKeys);
    m_findingKeys = false;

    return !m_usbKeys.isEmpty() || !m_pcscKeys.isEmpty();
}

void YubiKey::findValidKeysAsync()
{
    // Don't start another scan if we are already doing one
    if (!m_findingKeys) {
        m_findingKeys = true;
        QtConcurrent::run([this] { emit detectComplete(findValidKeys()); });
    }
}

YubiKey::KeyMap YubiKey::foundKeys()
{
    KeyMap foundKeys = m_usbKeys;
    foundKeys.unite(m_pcscKeys);

    return foundKeys;
}

int YubiKey::connectedKeys()
{
    return m_connectedKeys;
}

QString YubiKey::errorMessage()
{
    return m_error;
}

/**
 * Issue a test challenge to the specified slot to determine if challenge
 * response is properly configured.
 *
 * @param slot YubiKey configuration slot
 * @param wouldBlock return if the operation requires user input
 * @return whether the challenge succeeded
 */
bool YubiKey::testChallenge(YubiKeySlot slot, bool* wouldBlock)
{
    QMutexLocker lock(&s_interfaceMutex);

    if (m_usbKeys.contains(slot)) {
        return YubiKeyInterfaceUSB::instance()->testChallenge(slot, wouldBlock);
    }

    if (m_pcscKeys.contains(slot)) {
        return YubiKeyInterfacePCSC::instance()->testChallenge(slot, wouldBlock);
    }

    return false;
}

/**
 * Issue a challenge to the specified slot
 * This operation could block if the YubiKey requires a touch to trigger.
 *
 * @param slot YubiKey configuration slot
 * @param challenge challenge input to YubiKey
 * @param response response output from YubiKey
 * @return challenge result
 */
YubiKey::ChallengeResult
YubiKey::challenge(YubiKeySlot slot, const QByteArray& challenge, Botan::secure_vector<char>& response)
{
    m_error.clear();

    // Prevent re-entrant access to hardware keys
    QMutexLocker lock(&s_interfaceMutex);

    // Try finding key on the USB interface first
    auto ret = YubiKeyInterfaceUSB::instance()->challenge(slot, challenge, response);
    if (ret == ChallengeResult::YCR_ERROR) {
        m_error = YubiKeyInterfaceUSB::instance()->errorMessage();
        return ret;
    }

    // If a USB key was not found, try PC/SC interface
    if (ret == ChallengeResult::YCR_KEYNOTFOUND) {
        ret = YubiKeyInterfacePCSC::instance()->challenge(slot, challenge, response);
        if (ret == ChallengeResult::YCR_ERROR) {
            m_error = YubiKeyInterfacePCSC::instance()->errorMessage();
            return ret;
        }
    }

    if (ret == ChallengeResult::YCR_KEYNOTFOUND) {
        m_error =
            tr("Could not find hardware key with serial number %1. Please connect it to continue.").arg(slot.first);
    }

    return ret;
}

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

#include <QtConcurrent>

YubiKey::YubiKey()
    : m_interfaces_detect_mutex(QMutex::Recursive)
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
        qDebug("YubiKey: PCSC interface is disabled or not initialized.");
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
    bool found = false;
    if (m_interfaces_detect_mutex.tryLock(1000)) {
        found |= YubiKeyInterfaceUSB::instance()->findValidKeys();
        found |= YubiKeyInterfacePCSC::instance()->findValidKeys();
        m_interfaces_detect_mutex.unlock();
    }
    return found;
}

void YubiKey::findValidKeysAsync()
{
    QtConcurrent::run([this] {
        bool found = findValidKeys();
        emit detectComplete(found);
    });
}

QList<YubiKeySlot> YubiKey::foundKeys()
{
    QList<YubiKeySlot> foundKeys;

    auto keys = YubiKeyInterfaceUSB::instance()->foundKeys();
    QList<unsigned int> handledSerials = keys.uniqueKeys();
    for (auto serial : handledSerials) {
        for (const auto& key : keys.values(serial)) {
            foundKeys.append({serial, key.first});
        }
    }

    keys = YubiKeyInterfacePCSC::instance()->foundKeys();
    for (auto serial : keys.uniqueKeys()) {
        // Ignore keys that were detected on USB interface already
        if (handledSerials.contains(serial)) {
            continue;
        }

        for (const auto& key : keys.values(serial)) {
            foundKeys.append({serial, key.first});
        }
    }

    return foundKeys;
}

QString YubiKey::getDisplayName(YubiKeySlot slot)
{
    QString name;
    name.clear();

    if (YubiKeyInterfaceUSB::instance()->hasFoundKey(slot)) {
        name += YubiKeyInterfaceUSB::instance()->getDisplayName(slot);
    }

    if (YubiKeyInterfacePCSC::instance()->hasFoundKey(slot)) {
        // In some cases, the key might present on two interfaces
        // This should usually never happen, because the PCSC interface
        // filters the "virtual yubikey reader device".
        if (!name.isNull()) {
            name += " = ";
        }
        name += YubiKeyInterfacePCSC::instance()->getDisplayName(slot);
    }

    if (!name.isNull()) {
        return name;
    }

    return tr("%1 No interface, slot %2").arg(QString::number(slot.first), QString::number(slot.second));
}

QString YubiKey::errorMessage()
{
    QString error;
    error.clear();
    if (!m_error.isNull()) {
        error += tr("General: ") + m_error;
    }

    QString usb_error = YubiKeyInterfaceUSB::instance()->errorMessage();
    if (!usb_error.isNull()) {
        if (!error.isNull()) {
            error += " | ";
        }
        error += "USB: " + usb_error;
    }

    QString pcsc_error = YubiKeyInterfacePCSC::instance()->errorMessage();
    if (!pcsc_error.isNull()) {
        if (!error.isNull()) {
            error += " | ";
        }
        error += "PCSC: " + pcsc_error;
    }

    return error;
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
    if (YubiKeyInterfaceUSB::instance()->hasFoundKey(slot)) {
        return YubiKeyInterfaceUSB::instance()->testChallenge(slot, wouldBlock);
    }

    if (YubiKeyInterfacePCSC::instance()->hasFoundKey(slot)) {
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

    // Make sure we tried to find available keys
    if (foundKeys().isEmpty()) {
        findValidKeys();
    }

    if (YubiKeyInterfaceUSB::instance()->hasFoundKey(slot)) {
        return YubiKeyInterfaceUSB::instance()->challenge(slot, challenge, response);
    }

    if (YubiKeyInterfacePCSC::instance()->hasFoundKey(slot)) {
        return YubiKeyInterfacePCSC::instance()->challenge(slot, challenge, response);
    }

    m_error = tr("Could not find interface for hardware key with serial number %1. Please connect it to continue.")
                  .arg(slot.first);

    return YubiKey::ChallengeResult::YCR_ERROR;
}

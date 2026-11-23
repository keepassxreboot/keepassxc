/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
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
#include "YubiKeyInterfaceTPM.h"
#include "YubiKeyInterfaceUSB.h"

#include "core/Config.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QSet>
#include <QStringConverter>
#include <QThread>
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

    if (YubiKeyInterfaceTPM::instance()->isInitialized()) {
        connect(YubiKeyInterfaceTPM::instance(), SIGNAL(challengeStarted()), this, SIGNAL(challengeStarted()));
        connect(YubiKeyInterfaceTPM::instance(), SIGNAL(challengeCompleted()), this, SIGNAL(challengeCompleted()));
    }

    m_initialized = num_interfaces > 0;

    m_interactionTimer.setSingleShot(true);
    m_interactionTimer.setInterval(200);
    connect(&m_interactionTimer, SIGNAL(timeout()), this, SIGNAL(userInteractionRequest()));
    connect(this, &YubiKey::challengeStarted, this, [this] { m_interactionTimer.start(); });
    connect(this, &YubiKey::challengeCompleted, this, [this] { m_interactionTimer.stop(); });
}

YubiKey* YubiKey::m_instance(nullptr);

namespace
{
    YubiKey::PinProvider s_pinProvider;
}

void YubiKey::setPinProvider(PinProvider provider)
{
    s_pinProvider = std::move(provider);
}

Botan::secure_vector<char> YubiKey::pinFromString(const QString& text)
{
    QStringEncoder encoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    Botan::secure_vector<char> out(static_cast<size_t>(encoder.requiredSpace(text.size())));
    char* end = encoder.appendToBuffer(out.data(), text);
    out.resize(static_cast<size_t>(end - out.data()));
    return out;
}

bool YubiKey::requestPin(const YubiKeySlot& slot,
                         const QString& message,
                         Botan::secure_vector<char>& pin,
                         bool& remember)
{
    if (!s_pinProvider) {
        return false;
    }

    // Dialogs must be shown from the GUI thread
    if (QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread()) {
        bool ok = false;
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [&] { ok = s_pinProvider(slot, message, pin, remember); },
            Qt::BlockingQueuedConnection);
        return ok;
    }
    return s_pinProvider(slot, message, pin, remember);
}

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
    m_tpmKeys.clear();
    m_tpmKeysHint.clear();
    if (tpmEnabled()) {
        m_tpmKeys = YubiKeyInterfaceTPM::instance()->findValidKeys(m_connectedKeys);
        m_tpmKeysHint = YubiKeyInterfaceTPM::instance()->unusableKeysHint();
    }
    m_findingKeys = false;

    return !m_usbKeys.isEmpty() || !m_pcscKeys.isEmpty() || !m_tpmKeys.isEmpty();
}

void YubiKey::findValidKeysAsync()
{
    // Don't start another scan if we are already doing one
    if (!m_findingKeys) {
        m_findingKeys = true;
        auto future = QtConcurrent::run([this] { emit detectComplete(findValidKeys()); });
        Q_UNUSED(future)
    }
}

YubiKey::KeyMap YubiKey::foundKeys()
{
    KeyMap foundKeys = m_usbKeys;
    foundKeys.unite(m_pcscKeys);
    foundKeys.unite(m_tpmKeys);

    return foundKeys;
}

int YubiKey::connectedKeys()
{
    return m_connectedKeys;
}

bool YubiKey::tpmEnabled() const
{
    return config()->get(Config::Security_EnableTpm2).toBool();
}

QString YubiKey::pinNotice(const YubiKeySlot& slot)
{
    if (!tpmSlotNeedsPin(slot) || !tpmEnabled()) {
        return {};
    }
    QMutexLocker lock(&s_interfaceMutex);
    return YubiKeyInterfaceTPM::instance()->pinNotice(slot);
}

QString YubiKey::unusableTpmKeysHint() const
{
    return m_tpmKeysHint;
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
bool YubiKey::testChallenge(YubiKeySlot slot,
                            bool* wouldBlock,
                            const Botan::secure_vector<char>* pin,
                            ChallengeResult* result)
{
    QMutexLocker lock(&s_interfaceMutex);

    if (isTpmSlot(slot)) {
        m_error.clear();
        if (!tpmEnabled()) {
            return false;
        }
        const bool ok = YubiKeyInterfaceTPM::instance()->testChallengeWithPin(slot, wouldBlock, pin, result);
        if (!ok) {
            m_error = YubiKeyInterfaceTPM::instance()->errorMessage();
        }
        return ok;
    }

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
 * @param pin optional PIN/password for TPM2 keys that require one
 * @return challenge result
 */
YubiKey::ChallengeResult YubiKey::challenge(YubiKeySlot slot,
                                            const QByteArray& challenge,
                                            Botan::secure_vector<char>& response,
                                            const Botan::secure_vector<char>* pin)
{
    m_error.clear();

    // Prevent re-entrant access to hardware keys
    QMutexLocker lock(&s_interfaceMutex);

    // TPM2 keys are addressed by a dedicated slot range and never go through USB/PCSC
    if (isTpmSlot(slot)) {
        if (!tpmEnabled()) {
            m_error = tr("TPM2 keys are disabled in the application settings.");
            return ChallengeResult::YCR_ERROR;
        }
        auto tpmRet = YubiKeyInterfaceTPM::instance()->challengeWithPin(slot, challenge, response, pin);
        if (tpmRet == ChallengeResult::YCR_ERROR || tpmRet == ChallengeResult::YCR_AUTHFAILED) {
            m_error = YubiKeyInterfaceTPM::instance()->errorMessage();
        } else if (tpmRet == ChallengeResult::YCR_KEYNOTFOUND) {
            // The driver says what is wrong with the key (missing, bound to a policy, ...)
            m_error = YubiKeyInterfaceTPM::instance()->errorMessage();
            if (m_error.isEmpty()) {
                m_error = tr("Could not find TPM2 key at handle 0x%1 or it is not a usable HMAC-SHA1 key.")
                              .arg(slot.first, 8, 16, QLatin1Char('0'));
            }
        }
        return tpmRet;
    }

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

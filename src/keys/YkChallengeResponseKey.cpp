/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "keys/YkChallengeResponseKey.h"
#include "keys/drivers/YubiKey.h"

#include "core/AsyncTask.h"
#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QXmlStreamReader>
#include <QtConcurrent>

#include <cstring>
#include <gcrypt.h>
#include <sodium.h>

QUuid YkChallengeResponseKey::UUID("e092495c-e77d-498b-84a1-05ae0d955508");

YkChallengeResponseKey::YkChallengeResponseKey(int slot, bool blocking)
    : ChallengeResponseKey(UUID)
    , m_slot(slot)
    , m_blocking(blocking)
{
    if (getMainWindow()) {
        connect(this, SIGNAL(userInteractionRequired()), getMainWindow(), SLOT(showYubiKeyPopup()));
        connect(this, SIGNAL(userConfirmed()), getMainWindow(), SLOT(hideYubiKeyPopup()));
    }
}

YkChallengeResponseKey::~YkChallengeResponseKey()
{
    if (m_key) {
        gcry_free(m_key);
        m_keySize = 0;
        m_key = nullptr;
    }
}

QByteArray YkChallengeResponseKey::rawKey() const
{
    return QByteArray::fromRawData(m_key, static_cast<int>(m_keySize));
}

/**
 * Assumes yubikey()->init() was called
 */
bool YkChallengeResponseKey::challenge(const QByteArray& challenge)
{
    return this->challenge(challenge, 2);
}

bool YkChallengeResponseKey::challenge(const QByteArray& challenge, unsigned int retries)
{
    do {
        --retries;

        if (m_blocking) {
            emit userInteractionRequired();
        }

        QByteArray key;
        auto result = AsyncTask::runAndWaitForFuture(
            [this, challenge, &key]() { return YubiKey::instance()->challenge(m_slot, true, challenge, key); });

        if (m_blocking) {
            emit userConfirmed();
        }

        if (result == YubiKey::SUCCESS) {
            if (m_key) {
                gcry_free(m_key);
            }
            m_keySize = static_cast<std::size_t>(key.size());
            m_key = static_cast<char*>(gcry_malloc_secure(m_keySize));
            std::memcpy(m_key, key.data(), m_keySize);
            sodium_memzero(key.data(), static_cast<std::size_t>(key.capacity()));
            return true;
        }
    } while (retries > 0);

    return false;
}

QString YkChallengeResponseKey::getName() const
{
    unsigned int serial;
    QString fmt(QObject::tr("%1[%2] Challenge Response - Slot %3 - %4"));

    YubiKey::instance()->getSerial(serial);

    return fmt.arg(YubiKey::instance()->getVendorName(),
                   QString::number(serial),
                   QString::number(m_slot),
                   (m_blocking) ? QObject::tr("Press") : QObject::tr("Passive"));
}

bool YkChallengeResponseKey::isBlocking() const
{
    return m_blocking;
}

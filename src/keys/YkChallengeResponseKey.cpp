/*
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

QByteArray YkChallengeResponseKey::rawKey() const
{
    return m_key;
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

        auto result = AsyncTask::runAndWaitForFuture(
            [this, challenge]() { return YubiKey::instance()->challenge(m_slot, true, challenge, m_key); });

        if (m_blocking) {
            emit userConfirmed();
        }

        if (result == YubiKey::SUCCESS) {
            return true;
        }
    } while (retries > 0);

    return false;
}

QString YkChallengeResponseKey::getName() const
{
    unsigned int serial;
    QString fmt(QObject::tr("YubiKey[%1] Challenge Response - Slot %2 - %3"));

    YubiKey::instance()->getSerial(serial);

    return fmt.arg(
        QString::number(serial), QString::number(m_slot), (m_blocking) ? QObject::tr("Press") : QObject::tr("Passive"));
}

bool YkChallengeResponseKey::isBlocking() const
{
    return m_blocking;
}

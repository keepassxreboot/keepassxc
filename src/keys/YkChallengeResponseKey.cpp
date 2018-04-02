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

YkChallengeResponseKey::YkChallengeResponseKey(int slot, bool blocking)
    : m_slot(slot)
    , m_blocking(blocking)
{
    if (KEEPASSXC_MAIN_WINDOW) {
        connect(this, SIGNAL(userInteractionRequired()), KEEPASSXC_MAIN_WINDOW, SLOT(showYubiKeyPopup()));
        connect(this, SIGNAL(userConfirmed()), KEEPASSXC_MAIN_WINDOW, SLOT(hideYubiKeyPopup()));
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

bool YkChallengeResponseKey::challenge(const QByteArray& challenge, unsigned retries)
{
    Q_ASSERT(retries > 0);

    do {
        --retries;

        if (m_blocking) {
            emit userInteractionRequired();
        }

        QFuture<YubiKey::ChallengeResult> future = QtConcurrent::run(
            [this, challenge]() { return YubiKey::instance()->challenge(m_slot, true, challenge, m_key); });

        QEventLoop loop;
        QFutureWatcher<YubiKey::ChallengeResult> watcher;
        connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
        watcher.setFuture(future);
        loop.exec();

        if (m_blocking) {
            emit userConfirmed();
        }

        if (future.result() != YubiKey::ERROR) {
            return true;
        }

        // if challenge failed, retry to detect YubiKeys in the event the YubiKey was un-plugged and re-plugged
        if (retries > 0 && !YubiKey::instance()->init()) {
            continue;
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

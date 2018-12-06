/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "keys/YkChallengeResponseKeyCLI.h"
#include "keys/drivers/YubiKey.h"

#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QtCore/qglobal.h>

QUuid YkChallengeResponseKeyCLI::UUID("e2be77c0-c810-417a-8437-32f41d00bd1d");

YkChallengeResponseKeyCLI::YkChallengeResponseKeyCLI(
        int slot,
        bool blocking,
        QString messageInteraction,
        QString messageSuccess,
        QString messageFailure,
        FILE* outputDescriptor,
        FILE* errorDescriptor)
    : ChallengeResponseKey(UUID)
    , m_slot(slot)
    , m_blocking(blocking)
    , m_messageInteraction(messageInteraction)
    , m_messageSuccess(messageSuccess)
    , m_messageFailure(messageFailure)
    , m_out(outputDescriptor)
    , m_err(errorDescriptor)
{
}

QByteArray YkChallengeResponseKeyCLI::rawKey() const
{
    return m_key;
}

/**
 * Assumes yubikey()->init() was called
 */
bool YkChallengeResponseKeyCLI::challenge(const QByteArray& challenge)
{
    return this->challenge(challenge, 2);
}

bool YkChallengeResponseKeyCLI::challenge(const QByteArray& challenge, unsigned int retries)
{
    QTextStream out(m_out, QIODevice::WriteOnly);
    QTextStream err(m_err, QIODevice::WriteOnly);
    do {
        --retries;

        if (m_blocking) {
            out << m_messageInteraction << endl;
        }
        YubiKey::ChallengeResult result = YubiKey::instance()->challenge(m_slot, m_blocking, challenge, m_key);
        if (result == YubiKey::SUCCESS) {
            if (m_blocking) {
                out << m_messageSuccess << endl;
            }
            return true;
        }
    } while (retries > 0);

    err << m_messageFailure << endl;
    return false;
}

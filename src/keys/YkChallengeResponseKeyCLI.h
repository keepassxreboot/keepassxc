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

#ifndef KEEPASSX_YK_CHALLENGERESPONSEKEYCLI_H
#define KEEPASSX_YK_CHALLENGERESPONSEKEYCLI_H

#include "core/Global.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/drivers/YubiKey.h"

#include <QObject>
#include <QTextStream>

class YkChallengeResponseKeyCLI : public QObject, public ChallengeResponseKey
{
    Q_OBJECT

public:
    static QUuid UUID;

    explicit YkChallengeResponseKeyCLI(int slot, bool blocking, QString messageInteraction, FILE* outputDescriptor);

    QByteArray rawKey() const override;
    bool challenge(const QByteArray& challenge) override;
    bool challenge(const QByteArray& challenge, unsigned int retries);

private:
    QByteArray m_key;
    int m_slot;
    bool m_blocking;
    QString m_messageInteraction;
    FILE* m_out;
};

#endif // KEEPASSX_YK_CHALLENGERESPONSEKEYCLI_H

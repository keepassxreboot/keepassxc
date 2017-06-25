/*
*  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_COMPOSITEKEY_H
#define KEEPASSX_COMPOSITEKEY_H

#include <QList>
#include <QString>
#include <QSharedPointer>

#include "keys/Key.h"
#include "keys/ChallengeResponseKey.h"

class CompositeKey : public Key
{
public:
    CompositeKey();
    CompositeKey(const CompositeKey& key);
    ~CompositeKey();
    void clear();
    bool isEmpty() const;
    CompositeKey* clone() const;
    CompositeKey& operator=(const CompositeKey& key);

    QByteArray rawKey() const;
    QByteArray transform(const QByteArray& seed, quint64 rounds,
                         bool* ok, QString* errorString) const;
    bool challenge(const QByteArray& seed, QByteArray &result) const;

    void addKey(const Key& key);
    void addChallengeResponseKey(QSharedPointer<ChallengeResponseKey> key);

    static int transformKeyBenchmark(int msec);
    static CompositeKey readFromLine(QString line);

private:
    static QByteArray transformKeyRaw(const QByteArray& key, const QByteArray& seed,
                                      quint64 rounds, bool* ok, QString* errorString);

    QList<Key*> m_keys;
    QList<QSharedPointer<ChallengeResponseKey>> m_challengeResponseKeys;
};

#endif // KEEPASSX_COMPOSITEKEY_H

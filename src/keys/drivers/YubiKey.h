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

#ifndef KEEPASSX_YUBIKEY_H
#define KEEPASSX_YUBIKEY_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QTimer>

typedef QPair<unsigned int, int> YubiKeySlot;
Q_DECLARE_METATYPE(YubiKeySlot);

/**
 * Singleton class to manage the interface to hardware key(s)
 */
class YubiKey : public QObject
{
    Q_OBJECT

public:
    enum ChallengeResult
    {
        ERROR,
        SUCCESS,
        WOULDBLOCK
    };

    static YubiKey* instance();
    bool isInitialized();

    void findValidKeys();

    QList<YubiKeySlot> foundKeys();
    QString getDisplayName(YubiKeySlot slot);

    ChallengeResult challenge(YubiKeySlot slot, const QByteArray& challenge, QByteArray& response);
    bool testChallenge(YubiKeySlot slot, bool* wouldBlock = nullptr);

    QString errorMessage();

signals:
    /**
     * Emitted when a detection process completes. Use the `detectedSlots`
     * accessor function to get information on the available slots.
     *
     * @param found - true if a key was found
     */
    void detectComplete(bool found);

    /**
     * Emitted when user needs to interact with the hardware key to continue
     */
    void userInteractionRequest();

    /**
     * Emitted before/after a challenge-response is performed
     */
    void challengeStarted();
    void challengeCompleted();

    /**
     * Emitted when an error occurred during challenge/response
     */
    void challengeError(QString error);

private:
    explicit YubiKey();
    ~YubiKey();

    static YubiKey* m_instance;

    ChallengeResult
    performChallenge(void* key, int slot, bool mayBlock, const QByteArray& challenge, QByteArray& response);
    bool performTestChallenge(void* key, int slot, bool* wouldBlock);

    QHash<unsigned int, QList<QPair<int, QString>>> m_foundKeys;

    QMutex m_mutex;
    QTimer m_interactionTimer;
    bool m_initialized = false;
    QString m_error;

    Q_DISABLE_COPY(YubiKey)
};

#endif // KEEPASSX_YUBIKEY_H

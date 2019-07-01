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

#include <QMutex>
#include <QObject>

/**
 * Singleton class to manage the interface to the hardware
 */
class YubiKey : public QObject
{
    Q_OBJECT

public:
    enum ChallengeResult
    {
        ERROR = -1,
        SUCCESS = 0,
        WOULDBLOCK,
        ALREADY_RUNNING
    };

    /**
     * @brief YubiKey::instance - get instance of singleton
     * @return instance
     */
    static YubiKey* instance();

    /**
     * @brief YubiKey::init - initialize yubikey library and hardware
     * @return true on success
     */
    bool init();

    /**
     * @brief YubiKey::deinit - cleanup after init
     * @return true on success
     */
    bool deinit();

    /**
     * @brief YubiKey::challenge - issue a challenge
     *
     * This operation could block if the YubiKey requires a touch to trigger.
     *
     * TODO: Signal to the UI that the system is waiting for challenge response
     *       touch.
     *
     * @param slot YubiKey configuration slot
     * @param mayBlock operation is allowed to block
     * @param challenge challenge input to YubiKey
     * @param response response output from YubiKey
     * @return challenge result
     */
    ChallengeResult challenge(int slot, bool mayBlock, const QByteArray& challenge, QByteArray& response);

    /**
     * @brief YubiKey::getSerial - serial number of YubiKey
     * @param serial serial number
     * @return true on success
     */
    bool getSerial(unsigned int& serial);

    /**
     * @brief YubiKey::getVendorName - vendor name of token
     * @return vendor name
     */
    QString getVendorName();

    /**
     * @brief YubiKey::detect - probe for attached YubiKeys
     */
    void detect();

signals:
    /** Emitted in response to detect() when a device is found
     *
     * @slot is the slot number detected
     * @blocking signifies if the YK is setup in passive mode or if requires
     *           the user to touch it for a response
     */
    void detected(int slot, bool blocking);

    /**
     * Emitted when detection is complete
     */
    void detectComplete();

    /**
     * Emitted when no Yubikey could be found.
     */
    void notFound();

private:
    explicit YubiKey();
    static YubiKey* m_instance;

    // Create void ptr here to avoid ifdef header include mess
    void* m_yk_void;
    void* m_ykds_void;
    bool m_onlyKey;

    QMutex m_mutex;

    Q_DISABLE_COPY(YubiKey)
};

#endif // KEEPASSX_YUBIKEY_H

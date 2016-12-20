/*
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

#ifndef KEEPASSX_YUBIKEY_H
#define KEEPASSX_YUBIKEY_H

#include <QObject>

/**
 * Singleton class to manage the interface to the hardware
 */
class YubiKey : public QObject
{
    Q_OBJECT

public:
    enum ChallengeResult { ERROR = -1, SUCCESS = 0, WOULDBLOCK };

    static YubiKey* instance();

    /** Initialize the underlying yubico libraries */
    bool init();
    bool deinit();

    /** Issue a challenge to the hardware */
    ChallengeResult challenge(int slot, bool mayBlock,
                              const QByteArray& chal,
                              QByteArray& resp) const;

    /** Read the serial number from the hardware */
    bool getSerial(unsigned int& serial) const;

    /** Start looking for attached hardware devices */
    void detect();

Q_SIGNALS:
    /** Emitted in response to detect() when a device is found
     *
     * @slot is the slot number detected
     * @blocking signifies if the YK is setup in passive mode or if requires
     *           the user to touch it for a response
     */
    void detected(int slot, bool blocking);

private:
    explicit YubiKey();
    static YubiKey* m_instance;

    /* Create void ptr here to avoid ifdef header include mess */
    void *m_yk_void;
    void *m_ykds_void;

    Q_DISABLE_COPY(YubiKey)
};

#endif // KEEPASSX_YUBIKEY_H

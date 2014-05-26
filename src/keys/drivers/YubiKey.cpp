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

#include <stdio.h>

#include <QDebug>

#include <ykcore.h>
#include <yubikey.h>
#include <ykdef.h>
#include <ykstatus.h>

#include "core/Global.h"
#include "crypto/Random.h"

#include "YubiKey.h"

/* Cast the void pointer from the generalized class definition
 * to the proper pointer type from the now included system headers
 */
#define m_yk (static_cast<YK_KEY*>(m_yk_void))
#define m_ykds (static_cast<YK_STATUS*>(m_ykds_void))

YubiKey::YubiKey() : m_yk_void(NULL), m_ykds_void(NULL)
{
}

YubiKey* YubiKey::m_instance(Q_NULLPTR);

/**
 * @brief YubiKey::instance - get instance of singleton
 * @return
 */
YubiKey* YubiKey::instance()
{
    if (!m_instance) {
        m_instance = new YubiKey();
    }

    return m_instance;
}

/**
 * @brief YubiKey::init - initialize yubikey library and hardware
 * @return
 */
bool YubiKey::init()
{
    /* Previously initalized */
    if (m_yk != NULL && m_ykds != NULL) {

        if (yk_get_status(m_yk, m_ykds)) {
            /* Still connected */
            return true;
        } else {
            /* Initialized but not connected anymore, re-init */
            deinit();
        }
    }

    if (!yk_init()) {
        return false;
    }

    /* TODO: handle multiple attached hardware devices, currently own one */
    m_yk_void = static_cast<void*>(yk_open_first_key());
    if (m_yk == NULL) {
        return false;
    }

    m_ykds_void = static_cast<void*>(ykds_alloc());
    if (m_ykds == NULL) {
        yk_close_key(m_yk);
        m_yk_void = NULL;
        return false;
    }

    return true;
}

/**
 * @brief YubiKey::deinit - cleanup after init
 * @return true on success
 */
bool YubiKey::deinit()
{
    if (m_yk) {
        yk_close_key(m_yk);
        m_yk_void = NULL;
    }

    if (m_ykds) {
      ykds_free(m_ykds);
      m_ykds_void = NULL;
    }

    return true;
}

/**
 * @brief YubiKey::detect - probe for attached YubiKeys
 */
void YubiKey::detect()
{
    if (init()) {

        for (int i = 1; i < 3; i++) {
            YubiKey::ChallengeResult result;
            QByteArray rand = randomGen()->randomArray(1);
            QByteArray resp;

            result = challenge(i, false, rand, resp);

            if (result != YubiKey::ERROR) {
                Q_EMIT detected(i, result == YubiKey::WOULDBLOCK ? true : false);
            }
        }
    }
}

/**
 * @brief YubiKey::getSerial - serial number of yubikey
 * @param serial
 * @return
 */
bool YubiKey::getSerial(unsigned int& serial) const
{
    if (!yk_get_serial(m_yk, 1, 0, &serial)) {
        return false;
    }

    return true;
}

#ifdef QT_DEBUG
/**
 * @brief printByteArray - debug raw data
 * @param a array input
 * @return string representation of array
 */
static inline QString printByteArray(const QByteArray& a)
{
    QString s;
    for (int i = 0; i < a.size(); i++)
        s.append(QString::number(a[i] & 0xff, 16).rightJustified(2, '0'));
    return s;
}
#endif

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
 * @param chal challenge input to YubiKey
 * @param resp response output from YubiKey
 * @return SUCCESS when successful
 */
YubiKey::ChallengeResult YubiKey::challenge(int slot, bool mayBlock,
                                            const QByteArray& chal,
                                            QByteArray& resp) const
{
    int yk_cmd = (slot == 1) ? SLOT_CHAL_HMAC1 : SLOT_CHAL_HMAC2;
    QByteArray paddedChal = chal;

    /* yk_challenge_response() insists on 64 byte response buffer */
    resp.resize(64);

    /* The challenge sent to the yubikey should always be 64 bytes for
     * compatibility with all configurations.  Follow PKCS7 padding.
     *
     * There is some question whether or not 64 byte fixed length
     * configurations even work, some docs say avoid it.
     */
    const int padLen = 64 - paddedChal.size();
    if (padLen > 0) {
        paddedChal.append(QByteArray(padLen, padLen));
    }

    const unsigned char *c;
    unsigned char *r;
    c = reinterpret_cast<const unsigned char*>(paddedChal.constData());
    r = reinterpret_cast<unsigned char*>(resp.data());

#ifdef QT_DEBUG
    qDebug().nospace() << __func__ << "(" << slot << ") c = "
                       << printByteArray(paddedChal);
#endif

    int ret = yk_challenge_response(m_yk, yk_cmd, mayBlock,
                                    paddedChal.size(), c,
                                    resp.size(), r);

    if(!ret) {
        if (yk_errno == YK_EWOULDBLOCK) {
            return WOULDBLOCK;
        } else if (yk_errno == YK_ETIMEOUT) {
            return ERROR;
        } else if (yk_errno) {

            /* Something went wrong, close the key, so that the next call to
             * can try to re-open.
             *
             * Likely caused by the YubiKey being unplugged.
             */

            if (yk_errno == YK_EUSBERR) {
                qWarning() << "USB error:" << yk_usb_strerror();
            } else {
                qWarning() << "YubiKey core error:" << yk_strerror(yk_errno);
            }

            return ERROR;
        }
    }

    /* Actual HMAC-SHA1 response is only 20 bytes */
    resp.resize(20);

#ifdef QT_DEBUG
    qDebug().nospace() << __func__ << "(" << slot << ") r = "
                       << printByteArray(resp) << ", ret = " << ret;
#endif

    return SUCCESS;
}

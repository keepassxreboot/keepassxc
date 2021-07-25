/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include <QtConcurrent>

#include "core/Tools.h"
#include "crypto/Random.h"

#include "YubiKeyInterfacePCSC.h"

// MSYS2 does not define these macros
// So set them to the value used by pcsc-lite
#ifndef MAX_ATR_SIZE
#define MAX_ATR_SIZE 33
#endif
#ifndef MAX_READERNAME
#define MAX_READERNAME 128
#endif

// PCSC framework on OSX uses unsigned int
// Windows winscard and Linux pcsc-lite use unsigned long
#ifdef __APPLE__
typedef uint32_t SCUINT;
#else
typedef unsigned long SCUINT;
#endif

// This namescape contains static wrappers for the smart card API
// Which enable the communication with a Yubikey via PCSC ADPUs
namespace
{

    /***
     * @brief Returns the names of all connected smartcard readers
     *
     * @param context A pre-established smartcard API context
     * @returns New list of smartcard readers
     */
    QList<QString> getReaders(SCARDCONTEXT context)
    {
        QList<QString> readers_list;
        SCUINT dwReaders = 0;

        // Read size of required string buffer
        // OSX does not support auto-allocate
        SCardListReaders(context, NULL, NULL, &dwReaders);
        if (dwReaders == 0 || dwReaders > 16384) { // max 16kb
            return readers_list;
        }
        char* mszReaders = new char[dwReaders + 2];

        int32_t rv = SCardListReaders(context, NULL, mszReaders, &dwReaders);
        if (rv == SCARD_S_SUCCESS) {
            char* readhead = mszReaders;
            // Names are seperated by a null byte
            // The list is terminated by two null bytes
            while (*readhead != '\0') {
                QString reader = QString::fromUtf8(readhead);
                readers_list.append(reader);
                readhead += reader.size() + 1;
            }
        }

        delete[] mszReaders;
        return readers_list;
    }

    /***
     * @brief Reads the status of a smartcard handle
     *
     * This function does not actually transmit data,
     * instead it only reads the OS API state
     *
     * @param handle Smartcard handle
     * @param dwProt Protocol currently used
     * @param pioSendPci Pointer to the PCI header used for sending
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t getCardStatus(SCARDHANDLE handle, SCUINT& dwProt, const SCARD_IO_REQUEST*& pioSendPci)
    {
        int32_t rv = SCARD_E_UNEXPECTED;

        uint8_t pbAtr[MAX_ATR_SIZE] = {0}; // ATR record
        char pbReader[MAX_READERNAME] = {0}; // Name of the reader the card is placed in
        SCUINT dwAtrLen = sizeof(pbAtr); // ATR record size
        SCUINT dwReaderLen = sizeof(pbReader); // String length of the reader name
        SCUINT dwState = 0; // Unused. Contents differ depending on API implementation.

        if ((rv = SCardStatus(handle, pbReader, &dwReaderLen, &dwState, &dwProt, pbAtr, &dwAtrLen))
            == SCARD_S_SUCCESS) {
            switch (dwProt) {
            case SCARD_PROTOCOL_T0:
                pioSendPci = SCARD_PCI_T0;
                break;
            case SCARD_PROTOCOL_T1:
                pioSendPci = SCARD_PCI_T1;
                break;
            default:
                // This should not happen during normal use
                rv = SCARD_E_PROTO_MISMATCH;
                break;
            }
        }

        return rv;
    }

    /***
     * @brief Executes a sequence of transmissions, and retries it if the card is reset during transmission
     *
     * A card not opened in exclusive mode (like here) can be reset by another process.
     * The application has to acknowledge the reset and retransmit the transaction.
     *
     * @param handle Smartcard handle
     * @param atomic_action Lambda that contains the sequence to be executed as a transaction. Expected to return
     * SCARD_S_SUCCESS on success.
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t transactRetry(SCARDHANDLE handle, std::function<int32_t()> atomic_action)
    {
        int32_t rv = SCARD_E_UNEXPECTED;

        SCUINT dwProt = SCARD_PROTOCOL_UNDEFINED;
        const SCARD_IO_REQUEST* pioSendPci = nullptr;
        if ((rv = getCardStatus(handle, dwProt, pioSendPci)) == SCARD_S_SUCCESS) {
            // Begin a transaction. This locks out any other process from interfacing with the card
            if ((rv = SCardBeginTransaction(handle)) == SCARD_S_SUCCESS) {
                int i;
                for (i = 4; i > 0; i--) { // 3 tries for reconnecting after reset
                    int32_t rv_act = atomic_action();
                    if (rv_act == SCARD_S_SUCCESS) {
                        rv = SCARD_S_SUCCESS;
                        break;
                    } else if (rv_act == static_cast<int32_t>(SCARD_W_RESET_CARD)) {
                        // The card was reset during the transmission.
                        SCUINT dwProt_new = SCARD_PROTOCOL_UNDEFINED;
                        // Acknowledge the reset and reestablish the connection and handle
                        rv = SCardReconnect(handle, SCARD_SHARE_SHARED, dwProt, SCARD_LEAVE_CARD, &dwProt_new);
// On Windows, the transaction has to be re-started.
// On Linux and OSX (which use pcsc-lite), the transaction continues to be valid.
#ifdef _WIN32
                        if (rv == SCARD_S_SUCCESS) {
                            rv = SCardBeginTransaction(handle);
                        }
#endif
                        qDebug("Smardcard was reset and had to be reconnected");
                    } else {
                        rv = rv_act;
                        break;
                    }
                }
                if (i == 0) {
                    rv = SCARD_W_RESET_CARD;
                    qDebug("Smardcard was reset and failed to reconnect after 3 tries");
                }
            }
        }

        // This could return SCARD_W_RESET_CARD or SCARD_E_NOT_TRANSACTED, but we dont care
        // because then the transaction would have already been ended implicitly
        SCardEndTransaction(handle, SCARD_LEAVE_CARD);

        return rv;
    }

    /***
     * @brief Transmits a buffer to the smartcard, and reads the response
     *
     * @param handle Smartcard handle
     * @param pbSendBuffer Pointer to the data to be sent
     * @param dwSendLength Size of the data to be sent in bytes
     * @param pbRecvBuffer Pointer to the data to be received
     * @param dwRecvLength Size of the data to be received in bytes
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t transmit(SCARDHANDLE handle,
                     const uint8_t* pbSendBuffer,
                     SCUINT dwSendLength,
                     uint8_t* pbRecvBuffer,
                     SCUINT& dwRecvLength)
    {
        int32_t rv = SCARD_E_UNEXPECTED;

        SCUINT dwProt = SCARD_PROTOCOL_UNDEFINED;
        const SCARD_IO_REQUEST* pioSendPci = nullptr;
        if ((rv = getCardStatus(handle, dwProt, pioSendPci)) == SCARD_S_SUCCESS) {
            // Write to and read from the card
            // pioRecvPci is NULL because we do not expect any PCI response header
            if ((rv = SCardTransmit(handle, pioSendPci, pbSendBuffer, dwSendLength, NULL, pbRecvBuffer, &dwRecvLength))
                == SCARD_S_SUCCESS) {
                if (dwRecvLength < 2) {
                    // Any valid response should be at least 2 bytes (response status)
                    // However the protocol itself could fail
                    rv = SCARD_E_FILE_NOT_FOUND;
                } else {
                    if (pbRecvBuffer[dwRecvLength - 2] == SW_OK_HIGH || pbRecvBuffer[dwRecvLength - 1] == SW_OK_LOW) {
                        rv = SCARD_S_SUCCESS;
                    } else if (pbRecvBuffer[dwRecvLength - 2] == SW_PRECOND_HIGH
                               || pbRecvBuffer[dwRecvLength - 1] == SW_PRECOND_LOW) {
                        // This happens if the key requires eg. a button press or if the applet times out
                        // Solution: Re-present the card to the reader
                        rv = SCARD_W_CARD_NOT_AUTHENTICATED;
                    } else {
                        rv = SCARD_E_UNEXPECTED;
                    }
                }
            }
        }

        return rv;
    }

    /***
     * @brief Transmits an applet selection APDU to select the challenge-response applet
     *
     * @param handle Smartcard handle and applet ID bytestring pair
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t selectApplet(const SCardAID& handle)
    {
        uint8_t pbSendBuffer_head[5] = {
            CLA_ISO, INS_SELECT, SEL_APP_AID, 0, static_cast<uint8_t>(handle.second.size())};
        uint8_t* pbSendBuffer = new uint8_t[5 + handle.second.size()];
        memcpy(pbSendBuffer, pbSendBuffer_head, 5);
        memcpy(pbSendBuffer + 5, handle.second.constData(), handle.second.size());
        uint8_t pbRecvBuffer[12] = {
            0}; // 3 bytes version, 1 byte program counter, other stuff for various implementations, 2 bytes status
        SCUINT dwRecvLength = 12;

        int32_t rv = transmit(handle.first, pbSendBuffer, 5 + handle.second.size(), pbRecvBuffer, dwRecvLength);

        delete[] pbSendBuffer;

        return rv;
    }

    /***
     * @brief Reads the serial number of a key
     *
     * @param handle Smartcard handle and applet ID bytestring pair
     * @param serial The serial number
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t getSerial(const SCardAID& handle, unsigned int& serial)
    {
        // Ensure the transmission is retransmitted after card resets
        return transactRetry(handle.first, [&handle, &serial]() {
            int32_t rv_l = SCARD_E_UNEXPECTED;

            // Ensure that the card is always selected before sending the command
            if ((rv_l = selectApplet(handle)) != SCARD_S_SUCCESS) {
                return rv_l;
            }

            uint8_t pbSendBuffer[5] = {CLA_ISO, INS_API_REQ, CMD_GET_SERIAL, 0, 6};
            uint8_t pbRecvBuffer[6] = {0}; // 4 bytes serial, 2 bytes status
            SCUINT dwRecvLength = 6;

            rv_l = transmit(handle.first, pbSendBuffer, 5, pbRecvBuffer, dwRecvLength);
            if (rv_l == SCARD_S_SUCCESS && dwRecvLength >= 4) {
                // The serial number is encoded MSB first
                serial = (pbRecvBuffer[0] << 24) + (pbRecvBuffer[1] << 16) + (pbRecvBuffer[2] << 8) + (pbRecvBuffer[3]);
            }

            return rv_l;
        });
    }

    /***
     * @brief Creates a smartcard handle and applet select bytestring pair by looking up a serial key
     *
     * @param target_serial The serial number to search for
     * @param context A pre-established smartcard API context
     * @param atr_names A map which maps the card ATR to an applet select APDU bytestring and display name pair
     * @param handle The created smartcard handle and applet select bytestring pair
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t openKeySerial(const unsigned int target_serial,
                          SCARDCONTEXT context,
                          const QHash<QByteArray, QPair<QByteArray, QString>>& atr_names,
                          SCardAID* handle)
    {
        int32_t rv = SCARD_S_SUCCESS;
        QList<QString> readers_list = getReaders(context);

        // Iterate all connected readers
        foreach (const QString& reader_name, readers_list) {
            SCARDHANDLE hCard;
            SCUINT dwActiveProtocol = SCARD_PROTOCOL_UNDEFINED;
            rv = SCardConnect(context,
                              reader_name.toStdString().c_str(),
                              SCARD_SHARE_SHARED,
                              SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                              &hCard,
                              &dwActiveProtocol);

            if (rv == SCARD_S_SUCCESS) {
                // Read the ATR record of the card
                uint8_t pbAtr[MAX_ATR_SIZE] = {0};
                char pbReader[MAX_READERNAME] = {0};
                SCUINT dwAtrLen = sizeof(pbAtr);
                SCUINT dwReaderLen = sizeof(pbReader);
                SCUINT dwState = 0, dwProt = SCARD_PROTOCOL_UNDEFINED;
                rv = SCardStatus(hCard, pbReader, &dwReaderLen, &dwState, &dwProt, pbAtr, &dwAtrLen);
                if (rv == SCARD_S_SUCCESS) {
                    if (dwProt == SCARD_PROTOCOL_T0 || dwProt == SCARD_PROTOCOL_T1) {
                        // Lookup the ATR record in the applet selection map
                        QByteArray atr = QByteArray(reinterpret_cast<char*>(pbAtr), dwAtrLen);
                        if (atr_names.keys().contains(atr)) {
                            unsigned int serial = 0;
                            SCardAID satr = {hCard, atr_names.value(atr).first};
                            // Check the serial number of the card by using an ADPU transfer
                            getSerial(satr, serial);
                            if (serial == target_serial) {
                                handle->first = satr.first;
                                handle->second = satr.second;
                                return SCARD_S_SUCCESS;
                            }
                        }
                    } else {
                        rv = SCARD_E_PROTO_MISMATCH;
                    }
                }

                rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
            }
        }

        if (rv != SCARD_S_SUCCESS) {
            return rv;
        }

        return SCARD_E_NO_SMARTCARD;
    }

    /***
     * @brief Reads the status of a key
     *
     * The status is used for the firmware version only atm.
     *
     * @param handle Smartcard handle and applet ID bytestring pair
     * @param version The firmware version in [major, minor, patch] format
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t getStatus(const SCardAID& handle, uint8_t version[3])
    {
        // Ensure the transmission is retransmitted after card resets
        return transactRetry(handle.first, [&handle, &version]() {
            int32_t rv_l = SCARD_E_UNEXPECTED;

            // Ensure that the card is always selected before sending the command
            if ((rv_l = selectApplet(handle)) != SCARD_S_SUCCESS) {
                return rv_l;
            }

            uint8_t pbSendBuffer[5] = {CLA_ISO, INS_STATUS, 0, 0, 6};
            uint8_t pbRecvBuffer[8] = {0}; // 4 bytes serial, 2 bytes other stuff, 2 bytes status
            SCUINT dwRecvLength = 8;

            rv_l = transmit(handle.first, pbSendBuffer, 5, pbRecvBuffer, dwRecvLength);
            if (rv_l == SCARD_S_SUCCESS && dwRecvLength >= 3) {
                memcpy(version, pbRecvBuffer, 3);
            }

            return rv_l;
        });
    }

    /***
     * @brief Performs a challenge-response transmission
     *
     * The card computes the SHA1-HMAC  of the challenge
     * using its pre-programmed secret key and returns the response
     *
     * @param handle Smartcard handle and applet ID bytestring pair
     * @param slot_cmd Either CMD_HMAC_1 for slot 1 or CMD_HMAC_2 for slot 2
     * @param input Challenge byte buffer, exactly 64 bytes and padded using PKCS#7 or Yubikey padding
     * @param output Response byte buffer, exactly 20 bytes
     *
     * @returns SCARD_S_SUCCESS on success
     */
    int32_t getHMAC(const SCardAID& handle, uint8_t slot_cmd, const uint8_t input[64], uint8_t output[20])
    {
        // Ensure the transmission is retransmitted after card resets
        return transactRetry(handle.first, [&handle, &slot_cmd, &input, &output]() {
            int32_t rv_l = SCARD_E_UNEXPECTED;

            // Ensure that the card is always selected before sending the command
            if ((rv_l = selectApplet(handle)) != SCARD_S_SUCCESS) {
                return rv_l;
            }

            uint8_t pbSendBuffer[5 + 64] = {CLA_ISO, INS_API_REQ, slot_cmd, 0, 64};
            memcpy(pbSendBuffer + 5, input, 64);
            uint8_t pbRecvBuffer[22] = {0}; // 20 bytes hmac, 2 bytes status
            SCUINT dwRecvLength = 22;

            rv_l = transmit(handle.first, pbSendBuffer, 5 + 64, pbRecvBuffer, dwRecvLength);
            if (rv_l == SCARD_S_SUCCESS && dwRecvLength >= 20) {
                memcpy(output, pbRecvBuffer, 20);
            }

            // If transmission is successful but no data is returned
            // then the slot is probably not configured for HMAC-SHA1
            // but for OTP or nothing instead
            if (rv_l == SCARD_S_SUCCESS && dwRecvLength != 22) {
                return static_cast<int32_t>(SCARD_E_FILE_NOT_FOUND);
            }

            return rv_l;
        });
    }

} // namespace

YubiKeyInterfacePCSC::YubiKeyInterfacePCSC()
    : YubiKeyInterface()
{
    if (SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &m_sc_context) != SCARD_S_SUCCESS) {
        qDebug("YubiKey: Failed to establish PCSC context.");
    } else {
        m_initialized = true;
    }
}

YubiKeyInterfacePCSC::~YubiKeyInterfacePCSC()
{
    if (m_initialized && SCardReleaseContext(m_sc_context) != SCARD_S_SUCCESS) {
        qDebug("YubiKey: Failed to release PCSC context.");
    }
}

YubiKeyInterfacePCSC* YubiKeyInterfacePCSC::m_instance(nullptr);

YubiKeyInterfacePCSC* YubiKeyInterfacePCSC::instance()
{
    if (!m_instance) {
        m_instance = new YubiKeyInterfacePCSC();
    }

    return m_instance;
}

void YubiKeyInterfacePCSC::findValidKeys()
{
    m_error.clear();
    if (!isInitialized()) {
        return;
    }

    QtConcurrent::run([this] {
        // This mutex protects the smartcard against concurrent transmissions
        if (!m_mutex.tryLock(1000)) {
            emit detectComplete(false);
            return;
        }

        // Remove all known keys
        m_foundKeys.clear();

        // Connect to each reader and look for cards
        QList<QString> readers_list = getReaders(m_sc_context);
        foreach (const QString& reader_name, readers_list) {
            SCARDHANDLE hCard;
            SCUINT dwActiveProtocol = SCARD_PROTOCOL_UNDEFINED;
            int32_t rv = SCardConnect(m_sc_context,
                                      reader_name.toStdString().c_str(),
                                      SCARD_SHARE_SHARED,
                                      SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                                      &hCard,
                                      &dwActiveProtocol);

            if (rv == SCARD_S_SUCCESS) {
                // Read the potocol and the ATR record
                uint8_t pbAtr[MAX_ATR_SIZE] = {0};
                char pbReader[MAX_READERNAME] = {0};
                SCUINT dwAtrLen = sizeof(pbAtr);
                SCUINT dwReaderLen = sizeof(pbReader);
                SCUINT dwState = 0, dwProt = SCARD_PROTOCOL_UNDEFINED;
                rv = SCardStatus(hCard, pbReader, &dwReaderLen, &dwState, &dwProt, pbAtr, &dwAtrLen);
                if (rv == SCARD_S_SUCCESS) {
                    // Check for a valid protocol
                    if (dwProt == SCARD_PROTOCOL_T0 || dwProt == SCARD_PROTOCOL_T1) {
                        QByteArray atr = QByteArray(reinterpret_cast<char*>(pbAtr), dwAtrLen);
                        if (m_atr_names.keys().contains(atr)) {
                            // Build the UI name using the display name found in the ATR map,
                            // The firmware version and the serial number
                            QString name = m_atr_names.value(atr).second;
                            uint8_t version[3] = {0};
                            SCardAID satr = {hCard, m_atr_names.value(atr).first};
                            getStatus(satr, version);
                            name += QString(" v%1.%2.%3")
                                        .arg(QString::number(version[0]),
                                             QString::number(version[1]),
                                             QString::number(version[2]));
                            unsigned int serial = 0;
                            getSerial(satr, serial);
                            // This variable is ignored
                            // because blocking is always assumed, ie. the program waits
                            // for the user to touch the key to the reader
                            bool wouldBlock = false;
                            QList<QPair<int, QString>> ykSlots;
                            for (int slot = 1; slot <= 2; ++slot) {
                                if (performTestChallenge(&satr, slot, &wouldBlock)) {
                                    auto display =
                                        tr("(PCSC) %1 [%2] Challenge-Response - Slot %3 - %4")
                                            .arg(name,
                                                 QString::number(serial),
                                                 QString::number(slot),
                                                 tr("Present",
                                                    "PCSC Challenge-Response Key presentation to reader required"));
                                    ykSlots.append({slot, display});
                                }
                            }

                            if (!ykSlots.isEmpty()) {
                                m_foundKeys.insert(serial, ykSlots);
                            }
                        }
                    }
                }

                rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
            }
        }

        m_mutex.unlock();
        emit detectComplete(!m_foundKeys.isEmpty());
    });
}

bool YubiKeyInterfacePCSC::testChallenge(YubiKeySlot slot, bool* wouldBlock)
{
    bool ret = false;
    SCardAID hCard;
    int32_t rv = openKeySerial(slot.first, m_sc_context, m_atr_names, &hCard);

    if (rv == SCARD_S_SUCCESS) {
        ret = performTestChallenge(&hCard, slot.second, wouldBlock);
        SCardDisconnect(hCard.first, SCARD_LEAVE_CARD);
    }

    return ret;
}

bool YubiKeyInterfacePCSC::performTestChallenge(void* key, int slot, bool* wouldBlock)
{
    Q_UNUSED(wouldBlock);
    // Array has to be at least one byte or else the yubikey would interpret everything as padding
    auto chall = randomGen()->randomArray(1);
    Botan::secure_vector<char> resp;
    auto ret = performChallenge(static_cast<SCardAID*>(key), slot, false, chall, resp);
    if (ret != YubiKey::ChallengeResult::YCR_ERROR) {
        return true;
    }
    return false;
}

YubiKey::ChallengeResult
YubiKeyInterfacePCSC::challenge(YubiKeySlot slot, const QByteArray& challenge, Botan::secure_vector<char>& response)
{
    m_error.clear();
    if (!m_initialized) {
        m_error = tr("The YubiKey PCSC interface has not been initialized.");
        return YubiKey::ChallengeResult::YCR_ERROR;
    }

    // Try to grab a lock for 1 second, fail out if not possible
    if (!m_mutex.tryLock(1000)) {
        m_error = tr("Hardware key is currently in use.");
        return YubiKey::ChallengeResult::YCR_ERROR;
    }

    // Try for a few seconds to find the key
    emit challengeStarted();

    SCardAID hCard;
    int tries = 20; // 5 seconds, test every 250 ms
    while (tries > 0) {
        int32_t rv = openKeySerial(slot.first, m_sc_context, m_atr_names, &hCard);
        // Key with specified serial number is found
        if (rv == SCARD_S_SUCCESS) {
            auto ret = performChallenge(&hCard, slot.second, true, challenge, response);
            SCardDisconnect(hCard.first, SCARD_LEAVE_CARD);

            if (ret != YubiKey::ChallengeResult::YCR_ERROR) {
                emit challengeCompleted();
                m_mutex.unlock();
                return ret;
            }
        }

        tries--;
        if (tries > 0) {
            QThread::msleep(250);
        }
    }

    m_error = tr("Could not find or access hardware key with serial number %1. Please present it to continue. ")
                  .arg(slot.first)
              + m_error;
    emit challengeCompleted();
    m_mutex.unlock();
    return YubiKey::ChallengeResult::YCR_ERROR;
}

YubiKey::ChallengeResult YubiKeyInterfacePCSC::performChallenge(void* key,
                                                                int slot,
                                                                bool mayBlock,
                                                                const QByteArray& challenge,
                                                                Botan::secure_vector<char>& response)
{
    // Always block (i.e. wait for the user to touch the key to the reader)
    Q_UNUSED(mayBlock);

    m_error.clear();
    int yk_cmd = (slot == 1) ? CMD_HMAC_1 : CMD_HMAC_2;
    QByteArray paddedChallenge = challenge;

    response.clear();
    response.resize(20);

    /*
     * The challenge sent to the Yubikey should always be 64 bytes for
     * compatibility with all configurations.  Follow PKCS7 padding.
     *
     * There is some question whether or not 64 bytes fixed length
     * configurations even work, some docs say avoid it.
     *
     * In fact, the Yubikey always assumes the last byte (nr. 64)
     * and all bytes of the same value preceeding it to be padding.
     * This does not conform fully to PKCS7, because the the actual value
     * of the padding bytes is ignored.
     */
    const int padLen = 64 - paddedChallenge.size();
    if (padLen > 0) {
        paddedChallenge.append(QByteArray(padLen, padLen));
    }

    const unsigned char* c;
    unsigned char* r;
    c = reinterpret_cast<const unsigned char*>(paddedChallenge.constData());
    r = reinterpret_cast<unsigned char*>(response.data());

    int32_t rv = getHMAC(*static_cast<SCardAID*>(key), yk_cmd, c, r);

    if (rv != SCARD_S_SUCCESS) {
        if (rv == static_cast<int32_t>(SCARD_W_CARD_NOT_AUTHENTICATED)) {
            m_error = tr("Hardware key is locked or timed out. Unlock or re-present it to continue.");
        } else if (rv == static_cast<int32_t>(SCARD_E_FILE_NOT_FOUND)) {
            m_error = tr("Hardware key was not found or is misconfigured.");
        } else {
            m_error =
                tr("Failed to complete a challenge-response, the PCSC error code was: %1").arg(QString::number(rv));
        }

        return YubiKey::ChallengeResult::YCR_ERROR;
    }

    return YubiKey::ChallengeResult::YCR_SUCCESS;
}

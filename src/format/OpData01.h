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

#ifndef KEEPASSXC_OPDATA01_H
#define KEEPASSXC_OPDATA01_H

#include <QObject>

/*!
 * Packages and transports the AgileBits data structure called \c OpData01
 * used to encypt and provide HMAC for encrypted data.
 * \sa https://support.1password.com/opvault-design/#opdata01
 */
class OpData01 : public QObject
{
    Q_OBJECT

public:
    explicit OpData01(QObject* parent = nullptr);
    ~OpData01() override;

    /*!
     * The convenience equivalent of decode01(OpData01,const QByteArray,const QByteArray,const QByteArray) that simply
     * decodes the provided base64 string into its underlying \c QByteArray.
     */
    bool decodeBase64(QString const& b64String, const QByteArray& key, const QByteArray& hmacKey);

    /*!
     * Populates the given \code OpData01 structure by decoding the provided blob of data,
     * using the given key and then verifies using the given HMAC key.
     * \returns true if things went well and \code m_clearText is usable, false and \code m_errorStr will contain
     * details.
     */
    bool decode(const QByteArray& data, const QByteArray& key, const QByteArray& hmacKey);

    QByteArray getClearText();

    QString errorString();

private:
    QByteArray m_clearText;
    QString m_errorStr;
};

#endif // KEEPASSXC_OPDATA01_H

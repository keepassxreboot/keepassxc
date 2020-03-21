/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "HibpDownloader.h"
#include "core/Config.h"
#include "core/Global.h"
#include "core/NetworkManager.h"

#include <QCryptographicHash>
#include <QUrl>
#include <QtNetwork>

namespace
{
    /*
     * Return the SHA1 hash of the specified password in upper-case hex.
     *
     * The result is always exactly 40 characters long.
     */
    QString sha1Hex(const QString& password)
    {
        // Get the binary SHA1
        const auto sha1 = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);

        // Convert to hex
        QString ret;
        ret.reserve(40);
        auto p = sha1.data();
        for (int i = 0; i < sha1.size(); ++i) {
            const auto u = uint8_t(*p++);
            constexpr auto hex = "0123456789ABCDEF";
            ret += hex[(u >> 4) & 0xF];
            ret += hex[u & 0xF];
        }

        return ret;
    }

    /*
     * Search a password's hash in the output of the HIBP web service.
     *
     * Returns the number of times the password is found in breaches, or
     * 0 if the password is not in the HIBP result.
     */
    int pwnCount(const QString& password, const QString& hibpResult)
    {
        // The first 5 characters of the hash are in the URL already,
        // the HIBP result contains the remainder, which is:
        const auto hash = sha1Hex(password);
        const auto remainder = QStringRef(&hash, 5, 35);

        // Search the remainder in the HIBP output
        const auto pos = hibpResult.indexOf(remainder);
        if (pos < 0) {
            // Not found
            return 0;
        }

        // Found: Return the number that follows. We know that the
        // length of remainder is 35 and that a colon follows in
        // the HIBP result, followed by the number. So the number
        // begins here:
        const auto counter = hibpResult.midRef(pos + 35 + 1);

        // And where does the number end?
        auto end = counter.indexOf('\n');
        if (end < 0) {
            end = counter.size();
        }

        // So extract the number. Note that toInt doesn't have
        // a "scan until number ends, ignore whatever follows"
        // mode like atoi.
        return counter.left(end).toInt();
    }
} // namespace

HibpDownloader::HibpDownloader(QObject* parent)
    : QObject(parent)
    , m_reply(nullptr)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, SIGNAL(timeout()), SLOT(abortDownload()));
}

HibpDownloader::~HibpDownloader()
{
    abort();
}

/*
 * Add one password to the list list of passwords to check.
 *
 * Invoke this function once for every password to check,
 * then call validate().
 */
void HibpDownloader::add(const QString& password)
{
    if (!m_pwdsToTry.contains(password)) {
        m_pwdsToTry += password;
    }
}

/*
 * Start validating the passwords against HIBP.
 */
void HibpDownloader::validate()
{
    if (m_pwdsToTry.empty()) {
        return;
    }

    if (!m_timeout.isActive()) {
        int timeout = config()->get("HibpDownloadTimeout", 10).toInt();
        m_timeout.start(timeout * 1000);

        // Use the first password to start the download process.
        fetchPassword(m_pwdsToTry.takeFirst());
    }
}

/*
 * Abort the current online activity (if any).
 */
void HibpDownloader::abort()
{
    if (m_reply) {
        m_reply->abort();
    }
}

/*
 * Fetch the hash list of the specified password from HIBP.
 */
void HibpDownloader::fetchPassword(const QString& password)
{
    m_currentPwd = password;

    // The URL we query is https://api.pwnedpasswords.com/range/XXXXX,
    // where XXXXX is the first five bytes of the hex representation of
    // the password's SHA1.
    const auto url = QString("https://api.pwnedpasswords.com/range/") + sha1Hex(m_currentPwd).left(5);

    // HIBP requires clients to specify a user agent in the request
    // (https://haveibeenpwned.com/API/v3#UserAgent); however, in order
    // to minimize the amount of information we expose about ourselves,
    // we don't add the KeePassXC version number or platform.
    m_bytesReceived.clear();
    auto request = QNetworkRequest(url);
    request.setRawHeader("User-Agent", "KeePassXC");

    // Finally, submit the request to HIBP.
    m_reply = getNetMgr()->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &HibpDownloader::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &HibpDownloader::fetchReadyRead);
}

/*
 * Called when new data has been loaded from the HIBP server.
 */
void HibpDownloader::fetchReadyRead()
{
    m_bytesReceived += m_reply->readAll();
}

/*
 * Called after all data has been loaded from the HIBP server.
 */
void HibpDownloader::fetchFinished()
{
    // Get result status
    const auto ok = m_reply->error() == QNetworkReply::NoError;
    const auto err = m_reply->errorString();

    m_reply->deleteLater();
    m_reply = nullptr;
    m_timeout.stop();

    // If there was an error, assume it's permanent and abort
    // (don't process the rest of the password list).
    if (!ok) {
        auto msg = tr("Online password validation failed") + ":\n" + err;
        if (!m_bytesReceived.isEmpty()) {
            msg += "\n" + m_bytesReceived;
        }
        emit failed(m_currentPwd, msg);
    }

    // Current password validated, send the result to the caller
    emit finished(m_currentPwd, pwnCount(m_currentPwd, m_bytesReceived));

    // Continue with the next password
    if (m_pwdsToTry.empty()) {
        // None left, we're doe
        m_timeout.stop();
    } else {
        // Validate the next password in the list
        fetchPassword(m_pwdsToTry.takeFirst());
    }
}

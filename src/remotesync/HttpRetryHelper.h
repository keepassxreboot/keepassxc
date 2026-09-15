/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#ifndef KEEPASSXC_HTTPRETRYHELPER_H
#define KEEPASSXC_HTTPRETRYHELPER_H

#include <QAtomicInt>
#include <QNetworkReply>
#include <functional>

struct RetryPolicy
{
    int maxRetries = 3;
    int baseDelayMs = 1000; // Exponential backoff base delay
    int maxRetryAfterSec = 60; // Cap Retry-After at 60s, fail if longer
};

class HttpRetryHelper
{
public:
    using RequestFunc = std::function<QNetworkReply*(void)>;

    /**
     * Execute an HTTP request with retry logic.
     *
     * Calls makeRequest() to obtain a QNetworkReply*, waits for completion
     * (with timeout), and retries on 429 or 5xx responses up to policy.maxRetries times.
     *
     * On 429 with Retry-After header: uses that delay (capped at maxRetryAfterSec).
     * If Retry-After exceeds cap, fails immediately.
     *
     * Backoff: base * 2^attempt (so with base=1000ms: 1s, 2s, 4s). Overridden by
     * Retry-After when present and larger.
     *
     * @param makeRequest  Callable that creates and sends a QNetworkReply*
     * @param policy       Retry policy configuration
     * @param timeoutMs    Per-request timeout in milliseconds
     * @param abortFlag    Optional atomic flag; if set to non-zero, aborts retries
     * @return The final QNetworkReply* (caller must call deleteLater())
     */
    static QNetworkReply*
    execute(const RequestFunc& makeRequest, const RetryPolicy& policy, int timeoutMs, QAtomicInt* abortFlag = nullptr);

    /**
     * Check if an HTTP status code is retryable.
     * Returns true for 429 (Too Many Requests) and 5xx (Server Error).
     */
    static bool isRetryable(int httpStatus);

private:
    HttpRetryHelper() = default;
};

#endif // KEEPASSXC_HTTPRETRYHELPER_H

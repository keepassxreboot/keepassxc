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

#include "HttpRetryHelper.h"

#include <QEventLoop>
#include <QTimer>

QNetworkReply* HttpRetryHelper::execute(const RequestFunc& makeRequest,
                                        const RetryPolicy& policy,
                                        int timeoutMs,
                                        QAtomicInt* abortFlag)
{
    QNetworkReply* reply = nullptr;

    for (int attempt = 0; attempt <= policy.maxRetries; ++attempt) {
        // Check abort flag before each attempt
        if (abortFlag && abortFlag->loadAcquire() != 0) {
            // Return whatever reply we have (or nullptr on first attempt)
            return reply;
        }

        // Clean up previous reply if retrying
        if (reply) {
            reply->deleteLater();
            reply = nullptr;
        }

        reply = makeRequest();
        if (!reply) {
            return nullptr;
        }

        // Wait for reply to finish or timeout
        {
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);

            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

            timer.start(timeoutMs);
            if (!reply->isFinished()) {
                loop.exec();
            }

            if (!reply->isFinished()) {
                reply->abort();
                // Return the aborted reply -- caller sees the error
                return reply;
            }
        }

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // If not retryable or this was our last attempt, return as-is
        if (!isRetryable(httpStatus) || attempt == policy.maxRetries) {
            return reply;
        }

        // Determine delay before retry (use qint64 to prevent integer overflow)
        int shift = qMin(attempt, 20);
        qint64 baseDelay = static_cast<qint64>(policy.baseDelayMs) * (1 << shift);
        int delayMs = static_cast<int>(qMin(baseDelay, static_cast<qint64>(INT_MAX)));

        // Check for Retry-After header (seconds)
        if (reply->hasRawHeader("Retry-After")) {
            bool ok = false;
            int retryAfterSec = reply->rawHeader("Retry-After").toInt(&ok);
            if (ok && retryAfterSec > 0) {
                // Fail immediately if Retry-After exceeds our cap
                if (retryAfterSec > policy.maxRetryAfterSec) {
                    return reply;
                }
                int retryAfterMs =
                    static_cast<int>(qMin(static_cast<qint64>(retryAfterSec) * 1000, static_cast<qint64>(INT_MAX)));
                if (retryAfterMs > delayMs) {
                    delayMs = retryAfterMs;
                }
            }
        }

        // Wait for the delay, checking abort flag periodically
        {
            QEventLoop delayLoop;
            QTimer delayTimer;
            delayTimer.setSingleShot(true);
            QObject::connect(&delayTimer, &QTimer::timeout, &delayLoop, &QEventLoop::quit);

            // Poll abort flag every 100ms so abort() is responsive during long delays
            QTimer abortPollTimer;
            if (abortFlag) {
                abortPollTimer.setInterval(100);
                QObject::connect(&abortPollTimer, &QTimer::timeout, [&]() {
                    if (abortFlag->loadAcquire() != 0) {
                        delayLoop.quit();
                    }
                });
                abortPollTimer.start();
            }

            delayTimer.start(delayMs);
            delayLoop.exec();
        }

        // Check abort flag after delay
        if (abortFlag && abortFlag->loadAcquire() != 0) {
            return reply;
        }
    }

    return reply;
}

bool HttpRetryHelper::isRetryable(int httpStatus)
{
    return httpStatus == 429 || (httpStatus >= 500 && httpStatus <= 599);
}

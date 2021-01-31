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

#include <QApplication>
#include <QString>

#include "Database.h"
#include "Entry.h"
#include "Group.h"
#include "PasswordHealth.h"
#include "zxcvbn.h"

// Define the static member variable with the custom field name
const QString PasswordHealth::OPTION_KNOWN_BAD = QStringLiteral("KnownBad");

PasswordHealth::PasswordHealth(double entropy)
    : m_score(entropy)
    , m_entropy(entropy)
{
    switch (quality()) {
    case Quality::Bad:
    case Quality::Poor:
        m_scoreReasons << QApplication::tr("Very weak password");
        m_scoreDetails << QApplication::tr("Password entropy is %1 bits").arg(QString::number(m_entropy, 'f', 2));
        break;

    case Quality::Weak:
        m_scoreReasons << QApplication::tr("Weak password");
        m_scoreDetails << QApplication::tr("Password entropy is %1 bits").arg(QString::number(m_entropy, 'f', 2));
        break;

    default:
        // No reason or details for good and excellent passwords
        break;
    }
}

PasswordHealth::PasswordHealth(QString pwd)
    : PasswordHealth(ZxcvbnMatch(pwd.toLatin1(), nullptr, nullptr))
{
}

void PasswordHealth::setScore(int score)
{
    m_score = score;
}

void PasswordHealth::adjustScore(int amount)
{
    m_score += amount;
}

QString PasswordHealth::scoreReason() const
{
    return m_scoreReasons.join("\n");
}

void PasswordHealth::addScoreReason(QString reason)
{
    m_scoreReasons << reason;
}

QString PasswordHealth::scoreDetails() const
{
    return m_scoreDetails.join("\n");
}

void PasswordHealth::addScoreDetails(QString details)
{
    m_scoreDetails.append(details);
}

PasswordHealth::Quality PasswordHealth::quality() const
{
    if (m_score <= 0) {
        return Quality::Bad;
    } else if (m_score < 40) {
        return Quality::Poor;
    } else if (m_score < 65) {
        return Quality::Weak;
    } else if (m_score < 100) {
        return Quality::Good;
    }
    return Quality::Excellent;
}

/**
 * This class provides additional information about password health
 * than can be derived from the password itself (re-use, expiry).
 */
HealthChecker::HealthChecker(QSharedPointer<Database> db)
{
    // Build the cache of re-used passwords
    for (const auto* entry : db->rootGroup()->entriesRecursive()) {
        if (!entry->isRecycled() && !entry->isAttributeReference("Password")) {
            m_reuse[entry->password()]
                << QApplication::tr("Used in %1/%2").arg(entry->group()->hierarchy().join('/'), entry->title());
        }
    }
}

/**
 * Call operator of the Health Checker class.
 *
 * Returns the health of the password in `entry`, considering
 * password entropy, re-use, expiration, etc.
 */
QSharedPointer<PasswordHealth> HealthChecker::evaluate(const Entry* entry) const
{
    // Pointer sanity check
    if (!entry) {
        return {};
    }

    // First analyse the password itself
    const auto pwd = entry->password();
    auto health = QSharedPointer<PasswordHealth>(new PasswordHealth(pwd));

    // Second, if the password is in the database more than once,
    // reduce the score accordingly
    const auto& used = m_reuse[pwd];
    const auto count = used.size();
    if (count > 1) {
        constexpr auto penalty = 15;
        health->adjustScore(-penalty * (count - 1));
        health->addScoreReason(QApplication::tr("Password is used %1 times").arg(QString::number(count)));
        // Add the first 20 uses of the password to prevent the details display from growing too large
        for (int i = 0; i < used.size(); ++i) {
            health->addScoreDetails(used[i]);
            if (i == 19) {
                health->addScoreDetails("…");
                break;
            }
        }

        // Don't allow re-used passwords to be considered "good"
        // no matter how great their entropy is.
        if (health->score() > 64) {
            health->setScore(64);
        }
    }

    // Third, if the password has already expired, reduce score to 0;
    // or, if the password is going to expire in the next 30 days,
    // reduce score by 2 points per day.
    if (entry->isExpired()) {
        health->setScore(0);
        health->addScoreReason(QApplication::tr("Password has expired"));
        health->addScoreDetails(QApplication::tr("Password expiry was %1")
                                    .arg(entry->timeInfo().expiryTime().toString(Qt::DefaultLocaleShortDate)));
    } else if (entry->timeInfo().expires()) {
        const auto days = QDateTime::currentDateTime().daysTo(entry->timeInfo().expiryTime());
        if (days <= 30) {
            // First bring the score down into the "weak" range
            // so that the entry appears in Health Check. Then
            // reduce the score by 2 points for every day that
            // we get closer to expiry. days<=0 has already
            // been handled above ("isExpired()").
            if (health->score() > 60) {
                health->setScore(60);
            }
            // clang-format off
            health->adjustScore((30 - days) * -2);
            health->addScoreReason(days <= 2 ? QApplication::tr("Password is about to expire")
                                             : days <= 10 ? QApplication::tr("Password expires in %1 days").arg(days)
                                                          : QApplication::tr("Password will expire soon"));
            health->addScoreDetails(QApplication::tr("Password expires on %1")
                                        .arg(entry->timeInfo().expiryTime().toString(Qt::DefaultLocaleShortDate)));
            //clang-format on
        }
    }

    // Return the result
    return health;
}

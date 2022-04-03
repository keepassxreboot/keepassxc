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

#ifndef KEEPASSX_PASSWORDHEALTH_H
#define KEEPASSX_PASSWORDHEALTH_H

#include <QHash>
#include <QSharedPointer>

class Database;
class Entry;

/**
 * Health status of a single password.
 *
 * @see HealthChecker
 */
class PasswordHealth
{
public:
    explicit PasswordHealth(double entropy);
    explicit PasswordHealth(const QString& pwd);

    void init(double entropy);

    /*
     * The password score is defined to be the greater the better
     * (more secure) the password is. It doesn't have a dimension,
     * there are no defined maximum or minimum values, and score
     * values may change with different versions of the software.
     */
    int score() const
    {
        return m_score;
    }

    void setScore(int score);
    void adjustScore(int amount);

    /*
     * A text description for the password's quality assessment
     * (translated into the application language), and additional
     * information. Empty if nothing is wrong with the password.
     * May contain more than line, separated by '\n'.
     */
    QString scoreReason() const;
    void addScoreReason(QString reason);

    QString scoreDetails() const;
    void addScoreDetails(QString details);

    /*
     * The password quality assessment (based on the score).
     */
    enum class Quality
    {
        Bad,
        Poor,
        Weak,
        Good,
        Excellent
    };
    Quality quality() const;

    /*
     * The password's raw entropy value, in bits.
     */
    double entropy() const
    {
        return m_entropy;
    }

    struct Length
    {
        static const int Short = 8;
        static const int Long = 25;
    };

private:
    int m_score = 0;
    double m_entropy = 0.0;
    QStringList m_scoreReasons;
    QStringList m_scoreDetails;
};

/**
 * Password health check for all entries of a database.
 *
 * @see PasswordHealth
 */
class HealthChecker
{
public:
    explicit HealthChecker(QSharedPointer<Database>);

    // Get the health status of an entry in the database
    QSharedPointer<PasswordHealth> evaluate(const Entry* entry) const;

private:
    // To determine password re-use: first = password, second = entries that use it
    QHash<QString, QStringList> m_reuse;
};

#endif // KEEPASSX_PASSWORDHEALTH_H

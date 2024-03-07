/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BROWSERPASSKEYSCLIENT_H
#define BROWSERPASSKEYSCLIENT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>

class BrowserPasskeysClient : public QObject
{
    Q_OBJECT

public:
    explicit BrowserPasskeysClient() = default;
    ~BrowserPasskeysClient() = default;
    static BrowserPasskeysClient* instance();

    int
    getCredentialCreationOptions(const QJsonObject& publicKeyOptions, const QString& origin, QJsonObject* result) const;
    int getAssertionOptions(const QJsonObject& publicKeyOptions, const QString& origin, QJsonObject* result) const;

private:
    Q_DISABLE_COPY(BrowserPasskeysClient);

    friend class TestPasskeys;
};

static inline BrowserPasskeysClient* browserPasskeysClient()
{
    return BrowserPasskeysClient::instance();
}

#endif // BROWSERPASSKEYSCLIENT_H

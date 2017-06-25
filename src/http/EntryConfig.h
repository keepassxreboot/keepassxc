/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef ENTRYCONFIG_H
#define ENTRYCONFIG_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSet>

class Entry;

class EntryConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList Allow READ allowedHosts WRITE setAllowedHosts)
    Q_PROPERTY(QStringList Deny  READ deniedHosts  WRITE setDeniedHosts )
    Q_PROPERTY(QString     Realm READ realm        WRITE setRealm       )

public:
    EntryConfig(QObject * object = 0);

    bool load(const Entry * entry);
    void save(Entry * entry);
    bool isAllowed(const QString & host);
    void allow(const QString & host);
    bool isDenied(const QString & host);
    void deny(const QString & host);
    QString realm() const;
    void setRealm(const QString &realm);

private:
    QStringList allowedHosts() const;
    void setAllowedHosts(const QStringList &allowedHosts);
    QStringList deniedHosts() const;
    void setDeniedHosts(const QStringList &deniedHosts);

    QSet<QString> m_allowedHosts;
    QSet<QString> m_deniedHosts;
    QString       m_realm;
};

#endif // ENTRYCONFIG_H

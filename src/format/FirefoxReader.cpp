/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "FirefoxReader.h"

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

namespace
{
    QStringList parseCsvLine(const QString& line)
    {
        QStringList fields;
        QString field;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i) {
            QChar c = line[i];
            if (inQuotes) {
                if (c == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i;
                    } else {
                        inQuotes = false;
                    }
                } else {
                    field += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    fields.append(field);
                    field.clear();
                } else {
                    field += c;
                }
            }
        }
        fields.append(field);
        return fields;
    }

    QString extractHost(const QString& url)
    {
        QUrl parsed(url);
        if (parsed.isValid() && !parsed.host().isEmpty()) {
            return parsed.host();
        }
        return url;
    }

    Group* findOrCreateGroup(Group* root, const QString& host)
    {
        if (host.isEmpty()) {
            return root;
        }
        auto existing = root->findGroupByPath(host);
        if (existing) {
            return existing;
        }
        auto group = new Group();
        group->setName(host);
        group->setUuid(QUuid::createUuid());
        group->setParent(root);
        return group;
    }
} // namespace

bool FirefoxReader::hasError()
{
    return !m_error.isEmpty();
}

QString FirefoxReader::errorString()
{
    return m_error;
}

QSharedPointer<Database> FirefoxReader::convert(const QString& path)
{
    m_error.clear();

    QFileInfo fileinfo(path);
    if (!fileinfo.exists()) {
        m_error = QObject::tr("File does not exist.");
        return {};
    }

    QFile file(fileinfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        m_error = QObject::tr("Cannot open file: %1").arg(file.errorString());
        return {};
    }

    QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
    file.close();

    if (lines.size() < 2) {
        m_error = QObject::tr("File is empty or has no data rows.");
        return {};
    }

    QStringList headers = parseCsvLine(lines[0]);
    int urlIdx = headers.indexOf("url");
    int usernameIdx = headers.indexOf("username");
    int passwordIdx = headers.indexOf("password");
    int realmIdx = headers.indexOf("httpRealm");
    int timeCreatedIdx = headers.indexOf("timeCreated");

    if (urlIdx < 0 || usernameIdx < 0 || passwordIdx < 0) {
        m_error = QObject::tr("Invalid Firefox CSV format: missing required columns (url, username, password).");
        return {};
    }

    auto db = QSharedPointer<Database>::create();
    db->rootGroup()->setName(QObject::tr("Firefox Import"));
    db->metadata()->setName(QObject::tr("Firefox Import"));

    int imported = 0;
    for (int i = 1; i < lines.size(); ++i) {
        QStringList fields = parseCsvLine(lines[i]);
        if (fields.size() <= qMax(urlIdx, qMax(usernameIdx, passwordIdx))) {
            continue;
        }

        QString url = fields[urlIdx].trimmed();
        QString username = fields[usernameIdx].trimmed();
        QString password = fields[passwordIdx].trimmed();

        if (url.isEmpty() || password.isEmpty()) {
            continue;
        }

        auto entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setUrl(url);
        entry->setUsername(username);
        entry->setPassword(password);

        QString host = extractHost(url);
        QString title;
        if (!username.isEmpty()) {
            title = QString("%1 - %2").arg(host, username);
        } else {
            title = host;
        }
        entry->setTitle(title);

        if (realmIdx >= 0 && realmIdx < fields.size()) {
            QString realm = fields[realmIdx].trimmed();
            if (!realm.isEmpty()) {
                entry->attributes()->set("httpRealm", realm, false);
            }
        }

        if (timeCreatedIdx >= 0 && timeCreatedIdx < fields.size()) {
            qint64 ts = fields[timeCreatedIdx].trimmed().toLongLong();
            if (ts > 0) {
                    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts, Qt::UTC);
                    if (dt.isValid()) {
                        auto ti = entry->timeInfo();
                        ti.setCreationTime(dt);
                        entry->setTimeInfo(ti);
                    }
                }
        }

        Group* group = findOrCreateGroup(db->rootGroup(), host);
        entry->setGroup(group, false);

        ++imported;
    }

    if (imported == 0) {
        m_error = QObject::tr("No valid entries found in the file.");
        return {};
    }

    return db;
}

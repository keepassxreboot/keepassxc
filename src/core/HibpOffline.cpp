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

#include "HibpOffline.h"

#include <QCryptographicHash>
#include <QMultiHash>
#include <QProcess>

#include "core/Database.h"
#include "core/Group.h"

namespace HibpOffline
{
    const std::size_t SHA1_BYTES = 20;

    enum class ParseResult
    {
        Ok,
        Eof,
        Error
    };

    ParseResult parseHibpLine(QIODevice& input, QByteArray& sha1, int& count)
    {
        QByteArray hexSha1(SHA1_BYTES * 2, '\0');
        const qint64 rc = input.read(hexSha1.data(), hexSha1.size());
        if (rc == 0) {
            return ParseResult::Eof;
        } else if (rc != hexSha1.size()) {
            return ParseResult::Error;
        }

        sha1 = QByteArray::fromHex(hexSha1);

        char c;
        if (!input.getChar(&c) || c != ':') {
            return ParseResult::Error;
        }

        count = 0;
        while (true) {
            if (!input.getChar(&c)) {
                return ParseResult::Error;
            }

            if (c == '\n' || c == '\r') {
                break;
            }

            if (!('0' <= c && c <= '9')) {
                return ParseResult::Error;
            }

            count *= 10;
            count += (c - '0');
        }

        while (1 == input.peek(&c, 1) && (c == '\n' || c == '\r')) {
            input.getChar(&c);
        }

        return ParseResult::Ok;
    }

    bool
    report(QSharedPointer<Database> db, QIODevice& hibpInput, QList<QPair<const Entry*, int>>& findings, QString* error)
    {
        QMultiHash<QByteArray, const Entry*> entriesBySha1;
        for (const auto* entry : db->rootGroup()->entriesRecursive()) {
            if (!entry->isRecycled()) {
                const auto sha1 = QCryptographicHash::hash(entry->password().toUtf8(), QCryptographicHash::Sha1);
                entriesBySha1.insert(sha1, entry);
            }
        }

        QByteArray sha1;
        for (quint64 lineNum = 1;; ++lineNum) {
            int count = 0;

            switch (parseHibpLine(hibpInput, sha1, count)) {
            case ParseResult::Eof:
                return true;
            case ParseResult::Error:
                *error = QObject::tr("HIBP file, line %1: parse error").arg(lineNum);
                return false;
            default:
                break;
            }

            for (const auto* entry : entriesBySha1.values(sha1)) {
                findings.append({entry, count});
            }
        }
    }

    bool okonReport(QSharedPointer<Database> db,
                    const QString& okon,
                    const QString& okonDatabase,
                    QList<QPair<const Entry*, int>>& findings,
                    QString* error)
    {
        if (!okonDatabase.endsWith(".okon")) {
            *error = QObject::tr("To use okon you must provide a post-processed file (e.g. file.okon)");
            return false;
        }

        QProcess okonProcess;

        for (const auto* entry : db->rootGroup()->entriesRecursive()) {
            if (!entry->isRecycled()) {
                const auto sha1 = QCryptographicHash::hash(entry->password().toUtf8(), QCryptographicHash::Sha1);
                okonProcess.start(okon, {"--path", okonDatabase, "--hash", QString::fromLatin1(sha1.toHex())});
                if (!okonProcess.waitForStarted()) {
                    *error = QObject::tr("Could not start okon process: %1").arg(okon);
                    return false;
                }

                if (!okonProcess.waitForFinished()) {
                    *error = QObject::tr("Error: okon process did not finish");
                    return false;
                }

                switch (okonProcess.exitCode()) {
                case 1:
                    findings.append({entry, -1});
                    break;
                case 2:
                    *error = QObject::tr("Failed to load okon processed database: %1").arg(okonDatabase);
                    return false;
                }
            }
        }

        return true;
    }
} // namespace HibpOffline

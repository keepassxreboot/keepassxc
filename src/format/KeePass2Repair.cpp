/*
 *  Copyright (C) 2016 Felix Geyer <debfx@fobos.de>
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

#include "KeePass2Repair.h"

#include <QBuffer>

#include "core/Group.h"
#include "format/KeePass2.h"
#include "format/KeePass2RandomStream.h"
#include "format/KeePass2Reader.h"
#include "format/Kdbx4Reader.h"
#include "format/KdbxXmlReader.h"

KeePass2Repair::RepairOutcome KeePass2Repair::repairDatabase(QIODevice* device, const CompositeKey& key)
{
    m_errorStr.clear();

    KeePass2Reader reader;
    reader.setSaveXml(true);

    QScopedPointer<Database> db(reader.readDatabase(device, key, true));
    if (!reader.hasError()) {
        return qMakePair(NothingTodo, nullptr);
    }

    QByteArray xmlData = reader.reader()->xmlData();
    if (!db || xmlData.isEmpty()) {
        m_errorStr = reader.errorString();
        return qMakePair(UnableToOpen, nullptr);
    }

    bool repairAction = false;

    QString xmlStart = QString::fromLatin1(xmlData.constData(), qMin(100, xmlData.size()));
    QRegExp encodingRegExp("encoding=\"([^\"]+)\"", Qt::CaseInsensitive, QRegExp::RegExp2);
    if (encodingRegExp.indexIn(xmlStart) != -1) {
        if (encodingRegExp.cap(1).compare("utf-8", Qt::CaseInsensitive) != 0
                && encodingRegExp.cap(1).compare("utf8", Qt::CaseInsensitive) != 0)
        {
            // database is not utf-8 encoded, we don't support repairing that
            return qMakePair(RepairFailed, nullptr);
        }
    }

    // try to fix broken databases because of bug #392
    for (int i = (xmlData.size() - 1); i >= 0; i--) {
        auto ch = static_cast<quint8>(xmlData.at(i));
        if (ch < 0x20 && ch != 0x09 && ch != 0x0A && ch != 0x0D) {
            xmlData.remove(i, 1);
            repairAction = true;
        }
    }

    if (!repairAction) {
        // we were unable to find the problem
        return qMakePair(RepairFailed, nullptr);
    }

    KeePass2RandomStream randomStream(reader.reader()->protectedStreamAlgo());
    randomStream.init(reader.reader()->streamKey());
    bool hasError;

    QBuffer buffer(&xmlData);
    buffer.open(QIODevice::ReadOnly);
    if ((reader.version() & KeePass2::FILE_VERSION_CRITICAL_MASK) < KeePass2::FILE_VERSION_4) {
        KdbxXmlReader xmlReader(KeePass2::FILE_VERSION_3_1);
        xmlReader.readDatabase(&buffer, db.data(), &randomStream);
        hasError = xmlReader.hasError();
    } else {
        auto reader4 = reader.reader().staticCast<Kdbx4Reader>();
        QHash<QString, QByteArray> pool = reader4->binaryPool();
        KdbxXmlReader xmlReader(KeePass2::FILE_VERSION_4, pool);
        xmlReader.readDatabase(&buffer, db.data(), &randomStream);
        hasError = xmlReader.hasError();
    }

    if (hasError) {
        return qMakePair(RepairFailed, nullptr);
    }
    else {
        return qMakePair(RepairSuccess, db.take());
    }
}

QString KeePass2Repair::errorString() const
{
    return m_errorStr;
}

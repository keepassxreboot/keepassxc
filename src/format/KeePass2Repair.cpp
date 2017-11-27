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
#include <QScopedPointer>
#include <QRegExp>

#include "format/KeePass2RandomStream.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2XmlReader.h"

KeePass2Repair::RepairOutcome KeePass2Repair::repairDatabase(QIODevice* device, const CompositeKey& key)
{
    m_errorStr.clear();

    KeePass2Reader reader;
    reader.setSaveXml(true);

    QScopedPointer<Database> db(reader.readDatabase(device, key, true));
    if (!reader.hasError()) {
        return qMakePair(NothingTodo, nullptr);
    }

    QByteArray xmlData = reader.xmlData();
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
        quint8 ch = static_cast<quint8>(xmlData.at(i));
        if (ch < 0x20 && ch != 0x09 && ch != 0x0A && ch != 0x0D) {
            xmlData.remove(i, 1);
            repairAction = true;
        }
    }

    if (!repairAction) {
        // we were unable to find the problem
        return qMakePair(RepairFailed, nullptr);
    }

    KeePass2RandomStream randomStream;
    randomStream.init(reader.streamKey());
    KeePass2XmlReader xmlReader;
    QBuffer buffer(&xmlData);
    buffer.open(QIODevice::ReadOnly);
    xmlReader.readDatabase(&buffer, db.data(), &randomStream);

    if (xmlReader.hasError()) {
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

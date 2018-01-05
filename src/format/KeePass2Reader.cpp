/*
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

#include <QtGlobal>
#include <QString>
#include <QFile>

#include "core/Endian.h"
#include "keys/CompositeKey.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass1.h"
#include "format/KeePass2.h"
#include "format/Kdbx3Reader.h"
#include "format/Kdbx4Reader.h"

BaseKeePass2Reader::BaseKeePass2Reader()
    : m_error(false)
    , m_saveXml(false)
    , m_irsAlgo(KeePass2::InvalidProtectedStreamAlgo)
{
    m_errorStr.clear();
    m_xmlData.clear();
    m_protectedStreamKey.clear();
}

Database* BaseKeePass2Reader::readDatabase(const QString& filename, const CompositeKey& key)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        raiseError(file.errorString());
        return nullptr;
    }

    QScopedPointer<Database> db(readDatabase(&file, key));

    if (file.error() != QFile::NoError) {
        raiseError(file.errorString());
        return nullptr;
    }

    return db.take();
}

bool BaseKeePass2Reader::hasError()
{
    return m_error;
}

QString BaseKeePass2Reader::errorString()
{
    return m_errorStr;
}

void BaseKeePass2Reader::setSaveXml(bool save)
{
    m_saveXml = save;
}

QByteArray BaseKeePass2Reader::xmlData()
{
    return m_xmlData;
}

QByteArray BaseKeePass2Reader::streamKey()
{
    return m_protectedStreamKey;
}

KeePass2::ProtectedStreamAlgo BaseKeePass2Reader::protectedStreamAlgo() const {
    return m_irsAlgo;
}


void BaseKeePass2Reader::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}

Database* KeePass2Reader::readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase)
{
    m_error = false;
    m_errorStr.clear();

    bool ok;

    quint32 signature1 = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
    if (!ok || signature1 != KeePass2::SIGNATURE_1) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    quint32 signature2 = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
    if (ok && signature2 == KeePass1::SIGNATURE_2) {
        raiseError(tr("The selected file is an old KeePass 1 database (.kdb).\n\n"
                      "You can import it by clicking on Database > 'Import KeePass 1 database...'.\n"
                      "This is a one-way migration. You won't be able to open the imported "
                      "database with the old KeePassX 0.4 version."));
        return nullptr;
    }
    else if (!ok || signature2 != KeePass2::SIGNATURE_2) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    m_version = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok)
            & KeePass2::FILE_VERSION_CRITICAL_MASK;
    quint32 maxVersion = KeePass2::FILE_VERSION_4 & KeePass2::FILE_VERSION_CRITICAL_MASK;
    if (!ok || (m_version < KeePass2::FILE_VERSION_MIN) || (m_version > maxVersion)) {
        raiseError(tr("Unsupported KeePass 2 database version."));
        return nullptr;
    }

    device->seek(0);

    // Determine KDBX3 vs KDBX4
    if (m_version < KeePass2::FILE_VERSION_4) {
        m_reader.reset(new Kdbx3Reader());
    } else {
        m_reader.reset(new Kdbx4Reader());
    }

    m_reader->setSaveXml(m_saveXml);
    return m_reader->readDatabase(device, key, keepDatabase);
}

bool KeePass2Reader::hasError()
{
    return m_error || (!m_reader.isNull() && m_reader->hasError());
}

QString KeePass2Reader::errorString()
{
    return !m_reader.isNull() ? m_reader->errorString() : m_errorStr;
}

QByteArray KeePass2Reader::xmlData()
{
    return !m_reader.isNull() ? m_reader->xmlData() : m_xmlData;
}

QByteArray KeePass2Reader::streamKey()
{
    return !m_reader.isNull() ? m_reader->streamKey() : m_protectedStreamKey;
}

KeePass2::ProtectedStreamAlgo KeePass2Reader::protectedStreamAlgo() const
{
    return !m_reader.isNull() ? m_reader->protectedStreamAlgo() : m_irsAlgo;
}

quint32 KeePass2Reader::version() const
{
    return m_version;
}

QSharedPointer<BaseKeePass2Reader> KeePass2Reader::reader()
{
    return m_reader;
}
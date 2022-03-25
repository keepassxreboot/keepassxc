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
#include "ShareImport.h"
#include "core/Merger.h"
#include "format/KeePass2Reader.h"
#include "keeshare/KeeShare.h"
#include "keys/PasswordKey.h"

#include <QBuffer>
#include <minizip/unzip.h>

namespace
{
    QByteArray readZipFile(void* uf)
    {
        QByteArray data;
        int bytes, bytesRead = 0;
        unzOpenCurrentFile(uf);
        do {
            data.resize(data.size() + 8192);
            bytes = unzReadCurrentFile(uf, data.data() + bytesRead, 8192);
            if (bytes > 0) {
                bytesRead += bytes;
            }
        } while (bytes > 0);
        unzCloseCurrentFile(uf);
        data.truncate(bytesRead);
        return data;
    }
} // namespace

ShareObserver::Result ShareImport::containerInto(const QString& resolvedPath,
                                                 const KeeShareSettings::Reference& reference,
                                                 Group* targetGroup)
{
    QByteArray dbData;

    auto uf = unzOpen64(resolvedPath.toLatin1().constData());
    if (uf) {
        // Open zip share, extract database portion, ignore signature file
        char zipFileName[256];
        auto err = unzGoToFirstFile(uf);
        while (err == UNZ_OK) {
            unzGetCurrentFileInfo64(uf, nullptr, zipFileName, sizeof(zipFileName), nullptr, 0, nullptr, 0);
            if (QString(zipFileName).compare(KeeShare::containerFileName()) == 0) {
                dbData = readZipFile(uf);
            }
            err = unzGoToNextFile(uf);
        }
        unzClose(uf);
    } else {
        // Open KDBX file directly
        QFile file(resolvedPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Unable to open file %s.", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Error, file.errorString()};
        }
        dbData = file.readAll();
        file.close();
    }

    QBuffer buffer(&dbData);
    buffer.open(QIODevice::ReadOnly);

    KeePass2Reader reader;
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(reference.password));
    auto sourceDb = QSharedPointer<Database>::create();
    if (!reader.readDatabase(&buffer, key, sourceDb.data())) {
        qCritical("Error while parsing the database: %s", qPrintable(reader.errorString()));
        return {reference.path, ShareObserver::Result::Error, reader.errorString()};
    }

    qDebug("Synchronize %s %s with %s",
           qPrintable(reference.path),
           qPrintable(targetGroup->name()),
           qPrintable(sourceDb->rootGroup()->name()));

    Merger merger(sourceDb->rootGroup(), targetGroup);
    merger.setForcedMergeMode(Group::Synchronize);
    auto changelist = merger.merge();
    if (!changelist.isEmpty()) {
        return {reference.path, ShareObserver::Result::Success, ShareImport::tr("Successful import")};
    }

    return {};
}

/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_KEEPASS1READER_H
#define KEEPASSX_KEEPASS1READER_H

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>

class Database;
class Entry;
class Group;
class SymmetricCipherStream;
class QIODevice;

class KeePass1Reader
{
    Q_DECLARE_TR_FUNCTIONS(KeePass1Reader)

public:
    KeePass1Reader();
    Database* readDatabase(QIODevice* device, const QString& password,
                           QIODevice* keyfileDevice);
    Database* readDatabase(QIODevice* device, const QString& password,
                           const QString& keyfileName);
    Database* readDatabase(const QString& filename, const QString& password,
                           const QString& keyfileName);
    bool hasError();
    QString errorString();

private:
    enum PasswordEncoding
    {
        Windows1252,
        Latin1,
        UTF8
    };

    SymmetricCipherStream* testKeys(const QString& password, const QByteArray& keyfileData,
                                    qint64 contentPos);
    QByteArray key(const QByteArray& password, const QByteArray& keyfileData);
    bool verifyKey(SymmetricCipherStream* cipherStream);
    Group* readGroup(QIODevice* cipherStream);
    Entry* readEntry(QIODevice* cipherStream);
    void parseNotes(const QString& rawNotes, Entry* entry);
    bool constructGroupTree(const QList<Group*>& groups);
    void parseMetaStream(const Entry* entry);
    bool parseGroupTreeState(const QByteArray& data);
    bool parseCustomIcons4(const QByteArray& data);
    void raiseError(const QString& errorMessage);
    static QByteArray readKeyfile(QIODevice* device);
    static QDateTime dateFromPackedStruct(const QByteArray& data);
    static bool isMetaStream(const Entry* entry);

    Database* m_db;
    Group* m_tmpParent;
    QIODevice* m_device;
    quint32 m_encryptionFlags;
    QByteArray m_masterSeed;
    QByteArray m_encryptionIV;
    QByteArray m_contentHashHeader;
    QByteArray m_transformSeed;
    quint32 m_transformRounds;
    QHash<quint32, Group*> m_groupIds;
    QHash<Group*, quint32> m_groupLevels;
    QHash<QByteArray, Entry*> m_entryUuids;
    QHash<Entry*, quint32> m_entryGroupIds;

    bool m_error;
    QString m_errorStr;
};

#endif // KEEPASSX_KEEPASS1READER_H

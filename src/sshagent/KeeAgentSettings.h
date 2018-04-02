/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
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

#ifndef KEEAGENTSETTINGS_H
#define KEEAGENTSETTINGS_H

#include <QXmlStreamReader>
#include <QtCore>

class KeeAgentSettings
{
public:
    KeeAgentSettings();

    bool operator==(KeeAgentSettings& other);
    bool operator!=(KeeAgentSettings& other);
    bool isDefault();

    bool fromXml(const QByteArray& ba);
    QByteArray toXml();

    bool allowUseOfSshKey() const;
    bool addAtDatabaseOpen() const;
    bool removeAtDatabaseClose() const;
    bool useConfirmConstraintWhenAdding() const;
    bool useLifetimeConstraintWhenAdding() const;
    int lifetimeConstraintDuration() const;

    const QString selectedType() const;
    const QString attachmentName() const;
    bool saveAttachmentToTempFile() const;
    const QString fileName() const;

    void setAllowUseOfSshKey(bool allowUseOfSshKey);
    void setAddAtDatabaseOpen(bool addAtDatabaseOpen);
    void setRemoveAtDatabaseClose(bool removeAtDatabaseClose);
    void setUseConfirmConstraintWhenAdding(bool useConfirmConstraintWhenAdding);
    void setUseLifetimeConstraintWhenAdding(bool useLifetimeConstraintWhenAdding);
    void setLifetimeConstraintDuration(int lifetimeConstraintDuration);

    void setSelectedType(const QString& type);
    void setAttachmentName(const QString& attachmentName);
    void setSaveAttachmentToTempFile(bool);
    void setFileName(const QString& fileName);

private:
    bool readBool(QXmlStreamReader& reader);
    int readInt(QXmlStreamReader& reader);

    bool m_allowUseOfSshKey;
    bool m_addAtDatabaseOpen;
    bool m_removeAtDatabaseClose;
    bool m_useConfirmConstraintWhenAdding;
    bool m_useLifetimeConstraintWhenAdding;
    int m_lifetimeConstraintDuration;

    // location
    QString m_selectedType;
    QString m_attachmentName;
    bool m_saveAttachmentToTempFile;
    QString m_fileName;
};

#endif // KEEAGENTSETTINGS_H

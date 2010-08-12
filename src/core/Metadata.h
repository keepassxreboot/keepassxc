/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_METADATA_H
#define KEEPASSX_METADATA_H

#include "Uuid.h"

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtGui/QImage>

class Metadata
{
public:
    Metadata();

    QString generator() const;
    QString name() const;
    QDateTime nameChanged() const;
    QString description() const;
    QDateTime descriptionChanged() const;
    QString defaultUserName() const;
    QDateTime defaultUserNameChanged() const;
    int maintenanceHistoryDays() const;
    bool protectTitle() const;
    bool protectUsername() const;
    bool protectPassword() const;
    bool protectUrl() const;
    bool protectNotes() const;
    bool autoEnableVisualHiding() const;
    QHash<Uuid, QImage> customIcons() const;
    bool recycleBinEnabled() const;
    Uuid recycleBinUuid() const;
    QDateTime recycleBinChanged() const;
    Uuid entryTemplatesGroup() const;
    QDateTime entryTemplatesGroupChanged() const;
    Uuid lastSelectedGroup() const;
    Uuid lastTopVisibleGroup() const;
    QHash<QString, QString> customFields() const;

    void setGenerator(const QString& value);
    void setName(const QString& value);
    void setNameChanged(const QDateTime& value);
    void setDescription(const QString& value);
    void setDescriptionChanged(const QDateTime& value);
    void setDefaultUserName(const QString& value);
    void setDefaultUserNameChanged(const QDateTime& value);
    void setMaintenanceHistoryDays(int value);
    void setProtectTitle(bool value);
    void setProtectUsername(bool value);
    void setProtectPassword(bool value);
    void setProtectUrl(bool value);
    void setProtectNotes(bool value);
    void setAutoEnableVisualHiding(bool value);
    void addCustomIcon(const Uuid& uuid, const QImage& image);
    void removeCustomIcon(const Uuid& uuid);
    void setRecycleBinEnabled(bool value);
    void setRecycleBinUuid(const Uuid& value);
    void setRecycleBinChanged(const QDateTime& value);
    void setEntryTemplatesGroup(const Uuid& value);
    void setEntryTemplatesGroupChanged(const QDateTime& value);
    void setLastSelectedGroup(const Uuid& value);
    void setLastTopVisibleGroup(const Uuid& value);
    void addCustomField(const QString& key, const QString& value);
    void removeCustomField(const QString& key);

private:
    QString m_generator;
    QString m_name;
    QDateTime m_nameChanged;
    QString m_description;
    QDateTime m_descriptionChanged;
    QString m_defaultUserName;
    QDateTime m_defaultUserNameChanged;
    int m_maintenanceHistoryDays;

    bool m_protectTitle;
    bool m_protectUsername;
    bool m_protectPassword;
    bool m_protectUrl;
    bool m_protectNotes;
    bool m_autoEnableVisualHiding;

    QHash<Uuid, QImage> m_customIcons;

    bool m_recycleBinEnabled;
    Uuid m_recycleBinUuid;
    QDateTime m_recycleBinChanged;
    Uuid m_entryTemplatesGroup;
    QDateTime m_entryTemplatesGroupChanged;
    Uuid m_lastSelectedGroup;
    Uuid m_lastTopVisibleGroup;

    QHash<QString, QString> m_customFields;
};

#endif // KEEPASSX_METADATA_H

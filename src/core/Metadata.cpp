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

#include "Metadata.h"

#include "core/Database.h"
#include "core/Entry.h"

Metadata::Metadata(Database* parent)
    : QObject(parent)
    , m_parent(parent)
{
    m_recycleBin = 0;
    m_entryTemplatesGroup = 0;
    m_lastSelectedGroup = 0;
    m_lastTopVisibleGroup = 0;

    m_generator = "KeePassX";
    m_maintenanceHistoryDays = 365;
    m_recycleBinEnabled = true;
    m_masterKeyChangeRec = -1;
    m_masterKeyChangeForce = -1;

    QDateTime now = QDateTime::currentDateTimeUtc();
    m_nameChanged = now;
    m_descriptionChanged = now;
    m_defaultUserNameChanged = now;
    m_recycleBinChanged = now;
    m_entryTemplatesGroupChanged = now;
    m_masterKeyChanged = now;

    m_protectTitle = false;
    m_protectUsername = false;
    m_protectPassword = true;
    m_protectUrl = false;
    m_protectNotes = false;
    m_autoEnableVisualHiding = false;

    m_updateDatetime = true;
}

template <class T> bool Metadata::set(T& property, const T& value) {
    if (property != value) {
        property = value;
        Q_EMIT modified();
        return true;
    }
    else {
        return false;
    }
}

template <class T> bool Metadata::set(T& property, const T& value, QDateTime& dateTime) {
    if (property != value) {
        property = value;
        if (m_updateDatetime) {
            dateTime = QDateTime::currentDateTimeUtc();
        }
        Q_EMIT modified();
        return true;
    }
    else {
        return false;
    }
}

void Metadata::setUpdateDatetime(bool value) {
    m_updateDatetime = value;
}

QString Metadata::generator() const
{
    return m_generator;
}

QString Metadata::name() const
{
    return m_name;
}

QDateTime Metadata::nameChanged() const
{
    return m_nameChanged;
}

QString Metadata::description() const
{
    return m_description;
}

QDateTime Metadata::descriptionChanged() const
{
    return m_descriptionChanged;
}

QString Metadata::defaultUserName() const
{
    return m_defaultUserName;
}

QDateTime Metadata::defaultUserNameChanged() const
{
    return m_defaultUserNameChanged;
}

int Metadata::maintenanceHistoryDays() const
{
    return m_maintenanceHistoryDays;
}

bool Metadata::protectTitle() const
{
    return m_protectTitle;
}

bool Metadata::protectUsername() const
{
    return m_protectUsername;
}

bool Metadata::protectPassword() const
{
    return m_protectPassword;
}

bool Metadata::protectUrl() const
{
    return m_protectUrl;
}

bool Metadata::protectNotes() const
{
    return m_protectNotes;
}

bool Metadata::autoEnableVisualHiding() const
{
    return m_autoEnableVisualHiding;
}

QImage Metadata::customIcon(const Uuid& uuid) const
{
    return m_customIcons.value(uuid);
}

QHash<Uuid, QImage> Metadata::customIcons() const
{
    return m_customIcons;
}

bool Metadata::recycleBinEnabled() const
{
    return m_recycleBinEnabled;
}

const Group* Metadata::recycleBin() const
{
    return m_recycleBin;
}

QDateTime Metadata::recycleBinChanged() const
{
    return m_recycleBinChanged;
}

const Group* Metadata::entryTemplatesGroup() const
{
    return m_entryTemplatesGroup;
}

QDateTime Metadata::entryTemplatesGroupChanged() const
{
    return m_entryTemplatesGroupChanged;
}

const Group* Metadata::lastSelectedGroup() const
{
    return m_lastSelectedGroup;
}

const Group* Metadata::lastTopVisibleGroup() const
{
    return m_lastTopVisibleGroup;
}

QDateTime Metadata::masterKeyChanged() const
{
    return m_masterKeyChanged;
}

int Metadata::masterKeyChangeRec() const
{
    return m_masterKeyChangeRec;
}

int Metadata::masterKeyChangeForce() const
{
    return m_masterKeyChangeForce;
}

QHash<QString, QString> Metadata::customFields() const
{
    return m_customFields;
}

void Metadata::setGenerator(const QString& value)
{
    set(m_generator, value);
}

void Metadata::setName(const QString& value)
{
    if (set(m_name, value, m_nameChanged)) {
        Q_EMIT nameTextChanged(m_parent);
    }
}

void Metadata::setNameChanged(const QDateTime& value)
{
    m_nameChanged = value;
}

void Metadata::setDescription(const QString& value)
{
    set(m_description, value, m_descriptionChanged);
}

void Metadata::setDescriptionChanged(const QDateTime& value)
{
    m_descriptionChanged = value;
}

void Metadata::setDefaultUserName(const QString& value)
{
    set(m_defaultUserName, value, m_defaultUserNameChanged);
}

void Metadata::setDefaultUserNameChanged(const QDateTime& value)
{
    m_defaultUserNameChanged = value;
}

void Metadata::setMaintenanceHistoryDays(int value)
{
    set(m_maintenanceHistoryDays, value);
}

void Metadata::setProtectTitle(bool value)
{
    set(m_protectTitle, value);
}

void Metadata::setProtectUsername(bool value)
{
    set(m_protectUsername, value);
}

void Metadata::setProtectPassword(bool value)
{
    set(m_protectPassword, value);
}

void Metadata::setProtectUrl(bool value)
{
    set(m_protectUrl, value);
}

void Metadata::setProtectNotes(bool value)
{
    set(m_protectNotes, value);
}

void Metadata::setAutoEnableVisualHiding(bool value)
{
    set(m_autoEnableVisualHiding, value);
}

void Metadata::addCustomIcon(const Uuid& uuid, const QImage& icon)
{
    Q_ASSERT(!uuid.isNull());
    Q_ASSERT(!m_customIcons.contains(uuid));

    m_customIcons.insert(uuid, icon);
    Q_EMIT modified();
}

void Metadata::removeCustomIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    Q_ASSERT(m_customIcons.contains(uuid));

    m_customIcons.remove(uuid);
    Q_EMIT modified();
}

void Metadata::setRecycleBinEnabled(bool value)
{
    set(m_recycleBinEnabled, value);
}

void Metadata::setRecycleBin(Group* group)
{
    set(m_recycleBin, group, m_recycleBinChanged);
}

void Metadata::setRecycleBinChanged(const QDateTime& value)
{
    m_recycleBinChanged = value;
}

void Metadata::setEntryTemplatesGroup(Group* group)
{
    set(m_entryTemplatesGroup, group, m_entryTemplatesGroupChanged);
}

void Metadata::setEntryTemplatesGroupChanged(const QDateTime& value)
{
    m_entryTemplatesGroupChanged = value;
}

void Metadata::setLastSelectedGroup(Group* group)
{
    set(m_lastSelectedGroup, group);
}

void Metadata::setLastTopVisibleGroup(Group* group)
{
    set(m_lastTopVisibleGroup, group);
}

void Metadata::setMasterKeyChanged(const QDateTime& value)
{
    m_masterKeyChanged = value;
}

void Metadata::setMasterKeyChangeRec(int value)
{
    set(m_masterKeyChangeRec, value);
}

void Metadata::setMasterKeyChangeForce(int value)
{
    set(m_masterKeyChangeForce, value);
}

void Metadata::addCustomField(const QString& key, const QString& value)
{
    Q_ASSERT(!m_customFields.contains(key));

    m_customFields.insert(key, value);
    Q_EMIT modified();
}

void Metadata::removeCustomField(const QString& key)
{
    Q_ASSERT(m_customFields.contains(key));

    m_customFields.remove(key);
    Q_EMIT modified();
}

void Metadata::addEntryToRecycleBin(Entry* entry)
{
    entry->setGroup(m_recycleBin);
}

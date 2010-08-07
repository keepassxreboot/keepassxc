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

Metadata::Metadata()
{
}

QString Metadata::generator() const
{
    return m_generator;
};

QString Metadata::name() const
{
    return m_name;
};

QDateTime Metadata::nameChanged() const
{
    return m_nameChanged;
};

QString Metadata::description() const
{
    return m_description;
};

QDateTime Metadata::descriptionChanged() const
{
    return m_descriptionChanged;
};

QString Metadata::defaultUserName() const
{
    return m_defaultUserName;
};

int Metadata::maintenanceHistoryDays() const
{
    return m_maintenanceHistoryDays;
};

bool Metadata::protectTitle() const
{
    return m_protectTitle;
};

bool Metadata::protectUsername() const
{
    return m_protectUsername;
};

bool Metadata::protectPassword() const
{
    return m_protectPassword;
};

bool Metadata::protectUrl() const
{
    return m_protectUrl;
};

bool Metadata::protectNotes() const
{
    return m_protectNotes;
};

bool Metadata::autoEnableVisualHiding() const
{
    return m_autoEnableVisualHiding;
};

QHash<Uuid, QImage> Metadata::customIcons() const
{
    return m_customIcons;
};

bool Metadata::recycleBinEnabled() const
{
    return m_recycleBinEnabled;
};

Uuid Metadata::recycleBinUuid() const
{
    return m_recycleBinUuid;
};

QDateTime Metadata::recycleBinChanged() const
{
    return m_recycleBinChanged;
};

Uuid Metadata::entryTemplatesGroup() const
{
    return m_entryTemplatesGroup;
};

QDateTime Metadata::entryTemplatesGroupChanged() const
{
    return m_entryTemplatesGroupChanged;
};

Uuid Metadata::lastSelectedGroup() const
{
    return m_lastSelectedGroup;
};

Uuid Metadata::lastTopVisibleGroup() const
{
    return m_lastTopVisibleGroup;
};

QHash<QString, QString> Metadata::customFields() const
{
    return m_customFields;
};

void Metadata::setGenerator(const QString& value)
{
    m_generator = value;
}

void Metadata::setName(const QString& value)
{
    m_name = value;
}

void Metadata::setNameChanged(const QDateTime& value)
{
    m_nameChanged = value;
}

void Metadata::setDescription(const QString& value)
{
    m_description = value;
}

void Metadata::setDescriptionChanged(const QDateTime& value)
{
    m_descriptionChanged = value;
}

void Metadata::setDefaultUserName(const QString& value)
{
    m_defaultUserName = value;
}

void Metadata::setMaintenanceHistoryDays(int value)
{
    m_maintenanceHistoryDays = value;
}

void Metadata::setProtectTitle(bool value)
{
    m_protectTitle = value;
}

void Metadata::setProtectUsername(bool value)
{
    m_protectUsername = value;
}

void Metadata::setProtectPassword(bool value)
{
    m_protectPassword = value;
}

void Metadata::setProtectUrl(bool value)
{
    m_protectUrl = value;
}

void Metadata::setProtectNotes(bool value)
{
    m_protectNotes = value;
}

void Metadata::setAutoEnableVisualHiding(bool value)
{
    m_autoEnableVisualHiding = value;
}

void Metadata::addCustomIcon(const Uuid& uuid, const QImage& image)
{
    Q_ASSERT(!m_customIcons.contains(uuid));

    m_customIcons.insert(uuid, image);
}

void Metadata::removeCustomIcon(const Uuid& uuid)
{
    Q_ASSERT(m_customIcons.contains(uuid));

    m_customIcons.remove(uuid);
}

void Metadata::setRecycleBinEnabled(bool value)
{
    m_recycleBinEnabled = value;
}

void Metadata::setRecycleBinUuid(const Uuid& value)
{
    m_recycleBinUuid = value;
}

void Metadata::setRecycleBinChanged(const QDateTime& value)
{
    m_recycleBinChanged = value;
}

void Metadata::setEntryTemplatesGroup(const Uuid& value)
{
    m_entryTemplatesGroup = value;
}

void Metadata::setEntryTemplatesGroupChanged(const QDateTime& value)
{
    m_entryTemplatesGroupChanged = value;
}

void Metadata::setLastSelectedGroup(const Uuid& value)
{
    m_lastSelectedGroup = value;
}

void Metadata::setLastTopVisibleGroup(const Uuid& value)
{
    m_lastTopVisibleGroup = value;
}

void Metadata::addCustomField(const QString& key, const QString& value)
{
    Q_ASSERT(!m_customFields.contains(key));

    m_customFields.insert(key, value);
}

void Metadata::removeCustomField(const QString& key)
{
    Q_ASSERT(m_customFields.contains(key));

    m_customFields.remove(key);
}

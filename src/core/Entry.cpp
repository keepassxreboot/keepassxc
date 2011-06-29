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

#include "Entry.h"

#include "Database.h"
#include "DatabaseIcons.h"
#include "Group.h"
#include "Metadata.h"

Entry::Entry()
{
    m_group = 0;
    m_db = 0;

    m_iconNumber = 0; // TODO change?
}

Entry::~Entry()
{
    // TODO notify group
    qDeleteAll(m_history);
}

Uuid Entry::uuid() const
{
    return m_uuid;
}

QIcon Entry::icon() const
{
    if (m_customIcon.isNull()) {
        return DatabaseIcons::icon(m_iconNumber);
    }
    else {
        return m_db->metadata()->customIcon(m_customIcon);
    }
}

int Entry::iconNumber() const
{
    return m_iconNumber;
}

Uuid Entry::iconUuid() const
{
    return m_customIcon;
}

QColor Entry::foregroundColor() const
{
    return m_foregroundColor;
}

QColor Entry::backgroundColor() const
{
    return m_backgroundColor;
}

QString Entry::overrideUrl() const
{
    return m_overrideUrl;
}

QString Entry::tags() const
{
    return m_tags;
}

TimeInfo Entry::timeInfo() const
{
    return m_timeInfo;
}

bool Entry::autoTypeEnabled() const
{
    return m_autoTypeEnabled;
}

int Entry::autoTypeObfuscation() const
{
    return m_autoTypeObfuscation;
}

QString Entry::defaultAutoTypeSequence() const
{
    return m_defaultAutoTypeSequence;
}

const QList<AutoTypeAssociation>& Entry::autoTypeAssociations() const
{
    return m_autoTypeAssociations;
}

const QHash<QString, QString>& Entry::attributes() const
{
    return m_attributes;
}

const QHash<QString, QByteArray>& Entry::attachments() const
{
    return m_binaries;
}

bool Entry::isAttributeProtected(const QString& key) const
{
    return m_protectedAttributes.contains(key);
}

bool Entry::isAttachmentProtected(const QString& key) const
{
    return m_protectedAttachments.contains(key);
}

QString Entry::title() const
{
    return m_attributes.value("Title");
}

QString Entry::url() const
{
    return m_attributes.value("URL");
}

QString Entry::username() const
{
    return m_attributes.value("UserName");
}

QString Entry::password() const
{
    return m_attributes.value("Password");
}

QString Entry::notes() const
{
    return m_attributes.value("Notes");
}

void Entry::setUuid(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    m_uuid = uuid;
}

void Entry::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    m_iconNumber = iconNumber;
    m_customIcon = Uuid();
}

void Entry::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    m_iconNumber = 0;
    m_customIcon = uuid;
}

void Entry::setForegroundColor(const QColor& color)
{
    m_foregroundColor = color;
}

void Entry::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
}

void Entry::setOverrideUrl(const QString& url)
{
    m_overrideUrl = url;
}

void Entry::setTags(const QString& tags)
{
    m_tags = tags;
}

void Entry::setTimeInfo(const TimeInfo& timeInfo)
{
    m_timeInfo = timeInfo;
}

void Entry::setAutoTypeEnabled(bool enable)
{
    m_autoTypeEnabled = enable;
}

void Entry::setAutoTypeObfuscation(int obfuscation)
{
    m_autoTypeObfuscation = obfuscation;
}

void Entry::setDefaultAutoTypeSequence(const QString& sequence)
{
    m_defaultAutoTypeSequence = sequence;
}

void Entry::addAutoTypeAssociation(const AutoTypeAssociation& assoc)
{
    m_autoTypeAssociations << assoc;
}

void Entry::addAttribute(const QString& key, const QString& value, bool protect)
{
    m_attributes.insert(key, value);
    if (protect) {
        m_protectedAttributes.insert(key);
    }

    // TODO add all visible columns
    if (key == "Title") {
        Q_EMIT dataChanged(this);
    }
}

void Entry::removeAttribute(const QString& key)
{
    m_attributes.remove(key);
    m_protectedAttributes.remove(key);
}

void Entry::addAttachment(const QString& key, const QByteArray& value, bool protect)
{
    m_binaries.insert(key, value);
    if (protect) {
        m_protectedAttachments.insert(key);
    }
}

void Entry::removeAttachment(const QString& key)
{
    m_binaries.remove(key);
    m_protectedAttachments.remove(key);
}

void Entry::setTitle(const QString& title)
{
    addAttribute("Title", title);
}

void Entry::setUrl(const QString& url)
{
    addAttribute("URL", url);
}

void Entry::setUsername(const QString& username)
{
    addAttribute("UserName", username);
}

void Entry::setPassword(const QString& password)
{
    addAttribute("Password", password);
}

void Entry::setNotes(const QString& notes)
{
    addAttribute("Notes", notes);
}

QList<Entry*> Entry::historyItems()
{
    return m_history;
}

const QList<Entry*>& Entry::historyItems() const
{
    return m_history;
}

void Entry::addHistoryItem(Entry* entry)
{
    Q_ASSERT(!entry->parent());

    m_history.append(entry);
}

Group* Entry::group()
{
    return m_group;
}

void Entry::setGroup(Group* group)
{
    if (m_group) {
        m_group->removeEntry(this);
    }
    group->addEntry(this);
    m_group = group;
    m_db = group->database();
    QObject::setParent(group);
}

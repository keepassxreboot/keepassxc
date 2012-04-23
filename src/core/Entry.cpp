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

#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"

Entry::Entry()
{
    m_updateTimeinfo = true;
    m_tmpHistoryItem = 0;

    m_data.iconNumber = 0;
    m_data.autoTypeEnabled = true;
    m_data.autoTypeObfuscation = 0;

    m_attributes = new EntryAttributes(this);
    connect(m_attributes, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_attributes, SIGNAL(defaultKeyModified()), SLOT(emitDataChanged()));

    m_attachments = new EntryAttachments(this);
    connect(m_attachments, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(this, SIGNAL(modified()), this, SLOT(updateTimeinfo()));

    connect(this, SIGNAL(modified()), SLOT(updateModifiedSinceBegin()));
}

Entry::~Entry()
{
    if (m_group) {
        m_group->removeEntry(this);

        if (m_group->database()) {
            m_group->database()->addDeletedObject(m_uuid);
        }
    }

    qDeleteAll(m_history);
}

template <class T> bool Entry::set(T& property, const T& value)
{
    if (property != value) {
        property = value;
        Q_EMIT modified();
        return true;
    }
    else {
        return false;
    }
}

void Entry::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_data.timeInfo.setLastModificationTime(QDateTime::currentDateTimeUtc());
        m_data.timeInfo.setLastAccessTime(QDateTime::currentDateTimeUtc());
    }
}

void Entry::setUpdateTimeinfo(bool value)
{
    m_updateTimeinfo = value;
}

Uuid Entry::uuid() const
{
    return m_uuid;
}

QImage Entry::icon() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->icon(m_data.iconNumber);
    }
    else {
        // TODO check if database() is 0
        return database()->metadata()->customIcon(m_data.customIcon);
    }
}

QPixmap Entry::iconPixmap() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    else {
        QPixmap pixmap;
        if (!QPixmapCache::find(m_pixmapCacheKey, &pixmap)) {
            // TODO check if database() is 0
            pixmap = QPixmap::fromImage(database()->metadata()->customIcon(m_data.customIcon));
            *const_cast<QPixmapCache::Key*>(&m_pixmapCacheKey) = QPixmapCache::insert(pixmap);
        }

        return pixmap;
    }
}

int Entry::iconNumber() const
{
    return m_data.iconNumber;
}

Uuid Entry::iconUuid() const
{
    return m_data.customIcon;
}

QColor Entry::foregroundColor() const
{
    return m_data.foregroundColor;
}

QColor Entry::backgroundColor() const
{
    return m_data.backgroundColor;
}

QString Entry::overrideUrl() const
{
    return m_data.overrideUrl;
}

QString Entry::tags() const
{
    return m_data.tags;
}

TimeInfo Entry::timeInfo() const
{
    return m_data.timeInfo;
}

bool Entry::autoTypeEnabled() const
{
    return m_data.autoTypeEnabled;
}

int Entry::autoTypeObfuscation() const
{
    return m_data.autoTypeObfuscation;
}

QString Entry::defaultAutoTypeSequence() const
{
    return m_data.defaultAutoTypeSequence;
}

const QList<AutoTypeAssociation>& Entry::autoTypeAssociations() const
{
    return m_data.autoTypeAssociations;
}

QString Entry::title() const
{
    return m_attributes->value("Title");
}

QString Entry::url() const
{
    return m_attributes->value("URL");
}

QString Entry::username() const
{
    return m_attributes->value("UserName");
}

QString Entry::password() const
{
    return m_attributes->value("Password");
}

QString Entry::notes() const
{
    return m_attributes->value("Notes");
}

EntryAttributes* Entry::attributes()
{
    return m_attributes;
}

const EntryAttributes* Entry::attributes() const
{
    return m_attributes;
}

EntryAttachments* Entry::attachments()
{
    return m_attachments;
}

const EntryAttachments* Entry::attachments() const
{
    return m_attachments;
}

void Entry::setUuid(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    set(m_uuid, uuid);
}

void Entry::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    if (m_data.iconNumber != iconNumber || !m_data.customIcon.isNull()) {
        m_data.iconNumber = iconNumber;
        m_data.customIcon = Uuid();

        m_pixmapCacheKey = QPixmapCache::Key();

        Q_EMIT modified();
    }
}

void Entry::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (m_data.customIcon != uuid) {
        m_data.customIcon = uuid;
        m_data.iconNumber = 0;

        m_pixmapCacheKey = QPixmapCache::Key();

        Q_EMIT modified();
    }
}

void Entry::setForegroundColor(const QColor& color)
{
    set(m_data.foregroundColor, color);
}

void Entry::setBackgroundColor(const QColor& color)
{
    set(m_data.backgroundColor, color);
}

void Entry::setOverrideUrl(const QString& url)
{
    set(m_data.overrideUrl, url);
}

void Entry::setTags(const QString& tags)
{
    set(m_data.tags, tags);
}

void Entry::setTimeInfo(const TimeInfo& timeInfo)
{
    m_data.timeInfo = timeInfo;
}

void Entry::setAutoTypeEnabled(bool enable)
{
    set(m_data.autoTypeEnabled, enable);
}

void Entry::setAutoTypeObfuscation(int obfuscation)
{
    set(m_data.autoTypeObfuscation, obfuscation);
}

void Entry::setDefaultAutoTypeSequence(const QString& sequence)
{
    set(m_data.defaultAutoTypeSequence, sequence);
}

void Entry::addAutoTypeAssociation(const AutoTypeAssociation& assoc)
{
    m_data.autoTypeAssociations << assoc;
    Q_EMIT modified();
}

void Entry::setTitle(const QString& title)
{
    m_attributes->set("Title", title, m_attributes->isProtected("Title"));
}

void Entry::setUrl(const QString& url)
{
    m_attributes->set("URL", url, m_attributes->isProtected("URL"));
}

void Entry::setUsername(const QString& username)
{
    m_attributes->set("UserName", username, m_attributes->isProtected("UserName"));
}

void Entry::setPassword(const QString& password)
{
    m_attributes->set("Password", password, m_attributes->isProtected("Password"));
}

void Entry::setNotes(const QString& notes)
{
    m_attributes->set("Notes", notes, m_attributes->isProtected("Notes"));
}

void Entry::setExpires(const bool& value)
{
    if (m_data.timeInfo.expires() != value) {
        m_data.timeInfo.setExpires(value);
        Q_EMIT modified();
    }
}

void Entry::setExpiryTime(const QDateTime& dateTime)
{
    if (m_data.timeInfo.expiryTime() != dateTime) {
        m_data.timeInfo.setExpiryTime(dateTime);
        Q_EMIT modified();
    }
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
    Q_ASSERT(entry->uuid() == uuid());

    m_history.append(entry);
    Q_EMIT modified();
}

void Entry::beginUpdate()
{
    Q_ASSERT(!m_tmpHistoryItem);

    m_tmpHistoryItem = new Entry();
    m_tmpHistoryItem->setUpdateTimeinfo(false);
    m_tmpHistoryItem->m_uuid = m_uuid;
    m_tmpHistoryItem->m_data = m_data;
    *m_tmpHistoryItem->m_attributes = *m_attributes;
    *m_tmpHistoryItem->m_attachments = *m_attachments;

    m_modifiedSinceBegin = false;
}

void Entry::endUpdate()
{
    Q_ASSERT(m_tmpHistoryItem);

    if (m_modifiedSinceBegin) {
        m_tmpHistoryItem->setUpdateTimeinfo(true);
        addHistoryItem(m_tmpHistoryItem);
    }
    else {
        delete m_tmpHistoryItem;
    }

    m_tmpHistoryItem = 0;
}

void Entry::updateModifiedSinceBegin()
{
    m_modifiedSinceBegin = true;
}

Group* Entry::group()
{
    return m_group;
}

void Entry::setGroup(Group* group)
{
    if (m_group == group) {
        return;
    }

    if (m_group) {
        m_group->removeEntry(this);
        if (m_group->database() != group->database() && m_group->database()) {
            m_group->database()->addDeletedObject(m_uuid);
        }
    }

    group->addEntry(this);
    m_group = group;
    QObject::setParent(group);

    if (m_updateTimeinfo) {
        m_data.timeInfo.setLocationChanged(QDateTime::currentDateTimeUtc());
    }
}

void Entry::emitDataChanged()
{
    Q_EMIT dataChanged(this);
}

const Database* Entry::database() const
{
    if (m_group) {
        return m_group->database();
    }
    else {
        return 0;
    }
}

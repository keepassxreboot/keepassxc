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

const QStringList Entry::m_defaultAttibutes(QStringList() << "Title" << "URL" << "UserName" << "Password" << "Notes");

Entry::Entry()
{
    m_group = 0;
    m_db = 0;

    m_iconNumber = 0;
    m_autoTypeEnabled = true;
    m_autoTypeObfuscation = 0;

    Q_FOREACH (const QString& key, m_defaultAttibutes) {
        setAttribute(key, "");
    }
}

Entry::~Entry()
{
    if (m_group) {
        m_group->removeEntry(this);
    }

    qDeleteAll(m_history);
}

Uuid Entry::uuid() const
{
    return m_uuid;
}

QImage Entry::icon() const
{
    if (m_customIcon.isNull()) {
        return databaseIcons()->icon(m_iconNumber);
    }
    else {
        // TODO check if m_db is 0
        return m_db->metadata()->customIcon(m_customIcon);
    }
}

QPixmap Entry::iconPixmap() const
{
    if (m_customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_iconNumber);
    }
    else {
        QPixmap pixmap;
        if (!QPixmapCache::find(m_pixmapCacheKey, &pixmap)) {
            // TODO check if m_db is 0
            pixmap = QPixmap::fromImage(m_db->metadata()->customIcon(m_customIcon));
            *const_cast<QPixmapCache::Key*>(&m_pixmapCacheKey) = QPixmapCache::insert(pixmap);
        }

        return pixmap;
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

const QList<QString> Entry::attributes() const
{
    return m_attributes.keys();
}

QString Entry::attributeValue(const QString& key) const
{
    return m_attributes.value(key);
}

const QList<QString> Entry::attachments() const
{
    return m_binaries.keys();
}

QByteArray Entry::attachmentValue(const QString& key) const
{
    return m_binaries.value(key);
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

    m_pixmapCacheKey = QPixmapCache::Key();
}

void Entry::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    m_iconNumber = 0;
    m_customIcon = uuid;

    m_pixmapCacheKey = QPixmapCache::Key();
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

void Entry::setAttribute(const QString& key, const QString& value, bool protect)
{
    bool addAttribute = !m_attributes.contains(key);
    bool defaultAttribute = isDefaultAttribute(key);

    if (addAttribute && !defaultAttribute) {
        Q_EMIT attributeAboutToBeAdded(key);
    }

    m_attributes.insert(key, value);

    if (protect) {
        m_protectedAttributes.insert(key);
    }
    else {
        m_protectedAttributes.remove(key);
    }

    if (defaultAttribute) {
        Q_EMIT dataChanged(this);
    }
    else if (addAttribute) {
        Q_EMIT attributeAdded(key);
    }
    else {
        Q_EMIT attributeChanged(key);
    }
}

void Entry::removeAttribute(const QString& key)
{
    Q_ASSERT(!isDefaultAttribute(key));

    if (!m_attributes.contains(key)) {
        Q_ASSERT(false);
        return;
    }

    Q_EMIT attributeAboutToBeRemoved(key);

    m_attributes.remove(key);
    m_protectedAttributes.remove(key);

    Q_EMIT attributeRemoved(key);
}

void Entry::setAttachment(const QString& key, const QByteArray& value, bool protect)
{
    bool addAttachment = !m_binaries.contains(key);

    if (addAttachment) {
        Q_EMIT attachmentAboutToBeAdded(key);
    }

    m_binaries.insert(key, value);

    if (protect) {
        m_protectedAttachments.insert(key);
    }
    else {
        m_protectedAttachments.remove(key);
    }

    if (addAttachment) {
        Q_EMIT attachmentAdded(key);
    }
    else {
        Q_EMIT attachmentChanged(key);
    }
}

void Entry::removeAttachment(const QString& key)
{
    if (!m_binaries.contains(key)) {
        Q_ASSERT(false);
        return;
    }

    Q_EMIT attachmentAboutToBeRemoved(key);

    m_binaries.remove(key);
    m_protectedAttachments.remove(key);

    Q_EMIT attachmentRemoved(key);
}

void Entry::setTitle(const QString& title)
{
    setAttribute("Title", title);
}

void Entry::setUrl(const QString& url)
{
    setAttribute("URL", url);
}

void Entry::setUsername(const QString& username)
{
    setAttribute("UserName", username);
}

void Entry::setPassword(const QString& password)
{
    setAttribute("Password", password);
}

void Entry::setNotes(const QString& notes)
{
    setAttribute("Notes", notes);
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

bool Entry::isDefaultAttribute(const QString& key)
{
    return m_defaultAttibutes.contains(key);
}

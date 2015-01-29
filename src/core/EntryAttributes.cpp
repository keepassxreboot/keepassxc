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

#include "EntryAttributes.h"

const QString EntryAttributes::TitleKey = "Title";
const QString EntryAttributes::UserNameKey = "UserName";
const QString EntryAttributes::PasswordKey = "Password";
const QString EntryAttributes::URLKey = "URL";
const QString EntryAttributes::NotesKey = "Notes";
const QStringList EntryAttributes::DefaultAttributes(QStringList() << TitleKey << UserNameKey
                                                     << PasswordKey << URLKey << NotesKey);

EntryAttributes::EntryAttributes(QObject* parent)
    : QObject(parent)
{
    clear();
}

QList<QString> EntryAttributes::keys() const
{
    return m_attributes.keys();
}

bool EntryAttributes::hasKey(const QString& key) const
{
    return m_attributes.keys().contains(key);
}

QList<QString> EntryAttributes::customKeys()
{
    QList<QString> customKeys;
    Q_FOREACH (const QString& key, keys()) {
        if (!isDefaultAttribute(key)) {
            customKeys.append(key);
        }
    }
    return customKeys;
}

QString EntryAttributes::value(const QString& key) const
{
    return m_attributes.value(key);
}

bool EntryAttributes::contains(const QString &key) const
{
    return m_attributes.contains(key);
}

bool EntryAttributes::isProtected(const QString& key) const
{
    return m_protectedAttributes.contains(key);
}

void EntryAttributes::set(const QString& key, const QString& value, bool protect)
{
    bool emitModified = false;

    bool addAttribute = !m_attributes.contains(key);
    bool changeValue = !addAttribute && (m_attributes.value(key) != value);
    bool defaultAttribute = isDefaultAttribute(key);

    if (addAttribute && !defaultAttribute) {
        Q_EMIT aboutToBeAdded(key);
    }

    if (addAttribute || changeValue) {
        m_attributes.insert(key, value);
        emitModified = true;
    }

    if (protect) {
        if (!m_protectedAttributes.contains(key)) {
            emitModified = true;
        }
        m_protectedAttributes.insert(key);
    }
    else if (m_protectedAttributes.remove(key)) {
        emitModified = true;
    }

    if (emitModified) {
        Q_EMIT modified();
    }

    if (defaultAttribute && changeValue) {
        Q_EMIT defaultKeyModified();
    }
    else if (addAttribute) {
        Q_EMIT added(key);
    }
    else if (emitModified) {
        Q_EMIT customKeyModified(key);
    }
}

void EntryAttributes::remove(const QString& key)
{
    Q_ASSERT(!isDefaultAttribute(key));

    if (!m_attributes.contains(key)) {
        Q_ASSERT(false);
        return;
    }

    Q_EMIT aboutToBeRemoved(key);

    m_attributes.remove(key);
    m_protectedAttributes.remove(key);

    Q_EMIT removed(key);
    Q_EMIT modified();
}

void EntryAttributes::rename(const QString& oldKey, const QString& newKey)
{
    Q_ASSERT(!isDefaultAttribute(oldKey));
    Q_ASSERT(!isDefaultAttribute(newKey));

    if (!m_attributes.contains(oldKey)) {
        Q_ASSERT(false);
        return;
    }

    if (m_attributes.contains(newKey)) {
        Q_ASSERT(false);
        return;
    }

    QString data = value(oldKey);
    bool protect = isProtected(oldKey);

    Q_EMIT aboutToRename(oldKey, newKey);

    m_attributes.remove(oldKey);
    m_attributes.insert(newKey, data);
    if (protect) {
        m_protectedAttributes.remove(oldKey);
        m_protectedAttributes.insert(newKey);
    }

    Q_EMIT modified();
    Q_EMIT renamed(oldKey, newKey);
}

void EntryAttributes::copyCustomKeysFrom(const EntryAttributes* other)
{
    if (!areCustomKeysDifferent(other)) {
        return;
    }

    Q_EMIT aboutToBeReset();

    // remove all non-default keys
    Q_FOREACH (const QString& key, keys()) {
        if (!isDefaultAttribute(key)) {
            m_attributes.remove(key);
            m_protectedAttributes.remove(key);
        }
    }

    Q_FOREACH (const QString& key, other->keys()) {
        if (!isDefaultAttribute(key)) {
            m_attributes.insert(key, other->value(key));
            if (other->isProtected(key)) {
                m_protectedAttributes.insert(key);
            }
        }
    }

    Q_EMIT reset();
    Q_EMIT modified();
}

bool EntryAttributes::areCustomKeysDifferent(const EntryAttributes* other)
{
    // check if they are equal ignoring the order of the keys
    if (keys().toSet() != other->keys().toSet()) {
        return true;
    }

    Q_FOREACH (const QString& key, keys()) {
        if (isDefaultAttribute(key)) {
            continue;
        }

        if (isProtected(key) != other->isProtected(key) || value(key) != other->value(key)) {
            return true;
        }
    }

    return false;
}

void EntryAttributes::copyDataFrom(const EntryAttributes* other)
{
    if (*this != *other) {
        Q_EMIT aboutToBeReset();

        m_attributes = other->m_attributes;
        m_protectedAttributes = other->m_protectedAttributes;

        Q_EMIT reset();
        Q_EMIT modified();
    }
}

bool EntryAttributes::operator==(const EntryAttributes& other) const
{
    return (m_attributes == other.m_attributes
            && m_protectedAttributes == other.m_protectedAttributes);
}

bool EntryAttributes::operator!=(const EntryAttributes& other) const
{
    return (m_attributes != other.m_attributes
            || m_protectedAttributes != other.m_protectedAttributes);
}

void EntryAttributes::clear()
{
    Q_EMIT aboutToBeReset();

    m_attributes.clear();
    m_protectedAttributes.clear();

    Q_FOREACH (const QString& key, DefaultAttributes) {
        m_attributes.insert(key, "");
    }

    Q_EMIT reset();
    Q_EMIT modified();
}

int EntryAttributes::attributesSize()
{
    int size = 0;

    QMapIterator<QString, QString> i(m_attributes);
    while (i.hasNext()) {
        i.next();
        size += i.value().toUtf8().size();
    }
    return size;
}

bool EntryAttributes::isDefaultAttribute(const QString& key)
{
    return DefaultAttributes.contains(key);
}

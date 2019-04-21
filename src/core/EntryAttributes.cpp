/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "EntryAttributes.h"

#include "core/Tools.h"
#include "crypto/Crypto.h"

const QString EntryAttributes::TitleKey = "Title";
const QString EntryAttributes::UserNameKey = "UserName";
const QString EntryAttributes::PasswordKey = "Password";
const QString EntryAttributes::URLKey = "URL";
const QString EntryAttributes::NotesKey = "Notes";
const QStringList EntryAttributes::DefaultAttributes(
    QStringList() << TitleKey << UserNameKey << PasswordKey << URLKey << NotesKey);

const QString EntryAttributes::WantedFieldGroupName = "WantedField";
const QString EntryAttributes::SearchInGroupName = "SearchIn";
const QString EntryAttributes::SearchTextGroupName = "SearchText";

const QString EntryAttributes::RememberCmdExecAttr = "_EXEC_CMD";


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
    return m_attributes.contains(key);
}

QList<QString> EntryAttributes::customKeys() const
{
    QList<QString> customKeys;
    const QList<QString> keyList = keys();
    for (const QString& key : keyList) {
        if (!isDefaultAttribute(key)) {
            customKeys.append(key);
        }
    }
    return customKeys;
}

/**
 * Return attribute value.
 *
 * @param key attribute key
 * @param unprotect decrypt if value is protected
 * @return value or null string if value is protected and unprotect is false
 */
QString EntryAttributes::value(const QString& key, bool unprotect) const
{
    if (isProtected(key)) {
        if (unprotect) {
            bool ok;
            return QString::fromUtf8(Crypto::memDecryptValue(m_attributes.value(key), &ok));
        }
        return {};
    }
    return QString::fromUtf8(m_attributes.value(key));
}

/**
 * Return values of all given as a list.
 *
 * @param keys list of keys to retrieve
 * @param unprotect also return protected fields (will be omitted if set to false)
 * @return list of values
 */
QList<QString> EntryAttributes::values(const QList<QString>& keys, bool unprotect) const
{
    QList<QString> values;
    for (const QString& key : keys) {
        if (!isProtected(key) || unprotect) {
            values.append(value(key));
        }
    }
    return values;
}

bool EntryAttributes::contains(const QString& key) const
{
    return m_attributes.contains(key);
}

/**
 * Check if attributes contain an entry with the given value.
 *
 * @param value value to search for
 * @param unprotect whether to decrypt and search protected fields as well
 * @return true if value was found
 */
bool EntryAttributes::containsValue(const QString& value, bool unprotect) const
{
    return values(keys(), unprotect).contains(value.toUtf8());
}

bool EntryAttributes::isProtected(const QString& key) const
{
    return m_protectedAttributes.contains(key);
}

bool EntryAttributes::isReference(const QString& key) const
{
    if (!m_attributes.contains(key)) {
        Q_ASSERT(false);
        return false;
    }

    const QString data = value(key);
    return matchReference(data).hasMatch();
}

/**
 * Set an entry attribute.
 *
 * @param key attribute key
 * @param value attribute value
 * @param protect protect value in memory
 */
void EntryAttributes::set(const QString& key, QString value, bool protect)
{
    bool emitModified = false;

    bool addAttribute = !m_attributes.contains(key);
    bool defaultAttribute = isDefaultAttribute(key);

    QByteArray oldValue;
    bool oldProtected = !addAttribute && isProtected(key);

    bool ok;
    if (!addAttribute) {
        oldValue = m_attributes.value(key);
        if (oldProtected) {
            oldValue = Crypto::memDecryptValue(oldValue, &ok);
        }
    }

    QByteArray byteValue = value.toUtf8();
    bool changeValue = byteValue != oldValue || protect != oldProtected;
    Tools::wipeBuffer(oldValue);
    oldValue.clear();

    if (addAttribute && !defaultAttribute) {
        emit aboutToBeAdded(key);
    }

    if (addAttribute || changeValue) {
        if (protect) {
            byteValue = Crypto::memEncryptValue(byteValue, &ok);
            m_protectedAttributes.insert(key);
        } else {
            m_protectedAttributes.remove(key);
        }
        m_attributes.insert(key, byteValue);
        emitModified = true;
    }

    if (emitModified) {
        emit entryAttributesModified();
    }

    if (defaultAttribute && changeValue) {
        emit defaultKeyModified();
    } else if (addAttribute) {
        emit added(key);
    } else if (emitModified) {
        emit customKeyModified(key);
    }
}

void EntryAttributes::remove(const QString& key)
{
    Q_ASSERT(!isDefaultAttribute(key));

    if (!m_attributes.contains(key)) {
        return;
    }

    emit aboutToBeRemoved(key);

    m_attributes.remove(key);
    m_protectedAttributes.remove(key);

    emit removed(key);
    emit entryAttributesModified();
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

    QString data = value(oldKey, false);

    emit aboutToRename(oldKey, newKey);

    if (isProtected(oldKey)) {
        m_protectedAttributes.insert(newKey);
    }
    m_attributes.insert(newKey, data.toUtf8());

    emit entryAttributesModified();
    emit renamed(oldKey, newKey);
}

void EntryAttributes::copyCustomKeysFrom(const EntryAttributes* other)
{
    if (!areCustomKeysDifferent(other)) {
        return;
    }

    emit aboutToBeReset();

    // remove all non-default keys
    const QList<QString> keyList = keys();
    for (const QString& key : keyList) {
        if (!isDefaultAttribute(key)) {
            m_attributes.remove(key);
            m_protectedAttributes.remove(key);
        }
    }

    const QList<QString> otherKeyList = other->keys();
    for (const QString& key : otherKeyList) {
        if (!isDefaultAttribute(key)) {
            m_attributes.insert(key, other->m_attributes.value(key));
            if (other->isProtected(key)) {
                m_protectedAttributes.insert(key);
            }
        }
    }

    emit reset();
    emit entryAttributesModified();
}

bool EntryAttributes::areCustomKeysDifferent(const EntryAttributes* other)
{
    // check if they are equal ignoring the order of the keys
    if (keys().toSet() != other->keys().toSet()) {
        return true;
    }

    const QList<QString> keyList = keys();
    for (const QString& key : keyList) {
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
        emit aboutToBeReset();

        m_attributes = other->m_attributes;
        m_protectedAttributes = other->m_protectedAttributes;

        emit reset();
        emit entryAttributesModified();
    }
}

QUuid EntryAttributes::referenceUuid(const QString& key) const
{
    if (!m_attributes.contains(key)) {
        Q_ASSERT(false);
        return {};
    }

    auto match = matchReference(value(key));
    if (match.hasMatch()) {
        const QString uuid = match.captured("SearchText");
        if (!uuid.isEmpty()) {
            return QUuid::fromRfc4122(QByteArray::fromHex(uuid.toLatin1()));
        }
    }

    return {};
}

bool EntryAttributes::operator==(const EntryAttributes& other) const
{
    return m_attributes == other.m_attributes && m_protectedAttributes == other.m_protectedAttributes;
}

bool EntryAttributes::operator!=(const EntryAttributes& other) const
{
    return !operator==(other);
}

QRegularExpressionMatch EntryAttributes::matchReference(const QString& text)
{
    static QRegularExpression referenceRegExp(
        "\\{REF:(?<WantedField>[TUPANI])@(?<SearchIn>[TUPANIO]):(?<SearchText>[^}]+)\\}",
        QRegularExpression::CaseInsensitiveOption);

    return referenceRegExp.match(text);
}

void EntryAttributes::clear()
{
    emit aboutToBeReset();

    m_attributes.clear();
    m_protectedAttributes.clear();

    for (const QString& key : DefaultAttributes) {
        m_attributes.insert(key, "");
    }

    emit reset();
    emit entryAttributesModified();
}

int EntryAttributes::attributesSize() const
{
    int size = 0;
    for (auto it = m_attributes.cbegin(); it != m_attributes.cend(); ++it) {
        size += it.key().toUtf8().size() + value(it.key(), true).size();
    }
    return size;
}

bool EntryAttributes::isDefaultAttribute(const QString& key)
{
    return DefaultAttributes.contains(key);
}

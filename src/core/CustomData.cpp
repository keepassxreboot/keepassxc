/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "CustomData.h"

CustomData::CustomData(QObject* parent)
    : QObject(parent)
{
}

QList<QString> CustomData::keys() const
{
    return m_data.keys();
}

bool CustomData::hasKey(const QString& key) const
{
    return m_data.contains(key);
}

QString CustomData::value(const QString& key) const
{
    return m_data.value(key);
}

bool CustomData::contains(const QString& key) const
{
    return m_data.contains(key);
}

bool CustomData::containsValue(const QString& value) const
{
    return m_data.values().contains(value);
}

void CustomData::set(const QString& key, const QString& value)
{
    bool addAttribute = !m_data.contains(key);
    bool changeValue = !addAttribute && (m_data.value(key) != value);

    if (addAttribute) {
        emit aboutToBeAdded(key);
     }

    if (addAttribute || changeValue) {
        m_data.insert(key, value);
        emit modified();
    }

    if (addAttribute) {
        emit added(key);
    }
}

void CustomData::remove(const QString& key)
{
    emit aboutToBeRemoved(key);

    m_data.remove(key);

    emit removed(key);
    emit modified();
}

void CustomData::rename(const QString& oldKey, const QString& newKey)
{
    const bool containsOldKey = m_data.contains(oldKey);
    const bool containsNewKey = m_data.contains(newKey);
    Q_ASSERT(containsOldKey && !containsNewKey);
    if (!containsOldKey || containsNewKey) {
        return;
    }

    QString data = value(oldKey);

    emit aboutToRename(oldKey, newKey);

    m_data.remove(oldKey);
    m_data.insert(newKey, data);

    emit modified();
    emit renamed(oldKey, newKey);
}

void CustomData::copyDataFrom(const CustomData* other)
{
    if (*this == *other) {
        return;
    }

    emit aboutToBeReset();

    m_data = other->m_data;

    emit reset();
    emit modified();
}
bool CustomData::operator==(const CustomData& other) const
{
    return (m_data == other.m_data);
}

bool CustomData::operator!=(const CustomData& other) const
{
    return (m_data != other.m_data);
}

void CustomData::clear()
{
    emit aboutToBeReset();

    m_data.clear();

    emit reset();
    emit modified();
}

bool CustomData::isEmpty() const
{
    return m_data.isEmpty();
}

int CustomData::size() const
{
    return m_data.size();
}

int CustomData::dataSize() const
{
    int size = 0;

    QHashIterator<QString, QString> i(m_data);
    while (i.hasNext()) {
        i.next();
        size += i.key().toUtf8().size() + i.value().toUtf8().size();
    }
    return size;
}

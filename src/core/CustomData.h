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

#ifndef KEEPASSXC_CUSTOMDATA_H
#define KEEPASSXC_CUSTOMDATA_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

class CustomData : public QObject
{
Q_OBJECT

public:
    explicit CustomData(QObject* parent = nullptr);
    QList<QString> keys() const;
    bool hasKey(const QString& key) const;
    QString value(const QString& key) const;
    bool contains(const QString& key) const;
    bool containsValue(const QString& value) const;
    void set(const QString& key, const QString& value);
    void remove(const QString& key);
    void rename(const QString& oldKey, const QString& newKey);
    void clear();
    bool isEmpty() const;
    int size() const;
    int dataSize() const;
    void copyDataFrom(const CustomData* other);
    bool operator==(const CustomData& other) const;
    bool operator!=(const CustomData& other) const;


signals:
    void modified();
    void aboutToBeAdded(const QString& key);
    void added(const QString& key);
    void aboutToBeRemoved(const QString& key);
    void removed(const QString& key);
    void aboutToRename(const QString& oldKey, const QString& newKey);
    void renamed(const QString& oldKey, const QString& newKey);
    void aboutToBeReset();
    void reset();

private:
    QHash<QString, QString> m_data;
};

#endif // KEEPASSXC_CUSTOMDATA_H

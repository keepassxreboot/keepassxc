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

#ifndef KEEPASSX_ENTRYATTRIBUTES_H
#define KEEPASSX_ENTRYATTRIBUTES_H

#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QObject>
#include <QtCore/QStringList>

class EntryAttributes : public QObject
{
    Q_OBJECT

public:
    explicit EntryAttributes(QObject* parent = 0);
    QList<QString> keys() const;
    QString value(const QString& key) const;
    bool isProtected(const QString& key) const;
    void set(const QString& key, const QString& value, bool protect = false);
    void remove(const QString& key);
    void rename(const QString& oldKey, const QString& newKey);
    void copyCustomKeysFrom(const EntryAttributes* other);
    bool areCustomKeysDifferent(const EntryAttributes* other);
    void clear();
    EntryAttributes& operator=(const EntryAttributes& other);
    bool operator==(const EntryAttributes& other) const;
    bool operator!=(const EntryAttributes& other) const;

    static const QStringList DEFAULT_ATTRIBUTES;
    static bool isDefaultAttribute(const QString& key);

Q_SIGNALS:
    void modified();
    void defaultKeyModified();
    void customKeyModified(QString key);
    void aboutToBeAdded(QString key);
    void added(QString key);
    void aboutToBeRemoved(QString key);
    void removed(QString key);
    void aboutToRename(QString oldKey, QString newKey);
    void renamed(QString oldKey, QString newKey);
    void aboutToBeReset();
    void reset();

private:
    QMap<QString, QString> m_attributes;
    QSet<QString> m_protectedAttributes;
};

#endif // KEEPASSX_ENTRYATTRIBUTES_H

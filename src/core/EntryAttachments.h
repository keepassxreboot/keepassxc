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

#ifndef KEEPASSX_ENTRYATTACHMENTS_H
#define KEEPASSX_ENTRYATTACHMENTS_H

#include <QMap>
#include <QObject>

class QStringList;

class EntryAttachments : public QObject
{
    Q_OBJECT

public:
    explicit EntryAttachments(QObject* parent = nullptr);
    QList<QString> keys() const;
    bool hasKey(const QString& key) const;
    QSet<QByteArray> values() const;
    QByteArray value(const QString& key) const;
    void set(const QString& key, const QByteArray& value);
    void remove(const QString& key);
    void remove(const QStringList& keys);
    bool isEmpty() const;
    void clear();
    void copyDataFrom(const EntryAttachments* other);
    bool operator==(const EntryAttachments& other) const;
    bool operator!=(const EntryAttachments& other) const;
    int attachmentsSize() const;

signals:
    void modified();
    void keyModified(const QString& key);
    void aboutToBeAdded(const QString& key);
    void added(const QString& key);
    void aboutToBeRemoved(const QString& key);
    void removed(const QString& key);
    void aboutToBeReset();
    void reset();

private:
    QMap<QString, QByteArray> m_attachments;
};

#endif // KEEPASSX_ENTRYATTACHMENTS_H

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

#include <QtCore/QMap>
#include <QtCore/QSet>

class EntryAttachments : public QObject
{
    Q_OBJECT

public:
    explicit EntryAttachments(QObject* parent = 0);
    QList<QString> keys() const;
    QByteArray value(const QString& key) const;
    void set(const QString& key, const QByteArray& value);
    void remove(const QString& key);
    void copyFrom(const EntryAttachments* other);
    void clear();
    bool operator!=(const EntryAttachments& other) const;

Q_SIGNALS:
    void modified();
    void keyModified(QString key);
    void aboutToBeAdded(QString key);
    void added(QString key);
    void aboutToBeRemoved(QString key);
    void removed(QString key);
    void aboutToBeReset();
    void reset();

private:
    QMap<QString, QByteArray> m_attachments;
};

#endif // KEEPASSX_ENTRYATTACHMENTS_H

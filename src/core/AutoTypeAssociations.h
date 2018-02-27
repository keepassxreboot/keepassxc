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

#ifndef KEEPASSX_AUTOTYPEASSOCIATIONS_H
#define KEEPASSX_AUTOTYPEASSOCIATIONS_H

#include <QObject>

class AutoTypeAssociations : public QObject
{
    Q_OBJECT

public:
    struct Association
    {
        QString window;
        QString sequence;

        bool operator==(const AutoTypeAssociations::Association& other) const;
        bool operator!=(const AutoTypeAssociations::Association& other) const;
    };

    explicit AutoTypeAssociations(QObject* parent = nullptr);
    void copyDataFrom(const AutoTypeAssociations* other);
    void add(const AutoTypeAssociations::Association& association);
    void remove(int index);
    void removeEmpty();
    void update(int index, const AutoTypeAssociations::Association& association);
    AutoTypeAssociations::Association get(int index) const;
    QList<AutoTypeAssociations::Association> getAll() const;
    int size() const;
    int associationsSize() const;
    void clear();

private:
    QList<AutoTypeAssociations::Association> m_associations;

signals:
    void modified();
    void dataChanged(int index);
    void aboutToAdd(int index);
    void added(int index);
    void aboutToRemove(int index);
    void removed(int index);
    void aboutToReset();
    void reset();
};

Q_DECLARE_TYPEINFO(AutoTypeAssociations::Association, Q_MOVABLE_TYPE);

#endif // KEEPASSX_AUTOTYPEASSOCIATIONS_H

/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_MODIFIABLEOBJECT_H
#define KEEPASSXC_MODIFIABLEOBJECT_H

#include <QObject>

class ModifiableObject : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

public:
    /**
     * @brief check if the modified signal is enabled.
     * Note that this is NOT the same as m_emitModified.
     * The signal is enabled if neither the current object nor any of its parents disabled the signal.
     */
    bool modifiedSignalEnabled() const;

public slots:
    /**
     * @brief set whether the modified signal should be emitted from this object and all its children.
     *
     * This means that even after calling `setEmitModified(true)` on this object,
     * the signal may still be blocked in one of its parents.
     *
     * @param value
     */
    void setEmitModified(bool value);

protected:
    void emitModified();

signals:
    void modified();
    void emitModifiedChanged(bool value);

private:
    bool m_emitModified{true};
};

#endif // KEEPASSXC_MODIFIABLEOBJECT_H

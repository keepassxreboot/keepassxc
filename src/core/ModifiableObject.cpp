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

#include "ModifiableObject.h"

namespace
{
    template <typename T> T findParent(const QObject* obj)
    {
        if (!obj) {
            return {};
        }

        auto p = obj->parent();
        while (p) {
            auto casted = qobject_cast<T>(p);
            if (casted) {
                return casted;
            }
            p = p->parent();
        }
        return {};
    }
} // namespace

bool ModifiableObject::modifiedSignalEnabled() const
{
    auto p = this;
    while (p) {
        if (!p->m_emitModified) {
            return false;
        }
        p = findParent<ModifiableObject*>(p);
    }
    return true;
}

void ModifiableObject::setEmitModified(bool value)
{
    if (m_emitModified != value) {
        m_emitModified = value;
        emit emitModifiedChanged(m_emitModified);
    }
}

void ModifiableObject::emitModified()
{
    if (modifiedSignalEnabled()) {
        emit modified();
    }
}

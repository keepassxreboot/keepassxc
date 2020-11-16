/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "PromptAdaptor.h"

#include "fdosecrets/objects/Prompt.h"

namespace FdoSecrets
{

    PromptAdaptor::PromptAdaptor(PromptBase* parent)
        : DBusAdaptor(parent)
    {
        // p() isn't ready yet as this is called in Parent's constructor
        connect(parent, &PromptBase::completed, this, [this](bool dismissed, QVariant result) {
            // make sure the result contains a valid value, otherwise QDBusVariant refuses to marshall it.
            if (!result.isValid()) {
                result = QString{};
            }
            emit Completed(dismissed, QDBusVariant(std::move(result)));
        });
    }

    void PromptAdaptor::Prompt(const QString& windowId)
    {
        p()->prompt(windowId).handle(p());
    }

    void PromptAdaptor::Dismiss()
    {
        p()->dismiss().handle(p());
    }

} // namespace FdoSecrets

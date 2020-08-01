/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_CLOSE_H
#define KEEPASSXC_CLOSE_H

#include <QStringList>

#include "Command.h"

class Close final : public Command
{
public:
    Close(const QString& name, const QString& description)
        : Command(name, description, runmodeMask(Runmode::InteractiveCmd))
    {}

private:
    int execImpl(CommandCtx& ctx, const QCommandLineParser& parser) override;
};
DECL_TRAITS(Close, "close", "Close the currently opened database.");

#endif // KEEPASSXC_CLOSE_H

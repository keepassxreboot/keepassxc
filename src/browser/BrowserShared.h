/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_BROWSERSHARED_H
#define KEEPASSXC_BROWSERSHARED_H

#include <QString>

namespace BrowserShared
{
    constexpr int NATIVEMSG_MAX_LENGTH = 1024 * 1024;

    enum SupportedBrowsers : int
    {
        CHROME = 0,
        CHROMIUM,
        FIREFOX,
        VIVALDI,
        TOR_BROWSER,
        BRAVE,
        EDGE,
        CUSTOM,
        MAX_SUPPORTED
    };

    QString localServerPath();
} // namespace BrowserShared

#endif // KEEPASSXC_BROWSERSHARED_H

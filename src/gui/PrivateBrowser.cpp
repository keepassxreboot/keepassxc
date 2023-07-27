/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "PrivateBrowser.h"
#include "MainWindow.h"
#include "gui/osutils/OSUtils.h"
#include <QProcess>

void PrivateBrowser::openUrl(const QUrl& url)
{
    const auto applicationPath = osUtils->getDefaultApplicationForUrl(url);
    if (applicationPath.isEmpty()) {
        return;
    }

    const auto privateModeArg = getPrivateModeArg(applicationPath);

#if defined(Q_OS_MAC)
    QStringList args{"-n", applicationPath, "--args", privateModeArg, url.toString()};
    QProcess::startDetached("open", args);
#elif defined(Q_OS_WIN)

#elif defined(Q_OS_LINUX)
    QStringList args{privateModeArg, url.toString()};
    QProcess::startDetached(applicationPath, args);
#endif
    getMainWindow()->lower();
}

// Returns command line argument for private mode for selected browser
QString PrivateBrowser::getPrivateModeArg(const QString& applicationPath)
{
    const auto path = applicationPath.toLower();
    return path.contains("fox") || path.contains("librewolf") ? "-private-window" : "-incognito";
}

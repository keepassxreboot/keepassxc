/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "BrowserShared.h"

#include "config-keepassx.h"

#include <QDir>
#include <QStandardPaths>
#if defined(KEEPASSXC_DIST_SNAP)
#include <QProcessEnvironment>
#endif
#if defined(Q_OS_MACOS)
#include <unistd.h>
#endif

namespace BrowserShared
{
    // Get user's temporary directory. Bypasses $TMPDIR on macOS.
    QString getUserTemporaryDirectory()
    {
#if defined(Q_OS_MACOS)
        // In macOS QStandardPaths::TempLocation can be overridden with $TMPDIR. Use the location provided by the OS.
        // Otherwise the socket will be created to incorrect path, and connection with the browser extension breaks.
        const auto len = confstr(_CS_DARWIN_USER_TEMP_DIR, nullptr, 0);
        if (len > 0 && len <= 1024) {
            char rawPath[1024];
            confstr(_CS_DARWIN_USER_TEMP_DIR, rawPath, len);
            auto temporaryDirectory = QString::fromUtf8(rawPath);
            if (temporaryDirectory.endsWith("/")) {
                temporaryDirectory.chop(1);
            }
            return temporaryDirectory;
        }
#endif
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }

    QString localServerPath()
    {
        const auto serverName = QStringLiteral("/org.keepassxc.KeePassXC.BrowserServer");
#if defined(KEEPASSXC_DIST_SNAP)
        return QProcessEnvironment::systemEnvironment().value("SNAP_USER_COMMON") + serverName;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        // This returns XDG_RUNTIME_DIR or else a temporary subdirectory.
        QString path = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);

        // Put the socket in a dedicated directory.
        // This directory will be easily mountable by sandbox containers.
        QString subPath = path + "/app/org.keepassxc.KeePassXC";
        QDir().mkpath(subPath);

        QString socketPath = subPath + serverName;
#ifndef KEEPASSXC_DIST_FLATPAK
        // Create a symlink at the legacy location for backwards compatibility.
        const auto origSocketPath = path + serverName;
        QFile::remove(origSocketPath);
        QFile::link(socketPath, origSocketPath);
#endif

        return socketPath;
#elif defined(Q_OS_WIN)
        // Windows uses named pipes
        return serverName + "_" + qgetenv("USERNAME");
#else // Q_OS_MACOS and others
        return getUserTemporaryDirectory() + serverName;
#endif
    }
} // namespace BrowserShared

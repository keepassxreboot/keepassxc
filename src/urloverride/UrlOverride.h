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

#ifndef KEEPASSXC_URLOVERRIDE_H
#define KEEPASSXC_URLOVERRIDE_H

#include <QList>
#include <QString>

// Global URL scheme override support. Lets a URL scheme (e.g. "ssh", "kdbx") be mapped to an
// external command template instead of the default browser action. Storage is self-contained
// here rather than in core/Config so the whole feature can be built out via
// KPXC_FEATURE_URLOVERRIDE.
namespace UrlOverride
{
    struct Rule
    {
        bool enabled;
        QString scheme;
        QString command;

        bool operator==(const Rule& other) const
        {
            return enabled == other.enabled && scheme == other.scheme && command == other.command;
        }
    };

    QList<Rule> getRules();
    void setRules(const QList<Rule>& rules);
    QString findCommand(const QString& url);

    // Runs a "cmd://"-style external command detached from KeePassXC. On Windows, ensures a
    // visible console window is allocated for console-subsystem programs (e.g. ssh, plink) while
    // leaving GUI-subsystem programs (e.g. a browser) unaffected.
    void executeCommand(const QString& program, const QStringList& arguments);
} // namespace UrlOverride

#endif // KEEPASSXC_URLOVERRIDE_H

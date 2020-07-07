/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "MacUtils.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

#include <CoreGraphics/CGEventSource.h>


QPointer<MacUtils> MacUtils::m_instance = nullptr;

MacUtils::MacUtils(QObject* parent)
    : OSUtilsBase(parent)
    , m_appkit(new AppKit())
{
    connect(m_appkit.data(), SIGNAL(lockDatabases()), SIGNAL(lockDatabases()));
}

MacUtils::~MacUtils()
{
}

MacUtils* MacUtils::instance()
{
    if (!m_instance) {
        m_instance = new MacUtils(qApp);
    }

    return m_instance;
}

WId MacUtils::activeWindow()
{
    return m_appkit->activeProcessId();
}

bool MacUtils::raiseWindow(WId pid)
{
    return m_appkit->activateProcess(pid);
}

bool MacUtils::raiseOwnWindow()
{
    return m_appkit->activateProcess(m_appkit->ownProcessId());
}

bool MacUtils::raiseLastActiveWindow()
{
    return m_appkit->activateProcess(m_appkit->lastActiveProcessId());
}

bool MacUtils::hideOwnWindow()
{
    return m_appkit->hideProcess(m_appkit->ownProcessId());
}

bool MacUtils::isHidden()
{
    return m_appkit->isHidden(m_appkit->ownProcessId());
}

bool MacUtils::enableAccessibility()
{
    return m_appkit->enableAccessibility();
}

bool MacUtils::enableScreenRecording()
{
    return m_appkit->enableScreenRecording();
}

bool MacUtils::isDarkMode() const
{
    return m_appkit->isDarkMode();
}

QString MacUtils::getLaunchAgentFilename() const
{
    auto launchAgentDir = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/../LaunchAgents"));
    return QFile(launchAgentDir.absoluteFilePath(
        qApp->property("KPXC_QUALIFIED_APPNAME").toString().append(".plist"))).fileName();
}

bool MacUtils::isLaunchAtStartupEnabled() const
{
    return QFile::exists(getLaunchAgentFilename());
}

void MacUtils::setLaunchAtStartup(bool enable)
{
    if (enable) {
        QSettings agent(getLaunchAgentFilename(), QSettings::NativeFormat);
        agent.setValue("Label", qApp->property("KPXC_QUALIFIED_APPNAME").toString());
        agent.setValue("ProgramArguments", QStringList() << QApplication::applicationFilePath());
        agent.setValue("RunAtLoad", true);
        agent.setValue("StandardErrorPath", "/dev/null");
        agent.setValue("StandardOutPath", "/dev/null");
    } else if (isLaunchAtStartupEnabled()) {
        QFile::remove(getLaunchAgentFilename());
    }
}

bool MacUtils::isCapslockEnabled()
{
    return (CGEventSourceFlagsState(kCGEventSourceStateHIDSystemState) & kCGEventFlagMaskAlphaShift) != 0;
}

/**
 * Toggle application state between foreground app and UIElement app.
 * Foreground apps have dock icons, UIElement apps do not.
 *
 * @param foreground whether app is foreground app
 */
void MacUtils::toggleForegroundApp(bool foreground)
{
    m_appkit->toggleForegroundApp(foreground);
}

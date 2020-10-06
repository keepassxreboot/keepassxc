/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "BootstrapApplication.h"

#include "config-keepassx.h"
#include "core/Bootstrap.h"
#include "core/Config.h"
#include "core/Translator.h"
#include "gui/MessageBox.h"


namespace BootstrapApplication
{
    /**
     * Perform early application bootstrapping such as setting up search paths,
     * configuration OS security properties, and loading translators.
     * A QApplication object has to be instantiated before calling this function.
     */
    void bootstrapApplication()
    {
        Bootstrap::bootstrap();
        Translator::installTranslators();

#ifdef Q_OS_WIN
        // Qt on Windows uses "MS Shell Dlg 2" as the default font for many widgets, which resolves
        // to Tahoma 8pt, whereas the correct font would be "Segoe UI" 9pt.
        // Apparently, some widgets are already using the correct font. Thanks, MuseScore for this neat fix!
        QApplication::setFont(QApplication::font("QMessageBox"));
#endif

        MessageBox::initializeButtonDefs();

#ifdef Q_OS_MACOS
        // Don't show menu icons on OSX
        QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    }

    /**
     * Restore the main window's state after launch
     *
     * @param mainWindow the main window whose state to restore
     */
    void restoreMainWindowState(MainWindow& mainWindow)
    {
        // start minimized if configured
        if (config()->get(Config::GUI_MinimizeOnStartup).toBool()) {
            mainWindow.hideWindow();
        } else {
            mainWindow.bringToFront();
        }

        if (config()->get(Config::OpenPreviousDatabasesOnStartup).toBool()) {
            const QStringList fileNames = config()->get(Config::LastOpenedDatabases).toStringList();
            for (const QString& filename : fileNames) {
                if (!filename.isEmpty() && QFile::exists(filename)) {
                    mainWindow.openDatabase(filename);
                }
            }
            auto lastActiveFile = config()->get(Config::LastActiveDatabase).toString();
            if (!lastActiveFile.isEmpty()) {
                mainWindow.openDatabase(lastActiveFile);
            }
        }
    }

} // namespace BootstrapApplication

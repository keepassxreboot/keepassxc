/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_MACUTILS_H
#define KEEPASSXC_MACUTILS_H

#include "gui/osutils/OSUtilsBase.h"
#include "AppKit.h"

#include <QPointer>
#include <QScopedPointer>
#include <qwindowdefs.h>

class MacUtils : public OSUtilsBase
{
    Q_OBJECT

public:
    static MacUtils* instance();

    WId activeWindow();
    bool raiseWindow(WId pid);
    bool raiseLastActiveWindow();
    bool raiseOwnWindow();
    bool hideOwnWindow();
    bool isHidden();
    bool isDarkMode() override;
    bool enableAccessibility();
    bool enableScreenRecording();

signals:
    void lockDatabases();

protected:
    explicit MacUtils(QObject* parent = nullptr);
    ~MacUtils() override;

private:
    QScopedPointer<AppKit> m_appkit;
    static QPointer<MacUtils> m_instance;

    Q_DISABLE_COPY(MacUtils)
};

inline MacUtils* macUtils()
{
    return MacUtils::instance();
}

#endif // KEEPASSXC_MACUTILS_H

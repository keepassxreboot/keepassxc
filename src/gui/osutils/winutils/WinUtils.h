/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_WINUTILS_H
#define KEEPASSXC_WINUTILS_H

#include "gui/osutils/OSUtilsBase.h"

#include <QAbstractNativeEventFilter>
#include <QPointer>
#include <QScopedPointer>

class WinUtils : public OSUtilsBase
{
    Q_OBJECT

public:
    static WinUtils* instance();
    static void registerEventFilters();

    bool isDarkMode() const override;
    bool isLaunchAtStartupEnabled() const override;
    void setLaunchAtStartup(bool enable) override;
    bool isCapslockEnabled() override;

protected:
    explicit WinUtils(QObject* parent = nullptr);
    ~WinUtils() override;

private:
    class DWMEventFilter : public QAbstractNativeEventFilter
    {
    public:
        bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override;
    };

    static QPointer<WinUtils> m_instance;
    static QScopedPointer<DWMEventFilter> m_eventFilter;

    Q_DISABLE_COPY(WinUtils)
};

inline WinUtils* winUtils()
{
    return WinUtils::instance();
}

#endif // KEEPASSXC_WINUTILS_H

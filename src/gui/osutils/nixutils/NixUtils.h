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

#ifndef KEEPASSXC_NIXUTILS_H
#define KEEPASSXC_NIXUTILS_H

#include "gui/osutils/OSUtilsBase.h"
#include <QAbstractNativeEventFilter>
#include <QPointer>

class NixUtils : public OSUtilsBase, QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    static NixUtils* instance();

    bool isDarkMode() const override;
    bool isLaunchAtStartupEnabled() const override;
    void setLaunchAtStartup(bool enable) override;
    bool isCapslockEnabled() override;

    void registerNativeEventFilter() override;

    bool registerGlobalShortcut(const QString& name,
                                Qt::Key key,
                                Qt::KeyboardModifiers modifiers,
                                QString* error = nullptr) override;
    bool unregisterGlobalShortcut(const QString& name) override;

signals:
    void keymapChanged();

private:
    explicit NixUtils(QObject* parent = nullptr);
    ~NixUtils() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override;
    QString getAutostartDesktopFilename(bool createDirs = false) const;

    bool triggerGlobalShortcut(uint keycode, uint modifiers);

    static QPointer<NixUtils> m_instance;

    struct globalShortcut
    {
        uint nativeKeyCode;
        uint nativeModifiers;
    };
    QHash<QString, QSharedPointer<globalShortcut>> m_globalShortcuts;

    Q_DISABLE_COPY(NixUtils)
};

inline NixUtils* nixUtils()
{
    return NixUtils::instance();
}

#endif // KEEPASSXC_NIXUTILS_H

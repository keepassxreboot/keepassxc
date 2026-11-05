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
#include <QHash>
#include <QPointer>
#include <QSharedPointer>

class GlobalShortcutsPortal;
class RemoteDesktopPortal;
class SettingsPortal;

class NixUtils : public OSUtilsBase, QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    static NixUtils* instance();

    bool isDarkMode() const override;
    bool isStatusBarDark() const override;
    bool isLaunchAtStartupEnabled() const override;
    void setLaunchAtStartup(bool enable) override;
    bool isCapslockEnabled() override;
    void setUserInputProtection(bool enable) override;

    void registerNativeEventFilter() override;

    bool registerGlobalShortcut(const QString& name,
                                Qt::Key key,
                                Qt::KeyboardModifiers modifiers,
                                QString* error = nullptr) override;
    bool unregisterGlobalShortcut(const QString& name) override;

    bool canPreventScreenCapture() const override
    {
        return false;
    }
    bool setClipboardText(const QString& text) override;
    bool clearClipboardText(const QString& text) override;
    bool isClipboardAvailable() override;

    quint64 getProcessStartTime() const;

    bool externalGlobalShortcutsConfigurator() override;
    bool isWayland() const;

    GlobalShortcutsPortal* globalShortcutsPortal();
    RemoteDesktopPortal* remoteDesktopPortal();
    SettingsPortal* settingsPortal();

public slots:
    void configureGlobalShortcut(const QString& name) override;

private slots:
    void launchAtStartupRequested(uint response, const QVariantMap& results);

private:
    explicit NixUtils(QObject* parent = nullptr);
    ~NixUtils() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    QString getAutostartDesktopFilename(bool createDirs = false) const;

    bool triggerGlobalShortcut(uint keycode, uint modifiers);

    static QPointer<NixUtils> m_instance;

    struct globalShortcut
    {
        uint nativeKeyCode;
        uint nativeModifiers;
    };
    QHash<QString, QSharedPointer<globalShortcut>> m_globalShortcuts;

    GlobalShortcutsPortal* m_globalShortcutsPortal = nullptr;
    RemoteDesktopPortal* m_remoteDesktopPortal = nullptr;
    SettingsPortal* m_settingsPortal = nullptr;

    Q_DISABLE_COPY(NixUtils)
};

inline NixUtils* nixUtils()
{
    return NixUtils::instance();
}

#endif // KEEPASSXC_NIXUTILS_H

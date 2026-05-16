/*
 * Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REMOTEDESKTOPPORTAL_H
#define KEEPASSXC_REMOTEDESKTOPPORTAL_H

#include "gui/osutils/nixutils/DesktopPortal.h"

#include <QVector>
#include <optional>

class QTimer;

class RemoteDesktopPortal : public DesktopPortal
{
    Q_OBJECT

public:
    explicit RemoteDesktopPortal(QObject* parent = nullptr);

    bool isAvailable() const;
    bool isClipboardAvailable() const;
    void tryStartSession();
    void finishSession();
    void closeSession();
    void waitForSession();
    QString errorString() const;

    bool sendKeysym(int keysym, const QVector<int>& modifierKeysyms, QString& error);

    bool setClipboardText(const QString& text);
    bool clearClipboardText();

signals:
    void sessionStartupFinished();

protected:
    void onSessionClosed(const QVariantMap& details) override;

private:
    enum class ClipboardState
    {
        Disabled,
        Enabled,
        Owned,
    };

    QString request(const std::function<void(const QVariantMap&)>& handler);
    void selectDevices();
    void requestClipboard();
    void startSession();
    void resetClipboard();
    void scheduleCloseSession();
    void finishStartup();

    void handleSelectionTransfer(const QDBusObjectPath& sessionHandle, const QString& mimeType, uint serial);
    void handleSelectionOwnerChanged(const QDBusObjectPath& sessionHandle, const QVariantMap& options);

    bool m_sessionStarting = false;
    QString m_restoreToken;
    QString m_error;
    QTimer* m_closeSessionTimer;
    ClipboardState m_clipboardState = ClipboardState::Disabled;
    QString m_clipboardText;
    mutable std::optional<bool> m_clipboardAvailable;
};

#endif // KEEPASSXC_REMOTEDESKTOPPORTAL_H

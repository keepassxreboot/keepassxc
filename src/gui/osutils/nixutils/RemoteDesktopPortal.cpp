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

#include "RemoteDesktopPortal.h"

#include "core/Config.h"
#include "xdp_clipboard.h"
#include "xdp_remotedesktop.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

Q_GLOBAL_STATIC_WITH_ARGS(OrgFreedesktopPortalRemoteDesktopInterface,
                          s_remoteDesktopInterface,
                          ("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           QDBusConnection::sessionBus()));

Q_GLOBAL_STATIC_WITH_ARGS(OrgFreedesktopPortalClipboardInterface,
                          s_clipboardInterface,
                          ("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           QDBusConnection::sessionBus()));

RemoteDesktopPortal::RemoteDesktopPortal(QObject* parent)
    : DesktopPortal(parent)
    , m_closeSessionTimer(new QTimer(this))
{
    m_closeSessionTimer->setSingleShot(true);
    connect(m_closeSessionTimer, &QTimer::timeout, this, &RemoteDesktopPortal::closeSession);

    connect(s_clipboardInterface,
            &OrgFreedesktopPortalClipboardInterface::SelectionTransfer,
            this,
            &RemoteDesktopPortal::handleSelectionTransfer);
    connect(s_clipboardInterface,
            &OrgFreedesktopPortalClipboardInterface::SelectionOwnerChanged,
            this,
            &RemoteDesktopPortal::handleSelectionOwnerChanged);
}

bool RemoteDesktopPortal::isAvailable() const
{
    if (!s_remoteDesktopInterface->isValid()) {
        qWarning() << "XDG Remote Desktop portal is not available, Auto-Type disabled";
        return false;
    }
    if ((s_remoteDesktopInterface->availableDeviceTypes() & 1) == 0) {
        qWarning() << "XDG Remote Desktop portal does not support keyboard input, Auto-Type disabled";
        return false;
    }
    return true;
}

bool RemoteDesktopPortal::isClipboardAvailable() const
{
    if (!m_clipboardAvailable.has_value()) {
        m_clipboardAvailable = s_clipboardInterface->isValid() && s_clipboardInterface->version() > 0;
    }

    return m_clipboardAvailable.value();
}

void RemoteDesktopPortal::finishSession()
{
    if (!config()->get(Config::AutoTypeDesktopPortalPersistConnection).toBool()
        && m_clipboardState != ClipboardState::Owned) {
        scheduleCloseSession();
    }
}

void RemoteDesktopPortal::closeSession()
{
    m_closeSessionTimer->stop();
    resetClipboard();
    m_sessionStarting = false;
    DesktopPortal::closeSession();
}

void RemoteDesktopPortal::waitForSession()
{
    tryStartSession();

    if (m_sessionStarting) {
        QEventLoop loop;
        connect(this, &RemoteDesktopPortal::sessionStartupFinished, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

QString RemoteDesktopPortal::errorString() const
{
    return m_error;
}

bool RemoteDesktopPortal::sendKeysym(int keysym, const QVector<int>& modifierKeysyms, QString& error)
{
    if (!hasSession()) {
        error = tr("Remote desktop session is not active");
        return false;
    }

    auto waitForOkReply = [&](QDBusPendingReply<>& reply) {
        reply.waitForFinished();
        if (reply.isError()) {
            error = reply.error().message();
            return false;
        }
        return true;
    };

    QDBusPendingReply<> reply;
    for (auto modifier : modifierKeysyms) {
        reply = s_remoteDesktopInterface->NotifyKeyboardKeysym(
            sessionPath(), {}, modifier, uint(1)); // Assign rvalue to lvalue
        if (!waitForOkReply(reply)) { // Pass lvalue to lambda
            return false;
        }
    }

    reply = s_remoteDesktopInterface->NotifyKeyboardKeysym(sessionPath(), {}, keysym, uint(1));
    if (!waitForOkReply(reply)) {
        return false;
    }
    reply = s_remoteDesktopInterface->NotifyKeyboardKeysym(sessionPath(), {}, keysym, uint(0));
    if (!waitForOkReply(reply)) {
        return false;
    }

    for (auto modifier : modifierKeysyms) {
        reply = s_remoteDesktopInterface->NotifyKeyboardKeysym(sessionPath(), {}, modifier, uint(0));
        if (!waitForOkReply(reply)) {
            return false;
        }
    }

    return true;
}

bool RemoteDesktopPortal::setClipboardText(const QString& text)
{
    if (!config()->get(Config::AutoTypeDesktopPortalUseClipboard).toBool() || !isClipboardAvailable()) {
        return false;
    }

    waitForSession();

    if (!m_error.isEmpty() || !hasSession() || m_clipboardState == ClipboardState::Disabled) {
        finishSession();
        return false;
    }

    m_clipboardText = text;
    auto reply = s_clipboardInterface->SetSelection(sessionPath(),
                                                    {{QLatin1String("mime_types"),
                                                      QStringList{QStringLiteral("text/plain;charset=utf-8"),
                                                                  QStringLiteral("text/plain"),
                                                                  QStringLiteral("x-kde-passwordManagerHint")}}});
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Failed to set remote desktop clipboard selection:" << reply.error().message();
        m_clipboardText.clear();
        m_clipboardState = ClipboardState::Enabled;
        finishSession();
        return false;
    }

    m_clipboardState = ClipboardState::Owned;
    return true;
}

bool RemoteDesktopPortal::clearClipboardText()
{
    if (!hasSession()) {
        return false;
    }

    auto wasEnabled = m_clipboardState != ClipboardState::Disabled;
    auto success = true;
    if (m_clipboardState == ClipboardState::Owned) {
        auto reply = s_clipboardInterface->SetSelection(sessionPath(), {{QLatin1String("mime_types"), QStringList()}});
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "Failed to clear remote desktop clipboard selection:" << reply.error().message();
            success = false;
        }
    }

    m_clipboardText.clear();

    if (!success) {
        closeSession();
    } else if (wasEnabled && hasSession()) {
        m_clipboardState = ClipboardState::Enabled;
    }

    if (success && !config()->get(Config::AutoTypeDesktopPortalPersistConnection).toBool()) {
        scheduleCloseSession();
    }

    return success && wasEnabled;
}

void RemoteDesktopPortal::onSessionClosed(const QVariantMap& details)
{
    Q_UNUSED(details)

    resetClipboard();

    if (m_error.isEmpty()) {
        m_error = tr("Session closed");
    }

    if (m_sessionStarting) {
        finishStartup();
    }
}

QString RemoteDesktopPortal::request(const std::function<void(const QVariantMap&)>& handler)
{
    return portalRequest([this, handler](uint response, const QVariantMap& result) {
        switch (response) {
        case 0:
            handler(result);
            return;
        case 1:
            m_error = tr("User cancelled the interaction");
            break;
        default:
            m_error = tr("User interaction was canceled for unknown reason");
            break;
        }

        closeSession();
        emit sessionStartupFinished();
    });
}

void RemoteDesktopPortal::tryStartSession()
{
    m_closeSessionTimer->stop();

    if (m_sessionStarting || hasSession()) {
        return;
    }

    m_error.clear();
    m_sessionStarting = true;
    resetClipboard();

    auto sessionHandleToken = prepareSession();
    auto expectedSessionPath = sessionPath();
    auto handleToken = request([this, expectedSessionPath](const QVariantMap& results) {
        if (!isCurrentSession(expectedSessionPath)) {
            return;
        }

        auto sessionHandle = results.value("session_handle").toString();
        if (!sessionHandle.isEmpty() && sessionHandle != expectedSessionPath.path()) {
            m_error = tr("Remote desktop session handle does not match expected path");
            closeSession();
            finishStartup();
            return;
        }

        selectDevices();
    });
    auto reply = s_remoteDesktopInterface->CreateSession({
        {QLatin1String("session_handle_token"), sessionHandleToken},
        {QLatin1String("handle_token"), handleToken},
    });
    reply.waitForFinished();
    if (reply.isError()) {
        m_error = "Failed to create remote desktop session:" + reply.error().message();
        if (isCurrentSession(expectedSessionPath)) {
            closeSession();
        }
        finishStartup();
    }
}

void RemoteDesktopPortal::selectDevices()
{
    auto handleToken = request([this](const QVariantMap&) { requestClipboard(); });

    auto persistMode = config()->get(Config::AutoTypeDesktopPortalPersistMode).toUInt();
    auto options = QVariantMap{
        {QLatin1String("handle_token"), handleToken},
        {QLatin1String("types"), uint(1)},
        {QLatin1String("persist_mode"), persistMode},
    };

    auto restoreToken = config()->get(Config::AutoTypeDesktopPortalRestoreToken).toString();
    if (persistMode < 2 && !restoreToken.isEmpty()) {
        config()->set(Config::AutoTypeDesktopPortalRestoreToken, "");
    } else if (!restoreToken.isEmpty()) {
        options["restore_token"] = restoreToken;
    } else if (!m_restoreToken.isEmpty()) {
        options["restore_token"] = m_restoreToken;
    }

    auto reply = s_remoteDesktopInterface->SelectDevices(sessionPath(), options);
    reply.waitForFinished();
    if (reply.isError()) {
        m_error = "Failed to select remote desktop devices: " + reply.error().message();
        closeSession();
        emit sessionStartupFinished();
    }
}

void RemoteDesktopPortal::requestClipboard()
{
    m_clipboardState = ClipboardState::Disabled;

    if (!hasSession() || !config()->get(Config::AutoTypeDesktopPortalUseClipboard).toBool()
        || !isClipboardAvailable()) {
        startSession();
        return;
    }

    auto reply = s_clipboardInterface->RequestClipboard(sessionPath(), {});
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Failed to request remote desktop clipboard access:" << reply.error().message();
    }

    startSession();
}

void RemoteDesktopPortal::startSession()
{
    auto handleToken = request([this](const QVariantMap& results) {
        if (config()->get(Config::AutoTypeDesktopPortalUseClipboard).toBool() && isClipboardAvailable()
            && results.value("clipboard_enabled").toBool()) {
            m_clipboardState = ClipboardState::Enabled;
        } else {
            m_clipboardState = ClipboardState::Disabled;
        }

        auto persistMode = config()->get(Config::AutoTypeDesktopPortalPersistMode).toUInt();
        if (persistMode >= 2) {
            m_restoreToken = "";
            config()->set(Config::AutoTypeDesktopPortalRestoreToken, results["restore_token"].toString());
        } else if (persistMode == 1) {
            m_restoreToken = results["restore_token"].toString();
        } else {
            m_restoreToken = "";
            config()->set(Config::AutoTypeDesktopPortalRestoreToken, "");
        }
        m_error.clear();
        finishStartup();
    });

    auto reply = s_remoteDesktopInterface->Start(sessionPath(),
                                                 "",
                                                 {
                                                     {QLatin1String("handle_token"), handleToken},
                                                 });
    reply.waitForFinished();
    if (reply.isError()) {
        m_error = "Failed to start remote desktop session: " + reply.error().message();
        closeSession();
        emit sessionStartupFinished();
    }
}

void RemoteDesktopPortal::resetClipboard()
{
    m_clipboardState = ClipboardState::Disabled;
    m_clipboardText.clear();
}

void RemoteDesktopPortal::scheduleCloseSession()
{
    if (!hasSession() || config()->get(Config::AutoTypeDesktopPortalPersistConnection).toBool()) {
        return;
    }

    auto timeout = config()->get(Config::GlobalAutoTypeRetypeTime).toInt();
    if (timeout <= 0) {
        closeSession();
    } else {
        m_closeSessionTimer->start(timeout * 1000);
    }
}

void RemoteDesktopPortal::finishStartup()
{
    m_sessionStarting = false;
    emit sessionStartupFinished();
}

void RemoteDesktopPortal::handleSelectionTransfer(const QDBusObjectPath& sessionHandle,
                                                  const QString& mimeType,
                                                  uint serial)
{
    if (!isCurrentSession(sessionHandle)) {
        return;
    }

    QByteArray data;
    bool supported = true;
    if (mimeType == QLatin1String("text/plain;charset=utf-8") || mimeType == QLatin1String("text/plain")) {
        data = m_clipboardText.toUtf8();
    } else if (mimeType == QLatin1String("x-kde-passwordManagerHint")) {
        data = QByteArrayLiteral("secret");
    } else {
        supported = false;
    }

    bool success = false;
    if (m_clipboardState == ClipboardState::Owned && supported) {
        auto writeReply = s_clipboardInterface->SelectionWrite(sessionHandle, serial);
        writeReply.waitForFinished();
        if (writeReply.isError()) {
            qWarning() << "Failed to open remote desktop clipboard transfer:" << writeReply.error().message();
        } else {
            auto fd = writeReply.value();
            QFile file;
            if (fd.isValid() && file.open(fd.fileDescriptor(), QIODevice::WriteOnly, QFileDevice::DontCloseHandle)) {
                success = file.write(data) == data.size();
                file.close();
            }
        }
    }

    auto doneReply = s_clipboardInterface->SelectionWriteDone(sessionHandle, serial, success);
    doneReply.waitForFinished();
    if (doneReply.isError()) {
        qWarning() << "Failed to finish remote desktop clipboard transfer:" << doneReply.error().message();
    }
}

void RemoteDesktopPortal::handleSelectionOwnerChanged(const QDBusObjectPath& sessionHandle, const QVariantMap& options)
{
    if (!isCurrentSession(sessionHandle) || !options.contains(QLatin1String("session_is_owner"))) {
        return;
    }

    if (options.value(QLatin1String("session_is_owner")).toBool()) {
        m_clipboardState = ClipboardState::Owned;
    } else if (m_clipboardState == ClipboardState::Owned) {
        m_clipboardText.clear();
        m_clipboardState = ClipboardState::Enabled;
        if (!config()->get(Config::AutoTypeDesktopPortalPersistConnection).toBool()) {
            scheduleCloseSession();
        }
    }
}

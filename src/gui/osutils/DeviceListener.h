/*
 * Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef DEVICELISTENER_H
#define DEVICELISTENER_H

#include <QHash>
#include <QPair>
#include <QPointer>
#include <QWidget>

#if defined(Q_OS_WIN)
#include "winutils/DeviceListenerWin.h"
#elif defined(Q_OS_MACOS)
#include "macutils/DeviceListenerMac.h"
#elif defined(Q_OS_UNIX)
#include "nixutils/DeviceListenerLibUsb.h"
#endif

class QUuid;

class DeviceListener : public QWidget
{
    Q_OBJECT

public:
    typedef qintptr Handle;
    static constexpr int MATCH_ANY = -1;

    explicit DeviceListener(QWidget* parent);
    DeviceListener(const DeviceListener&) = delete;
    ~DeviceListener() override;

    /**
     * Register a hotplug notification callback.
     *
     * Fires devicePlugged() or deviceUnplugged() when the state of a matching device changes.
     * The signals are supplied with the platform-specific context and ID of the firing device.
     * Registering a new callback with the same DeviceListener will unregister any previous callbacks.
     *
     * @param arrived listen for new devices
     * @param left listen for device unplug
     * @param vendorId vendor ID to listen for or DeviceListener::MATCH_ANY
     * @param productId product ID to listen for or DeviceListener::MATCH_ANY
     * @param deviceClass device class GUID (Windows only)
     * @return callback handle
     */
    Handle registerHotplugCallback(bool arrived,
                                   bool left,
                                   int vendorId = MATCH_ANY,
                                   int productId = MATCH_ANY,
                                   const QUuid* deviceClass = nullptr);
    void deregisterHotplugCallback(Handle handle);
    void deregisterAllHotplugCallbacks();

signals:
    void devicePlugged(bool state, void* ctx, void* device);

private:
    QHash<Handle, QPointer<DEVICELISTENER_IMPL>> m_listeners;
    void connectSignals(DEVICELISTENER_IMPL* listener);
};

#endif // DEVICELISTENER_H

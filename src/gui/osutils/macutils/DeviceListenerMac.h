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

#ifndef DEVICELISTENER_MAC_H
#define DEVICELISTENER_MAC_H

#define DEVICELISTENER_IMPL DeviceListenerMac

#include <QObject>
#include <IOKit/hid/IOHIDManager.h>

class QUuid;

class DeviceListenerMac : public QObject
{
    Q_OBJECT

public:
    explicit DeviceListenerMac(QObject* parent);
    DeviceListenerMac(const DeviceListenerMac&) = delete;
    ~DeviceListenerMac() override;

    void registerHotplugCallback(bool arrived,
                                 bool left,
                                 int vendorId = -1,
                                 int productId = -1, const QUuid* = nullptr);
    void deregisterHotplugCallback();

signals:
    void devicePlugged(bool state, void* ctx, void* device);

private:
    void onDeviceStateChanged(bool state, void* device);
    IOHIDManagerRef m_mgr;
};

#endif // DEVICELISTENER_MAC_H
